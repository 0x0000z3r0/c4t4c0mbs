#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>

int
tracer_attached(void)
{
	char line[256];
	int tracer = 0;
	FILE *status = fopen("/proc/self/status", "r");

	if (status == NULL) {
		return 0;
	}
	while (fgets(line, sizeof(line), status) != NULL) {
		if (strncmp(line, "TracerPid:", 10) == 0) {
			sscanf(line, "TracerPid:%d", &tracer);
			break;
		}
	}
	fclose(status);
	if (tracer != 0) {
		return 1;
	} else {
		return 0;
	}
}

int
ptrace_blocked(void)
{
	/* A debugger that is already attached makes this call fail. */
	if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
		return 1;
	}
	return 0;
}

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
	if (tracer_attached()) {
		printf("Debugger detected (TracerPid).\n");
		return 1;
	}
	if (ptrace_blocked()) {
		printf("Debugger detected (ptrace).\n");
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
