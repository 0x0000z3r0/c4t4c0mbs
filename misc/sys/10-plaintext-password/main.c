#include <stdio.h>
#include <string.h>

int
check_password(const char *input)
{
	return strcmp(input, "firmware-admin") == 0;
}

int
main(void)
{
	char input[64];

	printf("--- Service console ---\n");
	printf("Password: ");
	fflush(stdout);
	if (fgets(input, sizeof(input), stdin) == NULL) {
		return 1;
	}
	input[strcspn(input, "\n")] = '\0';

	if (check_password(input)) {
		printf("Access granted.\n");
		return 0;
	}
	printf("Access denied.\n");
	return 1;
}
