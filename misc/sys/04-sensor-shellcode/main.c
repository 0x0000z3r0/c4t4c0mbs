#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
parse_sensor_packet()
{
	char packet_buffer[128];

	printf("Packet buffer allocated at: %p\n", packet_buffer);
	printf("Awaiting raw sensor data: ");
	fflush(stdout);

	// VULNERABILITY: gets() does not check bounds!
	// We compile this with an executable stack (-z execstack) to simulate older systems.
	gets(packet_buffer);

	printf("Packet received. Length: %lu\n", strlen(packet_buffer));
}

int
main()
{
	printf("--- Legacy Sensor Data Aggregator ---\n");
	parse_sensor_packet();
	return 0;
}
