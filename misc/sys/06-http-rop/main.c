#include <stdio.h>

void
handle_http_request()
{
	char request_uri[64];
	printf("Waiting for HTTP request...\n");
	printf("GET ");
	fflush(stdout);

	// VULNERABILITY: gets() does not check bounds
	gets(request_uri);

	printf("HTTP/1.1 404 Not Found\n");
}

int
main()
{
	printf("--- Simple Web Server v0.1 ---\n");
	handle_http_request();
	return 0;
}
