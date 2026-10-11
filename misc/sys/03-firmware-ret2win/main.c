#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
admin_debug_shell()
{
	printf("[!] WARNING: Entering undocumented debug mode.\n");
	system("/bin/sh");
	exit(0);
}

void
process_firmware_update()
{
	char update_data[64];
	printf("Ready for firmware update payload.\n");
	printf("Send data: ");
	fflush(stdout);

	// VULNERABILITY: gets() does not check bounds!
	// A malicious update can overflow the stack and overwrite the return address.
	gets(update_data);

	printf("Processing update: %s\n", update_data);
}

int
main()
{
	printf("--- IoT Device Firmware Updater v1.0 ---\n");
	process_firmware_update();
	printf("Update complete. Rebooting...\n");
	return 0;
}
