#include <stdio.h> // fprintf()
#include <getopt.h> // getopt_long()
#include <stdlib.h> // atoi()
#include <string.h> // strlen()
#include <strings.h> // bzero()

// Define return stats codes
#define SUCCESS 0
//#define BENCHMARK_FAIL 1 // Server is not on-line
#define BAD_PARAMETER 2 // Command-line parameters are bad.
//#define INTERNAL_ERROR 3 // Fork failed

// Support methods: GET, HEAD, OPTIONS, TRACE
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3

// Globals:
int client = 1; // The number of clients, default 1.
int force = 0; // = 1 means Don't wait for reply from server.
char *proxy_host = NULL;  // The proxy host name, default NULL.
int proxy_port = 80; // The proxy port number, default 80(http server).
int force_reload = 0; // = 1 means Send reload request - Pragma: no-cache.
int test_time = 30; // Time for testing, default 30.
int method = -1; // Accept method applied by user.

// Store host name.
#define MAX_HOST_NAME_SIZE 64
char host[MAX_HOST_NAME_SIZE];
int host_index = 0; // Cursor to fill host array.
// Store request contents.
#define MAX_REQUEST_SIZE 2048
char request[MAX_REQUEST_SIZE];
int request_index = 0; // Cursor to fill request array.
#define MAX_URL_SIZE 1500 // Max length of legal url

static const struct option long_options[] =
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

static void Usage() // Print usage guide for user
{
	// Don't use TAB since it is different width on different platform.
	// Only use space to align.
	fprintf(stderr,
	        "webtest [option] URL\n"
	        "  -c|--clients <n>         Run <n> HTTP clients at once. Default 1.\n"
	        "  -f|--force               Don't wait for reply from server.\n"
	        "  -h|--help             This information.\n"
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
	int length = strlen(src), index = 0;
	while(index < length)
	{
		if(*(src + index) == *substr) // Match the begin character, continue match.
		{
			for(const char *find = src + index; *substr && *find; ++substr, ++find)
			{
				if(*substr != *find) // Not match, return -1.
				{
					return -1;
				}
			}
			return index; // Match success.
		}
	}
	return -1; // Not match.
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
				return -1; // Not match.
			}
		}
		else if(*large != *small) // *small is not letter, so must match exactly.
		{
			return -1;
		}
	}
	return 0; // All match.
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
	return -1;
}

void BuildRequest(const char *url)
{
	printf("URL = %s\n", url);
	/*
	* Format of a request message:
	* <method> <request-URL> <version>
	* <headers>
	* "\r\n"
	* <entity-body> // We don't need entity-body.
	*/

	// Initialization: zero our host and request array
	bzero(host, MAX_HOST_NAME_SIZE);
	bzero(request, MAX_REQUEST_SIZE);

	printf("1. Fill <method> field.\n");
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
	*(request + request_index) = 0;
	printf("request = \"%s", request);

	int first_colon = FindFirstSubstring(url, "://"); // The index of the first ':' appears in url.
	// Check whether url is legal, URL syntax: `<scheme>://<host>:<port>/`
	if(first_colon == -1) // If "://" can't be found in url.
	{
		fprintf(stderr, "\n%s: is not a valid URL.\n", url);
		exit(BAD_PARAMETER);
	}
	if(strlen(url) > MAX_URL_SIZE) // If url is too long.
	{
		fprintf(stderr,"URL is too long.\n");
		exit(BAD_PARAMETER);
	}
	if(proxy_host == NULL) // Check <scheme> supplied by user.
	{
		// Scheme names are case-insensitive.
		if(IgnoreCaseMatch(url, "http://") != 0)
		{
			fprintf(stderr, "\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
			exit(BAD_PARAMETER);
		}
	}
/*
	// 2. Fill <request-URL> field
	int url_host_begin = first_colon + 3; // Index where host name start: `://host:port/
	// Index where `host:port` end, i.e., the index of '/'
	int url_port_end = FirstOccurIndex(url + url_host_begin, '/');
	// Host name must end with '/'
	if(url_port_end == -1)
	{
		fprintf(stderr,"\nInvalid URL - host name must ends with '/'.\n");
		exit(BAD_PARAMETER);
	}
	if(proxy_host == NULL) // No proxy, parse host-name and port-number.
	{
		int second_colon = FirstOccurIndex(url + url_host_begin, ':');
		int url_host_end = url_port_end; // If port number doesn't exist.
		// If port number exist.
		if(second_colon != -1 && second_colon < url_port_end)
		{
			// Get port number:
			int port = 0;
			for(int index = second_colon + 1; index < url_port_end; ++index)
			{
				port = port * 10 + *(url + index) - '0';
			}
			proxy_port = (port ? port : 80);
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

	// 3. Fill <version> and this line's CRLF
	StrCopy(request, &request_index, " HTTP/1.1\r\n");

	// 4. Fill <headers>
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
	// Add an empty line after all headers and add '\0' to end request array.
	StrCopy(request, &request_index, "\r\n\0");*/
}

int main(int argc, char **argv)
{
	// Check whether command-line-arguments are right.
	if(argc == 1)
	{
		printf("Should print Usage(), Omit.\n");
		//Usage(); // Tell user how to use it.
		return BAD_PARAMETER;
	}

	// Parse command-line options
	int option = 0; // Store return value from getopt_long()
	int options_index = 0; // Index of which argv-element to parse, begin with 0
	// If all command-line options have been parsed, returns -1.
	while((option = getopt_long(argc, argv, "c:fhp:rt:v019", long_options, &options_index)) != -1)
	{
		switch(option)
		{
		case 'c':
			client = atoi(optarg);
			printf("option -c|--clients. client number = %d\n", client);
			break;
		case 'f':
			force = 1;
			printf("option -f|--force. force = %d\n", force);
			break;
		case 'h':
			printf("option -h|--help. Should print Usage.\n");
			//Usage();
			//return BAD_PARAMETER;
			break;
		case 'p':
			printf("option -p|--proxy <server:port> | optarg = %s | ", optarg);
			// proxy server parsing "server:port"
			// Returns a pointer to the last occurrence of ':' in optarg
			int colon_index = FirstOccurIndex(optarg, ':'); // webbench use `strrchr(optarg, ':');`
			proxy_host = optarg;
			if(colon_index == -1) // ':' not find, parse next argument.
			{
				break;
			}
			if(colon_index == 0) // Input error: ":7188", missing host name.
			{
				fprintf(stderr, "Error in option --proxy %s: Missing host name.\n", optarg);
				return BAD_PARAMETER;
			}
			if(colon_index == strlen(optarg) - 1) // Input error: "name:", missing port.
			{
				fprintf(stderr, "Error in option --proxy %s Port number is missing.\n", optarg);
				return BAD_PARAMETER;
			}
			*(optarg + colon_index) = 0; // Delimit host_name ans port_number
			proxy_port = atoi(optarg + colon_index + 1);
			printf("host = %s | port = %d\n", proxy_host, proxy_port);
			break;
		case 'r':
			force_reload = 1;
			printf("option -r|--reload. force_reload = %d\n", force_reload);
			break;
		case 't':
			test_time = atoi(optarg);
			printf("option -t|--time <sec>. test_time = %d\n", test_time);
			break;
		case 0: // For long option, return 0 if flag != NULL
			printf("option --%s", long_options[options_index].name);
			if(optarg)
			{
				printf(" with arg = %s", optarg);
			}
			printf("\n");
			break;
		}
	}

	if(optind == argc)
	{
		fprintf(stderr,"webtest: Missing URL!\n");
		//Usage();
		return BAD_PARAMETER;
	}

	// Deal with wrong arguments.
	client = (client == 0 ? 1 : client);
	test_time = (test_time == 0 ? 30 : test_time);

	// Online support.
	printf("Visit webtest online: https://github.com/gaoxiangnumber1/webtest\n");

	BuildRequest(argv[optind]);

	return SUCCESS;
}
