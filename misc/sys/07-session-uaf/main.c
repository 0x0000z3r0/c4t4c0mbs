#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct session {
	void (*greet_func)();
	char username[24];
};

void
normal_greeting()
{
	printf("Hello there! Welcome to the service.\n");
}

void
admin_backdoor()
{
	printf("[!] Admin backdoor triggered!\n");
	system("/bin/sh");
}

int
main()
{
	printf("--- Heap Session Manager ---\n");
	printf("Admin backdoor is at: %p\n", admin_backdoor);

	// 1. Allocate a session
	struct session *sess = malloc(sizeof(struct session));
	sess->greet_func = normal_greeting;
	strcpy(sess->username, "guest");

	printf("Session allocated at %p\n", sess);

	// 2. Free the session (but we forget to set sess = NULL)
	printf("Logging out and freeing session...\n");
	free(sess);

	// 3. User provides some profile data.
	// malloc() will likely reuse the recently freed chunk!
	printf("Enter new profile data: ");
	fflush(stdout);

	char *profile = malloc(32);  // Same size as struct session
	// We read up to 32 bytes, which can overwrite the greet_func pointer
	// if this chunk reuses the freed session chunk.
	read(0, profile, 32);

	// 4. USE AFTER FREE
	// The program mistakenly uses the old session pointer
	printf("Triggering session greeting...\n");
	sess->greet_func();

	return 0;
}
