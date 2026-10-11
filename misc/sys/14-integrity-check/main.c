#include <stdio.h>
#include <string.h>

unsigned expected_checksum = 0xc0ffee42u;

int
check_password(const char *input)
{
	if (strcmp(input, "firmware-admin") == 0) {
		return 1;
	} else {
		return 0;
	}
}

unsigned
code_checksum(void)
{
	unsigned char *code = (unsigned char *)check_password;
	unsigned sum = 2166136261u;
	int i;

	for (i = 0; i < 64; i++) {
		sum ^= code[i];
		sum *= 16777619u;
	}
	return sum;
}

int
main(void)
{
	char input[64];

	printf("--- Service console ---\n");
	if (code_checksum() != expected_checksum) {
		printf("Tamper detected.\n");
		return 1;
	}

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
