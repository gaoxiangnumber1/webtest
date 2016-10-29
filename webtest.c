#define _GNU_SOURCE

#include <stdio.h> // fprintf(), printf(), perror(), fdopen(), fclose(), setvbuf(), fscanf()
#include <getopt.h> // getopt_long()
#include <stdlib.h> // atoi(), exit()
#include <string.h> // CAST(int)strlen(), memcpy()
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h> // gethostbyname()
#include <sys/types.h> // socket(), connect()
#include <signal.h> // sigaction()
#include <unistd.h> // alarm(), write(), read(), close(), pipe(), fork()

#ifndef CAST
#define CAST(type) (type)
#endif // CAST

// Define return status codes
#define FAIL -1
#define SUCCESS 0
#define SERVER_ERROR 1 // Server is not on-line
#define BAD_PARAMETER 2 // Command-line parameters are bad.
#define INTERNAL_ERROR 3 // pipe() failed, fork() failed

// Support methods: GET, HEAD, OPTIONS, TRACE
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3

// Globals:
int clients = 1; // The number of clients, default 1.
int force = 0; // 1 means Don't wait for reply from server.
char *proxy_host = NULL;  // The proxy host name, default NULL.
int proxy_port = 80; // The proxy port number, default 80(http server).
int force_reload = 0; // = 1 means Send reload request - Pragma: no-cache.
int test_time = 30; // Time for testing, default 30.
int method = METHOD_GET; // Accept method applied by user, default GET.

// Store host name.
#define MAX_HOST_NAME_SIZE 64
char host[MAX_HOST_NAME_SIZE];
// Store request contents.
#define MAX_REQUEST_SIZE 2048
char request[MAX_REQUEST_SIZE];
int request_index = 0; // Cursor to fill request array.
// Max length of legal url
#define MAX_URL_SIZE 1500
// Pipe used by parent and child processes.
int pc_pipe[2]; // pc means parent-child
// Test measurement data:
int speed = 0, failed = 0, bytes = 0;
// Timer
volatile int timer_expired = 0; // TODO: Study `volatile` in Cpp-Primer $19.8.2 P1047
// Max size of buffer that stores reply messages from server.
#define MAX_BUF_SIZE 1500

const struct option long_options[] =
{
	{"clients", required_argument, NULL, 'c'},
	{"force", no_argument, &force, 1},
	{"help", no_argument,  NULL, 'h'},
	{"proxy", required_argument,  NULL, 'p'},
	{"reload", no_argument, &force_reload, 1},
	{"time", required_argument, NULL, 't'},
	{"get", no_argument, &method, METHOD_GET},
	{"head", no_argument, &method, METHOD_HEAD},
	{"options", no_argument, &method, METHOD_OPTIONS},
	{"trace", no_argument, &method, METHOD_TRACE},
	{0, 0, 0, 0}
};

void Usage() // Print usage guide for user
{
	// Don't use TAB since it is different width on different platform.
	// Only use space to align.
	fprintf(stderr,
	        "webtest [option] URL\n"
	        "  -c|--clients <n>         Run <n> HTTP clients at once. Default 1.\n"
	        "  -f|--force               Don't wait for reply from server.\n"
	        "  -h|--help                This information.\n"
	        "  -p|--proxy <server:port> Use proxy server for request.\n"
	        "  -r|--reload              Send reload request - Pragma: no-cache.\n"
	        "  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
	        "  --get                    Use GET request method.\n"
	        "  --head                   Use HEAD request method.\n"
	        "  --options                Use OPTIONS request method.\n"
	        "  --trace                  Use TRACE request method.\n");
}

// Copy string pointed to by src to string pointed to by dest start at dest_index.
// Not including the terminating null byte ('\0')
void StrCopy(char *dest, int *dest_index, const char *src)
{
	// Since we know our dest(request/host) must be large enough,
	// so we don't need check size.
	for(; *src; ++src) // Not including the terminating null byte ('\0')
	{
		*(dest + (*dest_index)++) = *src;
	}
}

// Return the index at which the substr begin in src, if match.
// -1 means not find.
int FindFirstSubstring(const char *src, const char *substr)
{
	int length = CAST(int)strlen(src);
	for(int index = 0; index < length; ++index)
	{
		if(*(src + index) == *substr) // Match the begin character, continue match.
		{
			for(const char *find = src + index; *substr && *find; ++substr, ++find)
			{
				if(*substr != *find) // Not match, return -1.
				{
					return FAIL;
				}
			}
			return index; // Match success.
		}
	}
	return FAIL; // Not match.
}

// Return 0 if match, -1 otherwise.
int IgnoreCaseMatch(const char *large, const char *small)
{
	// For different case of the same character: lowercase > uppercase.
	// For the same case of different character: 'a' < 'z', 'A' < 'Z'.
	for(; *small; ++large, ++small)
	{
		// If *small is a letter, either lowercase or uppercase.
		if(('A' <= *small && *small <= 'Z') || ('a' <= *small && *small <= 'z'))
		{
			// The difference between the upper and lower case for the same character
			int differ = 'a' - 'A';
			if(!(*large == *small // Match exactly
			        || *large == *small - differ // Match uppercase for lowercase letter
			        || *large == *small + differ)) // Match lowercase for uppercase letter
			{
				return FAIL; // Not match.
			}
		}
		else if(*large != *small) // *small is not letter, so must match exactly.
		{
			return FAIL;
		}
	}
	return SUCCESS; // All match.
}

// Return the index of the first occurrence of ch in str: -1 if not find.
int FirstOccurIndex(const char *str, const char ch)
{
	for(int index = 0; *str; ++str, ++index)
	{
		if(*str == ch)
		{
			return index;
		}
	}
	return FAIL;
}

// Return socket if connect to <host:post> success, -1(FAIL) on error.
int TcpConnect(const char *host, int port)
{
	// Step 1. Fill struct sockaddr_in.
	struct sockaddr_in sa;
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET; // IPv4
	sa.sin_port = htons(CAST(uint16_t)port);

	// in_addr_t inet_addr(const char *cp);
	// Return: 32-bit binary network byte ordered IPv4 address; INADDR_NONE if error
	int inaddr = CAST(int)inet_addr(host);
	if(CAST(unsigned)inaddr != INADDR_NONE) // If success, i.e., if host is IP address.
	{
		memcpy(&sa.sin_addr, &inaddr, CAST(size_t)sizeof(inaddr));
	}
	else // If host is "www.example.com"
	{
		struct hostent *hp = gethostbyname(host);
		if(hp == NULL)
		{
			return FAIL;
		}
		memcpy(&sa.sin_addr, hp->h_addr, CAST(size_t)(hp->h_length));
	}

	// Step 2. Create socket.
	int sock = socket(AF_INET, SOCK_STREAM, 0); // IPv4 & TCP
	if(sock == FAIL)
	{
		return FAIL;
	}
	// Step 3. Connect to server.
	if(connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		return FAIL;
	}
	return sock;
}

void alarm_handler(int signal)
{
	timer_expired = 1;
}

void HttpTransaction(const char *host, const int port, const char *request)
{
	int req_len = CAST(int)strlen(request), socket, read_bytes;
	char buf[MAX_BUF_SIZE];

	for(int read_error = 0;;)
	{
		if(timer_expired) // If have received alarm signal.
		{
			// When timer expired, os will send SIGALRM signal that will interrupt(errno = EINTR)
			// slow system calls(connect, write, read and so on, see APUE.), that is, every time the
			// timer expired, failed will increment by 1. So, when the timer expired and failed > 0,
			// we should subtract one from failed that caused by timer expired.
			if(failed > 0)
			{
				--failed;
			}
			return; // Terminate this function.
		}

		// Step 1. Create tcp connection to the server.
		if((socket = TcpConnect(host, port)) == FAIL)
		{
			//perror("socket fail");
			++failed;
			continue;
		}
		// Step 2. Send http request to the server.
		// ssize_t write(int fd, const void *buf, size_t count);
		if(req_len != write(socket, request, CAST(size_t)req_len))
		{
			//perror("write fail");
			++failed;
			close(socket);
			continue;
		}
		// Step 3. Read reply from server if there is no force option.
		if(force == 0)
		{
			for(; timer_expired == 0 && read_error == 0;) // Read all available data from socket
			{
				if((read_bytes = CAST(int)read(socket, buf, MAX_BUF_SIZE)) == FAIL) // read error
				{
					//perror("read error");
					++failed;
					close(socket);
					read_error = 1;
				}
				else if(read_bytes==0) // Encounter end of file, stop reading.
				{
					break;
				}
				else
				{
					bytes += read_bytes;
				}
			}
		}
		if(read_error == 0) // If all read success.
		{
			// Step 4. Close tcp connection and continue loop.
			if(close(socket) == FAIL) // Close error
			{
				//perror("close fail");
				++failed;
				continue;
			}
			++speed; // Finish one request-reply http transaction.
		}
	}
}

void ParseArguments(int argc, char **argv) // Step 1. Parse arguments.
{
	// Step 1. Check whether there are enough command-line-arguments.
	if(argc == 1)
	{
		//printf("Should print Usage(), Omit.\n");
		Usage(); // Tell user how to use it.
		exit(BAD_PARAMETER);
	}

	// Step 2. Parse command-line options
	int option = 0; // Store return value from getopt_long()
	int option_index = 0; // Index of which argv-element to parse, begin with 0
	// If all arguments have been parsed, getopt_long will returns -1.
	int colon_index; // Used to parse proxy:port.
	while((option = getopt_long(argc, argv, "c:fhp:rt:v019", long_options, &option_index)) != -1)
	{
		switch(option)
		{
		case 'c':
			clients = atoi(optarg);
			//printf("option -c|--clients. client number = %d\n", clients);
			break;
		case 'f':
			force = 1;
			//printf("option -f|--force. force = %d\n", force);
			break;
		case 'h':
			//printf("option -h|--help. Should print Usage.\n");
			Usage();
			exit(BAD_PARAMETER);
		case 'p':
			//printf("option -p|--proxy <server:port> | optarg = %s | ", optarg);
			// proxy server parsing "server:port"
			// Return a pointer to the first occurrence of ':' in optarg
			// TODO: Why webbench use `strrchr(optarg, ':');` that finds
			// the last occurrence of ':' in optarg?
			colon_index = FirstOccurIndex(optarg, ':');
			proxy_host = optarg;
			if(colon_index == FAIL) // ':' not find, parse next argument.
			{
				break;
			}
			if(colon_index == 0) // Input error: ":7188", missing host name.
			{
				fprintf(stderr, "Error in option --proxy %s: Missing host name.\n", optarg);
				exit(BAD_PARAMETER);
			}
			if(colon_index == CAST(int)strlen(optarg) - 1) // Input error: "name:", missing port.
			{
				fprintf(stderr, "Error in option --proxy %s Port number is missing.\n", optarg);
				exit(BAD_PARAMETER);
			}
			*(optarg + colon_index) = 0; // Delimit host_name ans port_number
			proxy_port = atoi(optarg + colon_index + 1);
			//printf("host = %s | port = %d\n", proxy_host, proxy_port);
			break;
		case 'r':
			force_reload = 1;
			//printf("option -r|--reload. force_reload = %d\n", force_reload);
			break;
		case 't':
			test_time = atoi(optarg);
			//printf("option -t|--time <sec>. test_time = %d\n", test_time);
			break;
		case 0: // For long option, return 0 if flag != NULL
			//printf("option --%s", long_options[option_index].name);
			//if(optarg)
			//{
			//	printf(" with arg = %s", optarg);
			//}
			//printf("\n");
			break;
		}
	}

	// Step 3. Check whether there has url.
	if(optind == argc)
	{
		fprintf(stderr,"webtest: Missing URL!\n");
		Usage();
		exit(BAD_PARAMETER);
	}

	// Step 4. Deal with wrong arguments, reset to default value.
	clients = (clients <= 0 ? 1 : clients);
	test_time = (test_time <= 0 ? 30 : test_time);
}

void BuildRequest(const char *url) // Step 2. Build http request message.
{
	//printf("URL = %s\n", url);

	// Step 1. Check whether url is legal, URL syntax: `<scheme>://<host>:<port>/`
	if(proxy_host == NULL) // Check <scheme> supplied by user.
	{
		// Scheme names are case-insensitive.
		if(IgnoreCaseMatch(url, "http://") != 0)
		{
			fprintf(stderr, "\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
			exit(BAD_PARAMETER);
		}
	}
	int first_colon = FindFirstSubstring(url, "://"); // The index of the first ':' appears in url.
	if(first_colon == FAIL) // If "://" can't be found in url.
	{
		fprintf(stderr, "\n%s: is not a valid URL.\n", url);
		exit(BAD_PARAMETER);
	}
	int url_host_begin = first_colon + 3; // Index where host name start: `://host:port/
	// Index where `host:port` end, i.e., the index of '/' in the whole url.
	int url_port_end = FirstOccurIndex(url + url_host_begin, '/') + url_host_begin;
	// Host name must end with '/'
	if(url_port_end == url_host_begin -1)
	{
		fprintf(stderr,"\nInvalid URL - host name must ends with '/'.\n");
		exit(BAD_PARAMETER);
	}
	if(CAST(int)strlen(url) > MAX_URL_SIZE) // If url is too long.
	{
		fprintf(stderr,"URL is too long.\n");
		exit(BAD_PARAMETER);
	}

	// Format of a request message:
	//		<method> <request-URL> <version>"\r\n"
	//		<headers>"\r\n"
	//		"\r\n"
	//		<entity-body>

	// Step 2. Initialize(zero) host and request array.
	bzero(host, MAX_HOST_NAME_SIZE);
	bzero(request, MAX_REQUEST_SIZE);

	// Step 3. Fill <method> field.
	switch(method)
	{
	default:
	case METHOD_GET:
		StrCopy(request, &request_index, "GET");
		break;
	case METHOD_HEAD:
		StrCopy(request, &request_index, "HEAD");
		break;
	case METHOD_OPTIONS:
		StrCopy(request, &request_index, "OPTIONS");
		break;
	case METHOD_TRACE:
		StrCopy(request, &request_index, "TRACE");
		break;
	}
	// Add the space between <method> and <request-URL>
	*(request + (request_index++)) = ' ';

	// Step 4. Fill <request-URL> field
	if(proxy_host == NULL) // No proxy, parse host-name and port-number.
	{
		int second_colon = FirstOccurIndex(url + url_host_begin, ':') + url_host_begin;
		int url_host_end = url_port_end; // If port number doesn't exist.
		// If port number exist.
		if(second_colon != url_host_begin -1 && second_colon < url_port_end)
		{
			// Get port number:
			int port = 0;
			for(int index = second_colon + 1; index < url_port_end; ++index)
			{
				port = port * 10 + *(url + index) - '0';
			}
			proxy_port = (port ? port : 80);
			//printf("proxy_port = %d\n", proxy_port);
			url_host_end = second_colon;
		}
		// Get host name.
		int dest_index = 0, src_index = url_host_begin;
		for(; src_index < url_host_end; ++dest_index, ++src_index)
		{
			host[dest_index] = *(url + src_index);
		}
		host[dest_index] = 0; // End host name.
		StrCopy(request, &request_index, url + url_port_end); // Fill <request-URL>
	}
	else
	{
		StrCopy(request, &request_index, url); // Fill <request-URL>
	}

	//  Step 5. Fill <version> and this line's CRLF
	StrCopy(request, &request_index, " HTTP/1.1\r\n");

	//  Step 6. Fill <headers>
	StrCopy(request, &request_index, "User-Agent: WebTest-Xiang Gao\r\n");
	if(proxy_host == NULL) // Without proxy server, fill <Host:> header.
	{
		StrCopy(request, &request_index, "Host: ");
		StrCopy(request, &request_index, host);
		StrCopy(request, &request_index, "\r\n");
	}
	if(force_reload && proxy_host != NULL)
	{
		StrCopy(request, &request_index, "Pragma: no-cache\r\n");
	}
	StrCopy(request, &request_index, "Connection: close\r\n");
	// Step 7. Add an empty line after all headers and add '\0' to end request array.
	StrCopy(request, &request_index, "\r\n\0");
	//printf("%s", request);
}

void PrintConfigure(char **argv) // Step 3. Print this test's configure information.
{
	// Testing: <method> <url> <version>\n
	// With <clients> <time> <force> <proxy> <reload>
	printf("Testing: ");
	switch(method)
	{
	default:
	case METHOD_GET:
		printf("GET");
		break;
	case METHOD_OPTIONS:
		printf("OPTIONS");
		break;
	case METHOD_HEAD:
		printf("HEAD");
		break;
	case METHOD_TRACE:
		printf("TRACE");
		break;
	}
	printf(" %s using HTTP/1.1\n", argv[optind]);
	printf("With %d client(s), running %d second(s)", clients, test_time);
	if(force)
	{
		printf(", early socket close");
	}
	if(proxy_host)
	{
		printf(", via proxy server %s:%d", proxy_host, proxy_port);
		if(force_reload) // Only with proxy server, use reload is useful.
		{
			printf(", forcing reload");
		}
	}
	printf(".\n");
}

void TestServerState() // Step 4. Test whether server is working okay.
{
	// Check whether the server is working by one trying connection.
	int check_socket = TcpConnect(proxy_host ? proxy_host : host, proxy_port);
	if(check_socket == FAIL)
	{
		fprintf(stderr, "Connect to server failed. Abort test.\n");
		exit(SERVER_ERROR);
	}
	close(check_socket);
}

void TestMain() // Step 5. Real test.
{
	// Step 1. Create pipe.
	if(pipe(pc_pipe) == FAIL)
	{
		// When library calls fail, we call perror() to print the
		// message corresponding to current errno.
		perror("pipe failed");
		exit(INTERNAL_ERROR);
	}

	// Step 2. Create children processes.
	pid_t pid = 0;
	for(int cnt = 0; cnt < clients; ++cnt)
	{
		if((pid = fork()) < 0) // fork error, exit.
		{
			perror("fork failed");
			exit(INTERNAL_ERROR);
		}
		else if(pid == 0) // Child
		{
			break; // Don't let child create child process.
		}
	}

	FILE *fp = NULL; // Used by parent and children to read or write, respectively.
	// Step 3. For parent process:
	// 1. Read test data through pipe from children.
	// 2. Calculate and output the result.
	if(pid > 0)
	{
		// Step 1. Read test data through pipe from children.
		close(pc_pipe[1]); // Close write end.
		if((fp = fdopen(pc_pipe[0], "r")) == NULL)
		{
			perror("open pipe for reading failed");
			exit(INTERNAL_ERROR);
		}
		setvbuf(fp, NULL, _IONBF, 0); // Set fp stream unbuffered by _IONBF.
		int child_speed, child_failed, child_bytes;
		for(;;)
		{
			// Should read three values.
			if(fscanf(fp, "%d %d %d", &child_speed, &child_failed, &child_bytes) != 3)
			{
				fprintf(stderr, "fscanf from pipe != 3.\n");
				break;
			}
			speed += child_speed;
			failed += child_failed;
			bytes += child_bytes;

			if(--clients == 0) // Have finished all children's data.
			{
				break;
			}
		}
		fclose(fp);

		// Step 2. Calculate and output the result.
		printf("Speed = %d pages/sec, %d bytes/sec.\n",
		       (speed + failed)/test_time, bytes/test_time);
		printf("Requests: %d success, %d failed.\n", speed, failed);
		exit(SUCCESS);
	}
	/// Step 4. For child process:
	// 1. Setup alarm signal handler.
	// 2. Preform HTTP Transaction: connect-> write request-> read reply->close.
	// 3. Write test data to parent through pipe.
	if(pid == 0)
	{
		// Step 1. Setup alarm signal handler.
		struct sigaction sa;
		sa.sa_handler = alarm_handler;
		sa.sa_flags = 0;
		if(sigaction(SIGALRM, &sa, NULL) == FAIL)
		{
			perror("sigaction(SIGALRM) error");
			exit(INTERNAL_ERROR);
		}
		alarm(CAST(unsigned int)test_time);

		// Step 2. Preform HTTP Transaction: connect-> write request-> read reply->close.
		HttpTransaction(proxy_host ? proxy_host : host, proxy_port, request);

		// Step 3. Write test data to parent through pipe.
		close(pc_pipe[0]); // Close read end of pipe.
		if((fp = fdopen(pc_pipe[1], "w")) == NULL)
		{
			perror("open pipe for writing failed");
			exit(INTERNAL_ERROR);
		}
		fprintf(fp, "%d %d %d\n", speed, failed, bytes);
		fclose(fp);
		exit(SUCCESS);
	}
}

int main(int argc, char **argv)
{
	// On-line support.
	printf("Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest\n");

	// Step 1. Parse arguments.
	ParseArguments(argc, argv);
	// Step 2. Build http request message.
	BuildRequest(argv[optind]);
	// Step 3. Print this test's configure information.
	PrintConfigure(argv);
	// Step 4. Test whether server is working okay.
	TestServerState();
	// Step 5. Real test.
	TestMain();

	return SUCCESS;
}
