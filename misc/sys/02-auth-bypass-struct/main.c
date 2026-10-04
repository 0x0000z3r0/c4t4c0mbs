#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct auth_context {
	char username[32];
	int is_admin;
};

void
authenticate_user()
{
	struct auth_context ctx;
	ctx.is_admin = 0;  // Default to standard user

	printf("Enter username: ");
	fflush(stdout);

	// VULNERABILITY: gets() doesn't check bounds.
	// Writing more than 32 bytes will overflow into ctx.is_admin
	gets(ctx.username);

	printf("Welcome, %s!\n", ctx.username);

	if (ctx.is_admin) {
		printf("[+] ACCESS GRANTED: Admin privileges active.\n");
	} else {
		printf("[-] ACCESS DENIED: Standard user.\n");
	}
}

int
main()
{
	printf("--- Legacy Auth Service ---\n");
	authenticate_user();
	return 0;
}
