#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
process_firmware_update()
{
	char update_data[64];
	printf("Ready for firmware update payload.\n");
	printf("Send data: ");
	fflush(stdout);

	// VULNERABILITY: gets() does not check bounds!
	gets(update_data);

	printf("Processing update: %s\n", update_data);
}

int
main()
{
	printf("--- IoT Device Firmware Updater v1.0 (Mitigations Test) ---\n");
	process_firmware_update();
	return 0;
}
