#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int debug_mode = 0;

void
process_log_entry()
{
	char log_msg[256];
	printf("Enter log message: ");
	fflush(stdout);

	if (fgets(log_msg, sizeof(log_msg), stdin) == NULL) {
		return;
	}

	// VULNERABILITY: printf() is called directly with user input!
	// A malicious user can supply format specifiers like %x, %p, or %n.
	printf("Logged: ");
	printf(log_msg);

	if (debug_mode != 0) {
		printf("\n[!] DEBUG MODE ACTIVATED. Spawning shell...\n");
		system("/bin/sh");
	}
}

int
main()
{
	printf("--- Centralized Log Service v2.1 ---\n");
	printf("Target variable 'debug_mode' is at: %p\n", &debug_mode);
	process_log_entry();
	return 0;
}
