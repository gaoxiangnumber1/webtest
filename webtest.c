#include <stdio.h> // fprintf()
#include <getopt.h>

/* Define return state codes
 *    0 - sucess
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad parameter
 *    3 - internal error, fork failed*/
#define	SUCCESS					0
#define	BADPARAMETER	2

void Usage() // Print usage guide for user
{
	fprintf(stderr,
	        "webtest [option] URL\n"
	        "  -f|--force               Don't wait for reply from server.\n"
	        "  -r|--reload              Send reload request - Pragma: no-cache.\n"
	        "  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
	        "  -p|--proxy <server:port> Use proxy server for request.\n"
	        "  -c|--clients <n>         Run <n> HTTP clients at once. Default 1.\n"
	        "  -9|--http09              Use HTTP/0.9 style requests.\n"
	        "  -1|--http10              Use HTTP/1.0 protocol.\n"
	        "  -2|--http11              Use HTTP/1.1 protocol.\n"
	        "  --get                    Use GET request method.\n"
	        "  --head                   Use HEAD request method.\n"
	        "  --options                Use OPTIONS request method.\n"
	        "  --trace                  Use TRACE request method.\n"
	        "  -?|-h|--help             This information.\n"
	        "  -V|--version             Display program version.\n"
	       );
}

int main(int argc, char **argv)
{
	// Check whether command-line-arguments are right.
	if(argc == 1)
	{
		Usage(); // Tell user how to use it.
		return BADPARAMETER;
	}
	getopt_long(argc, argv, NULL, NULL, NULL);
}
