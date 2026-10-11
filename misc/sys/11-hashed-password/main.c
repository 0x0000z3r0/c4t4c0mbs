#include <stdio.h>
#include <string.h>

unsigned
hash_password(const char *text)
{
	unsigned hash = 5381;

	while (*text != '\0') {
		hash = (hash * 33) + (unsigned char)*text;
		text++;
	}
	return hash;
}

int
check_password(const char *input)
{
	return hash_password(input) == 0xd2f7c478u;
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
