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

	// SECURE: fgets() checks bounds and prevents overflow
	if (fgets(update_data, sizeof(update_data), stdin) != NULL) {
		update_data[strcspn(update_data, "\n")] = '\0';
		printf("Processing update: %s\n", update_data);
	} else {
		printf("Error reading input.\n");
	}
}

int
main()
{
	printf("--- IoT Device Firmware Updater v2.0 (SECURE) ---\n");
	process_firmware_update();
	return 0;
}
