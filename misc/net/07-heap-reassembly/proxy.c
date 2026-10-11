#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define MAGIC 0xDEADBEEF

#define FLAG_FRAG 0x0001
#define FLAG_LAST 0x0002

struct proxy_header {
	uint32_t magic;
	uint32_t session_id;
	uint16_t inner_len;
	uint16_t flags;
	uint16_t frag_offset;
};

struct reassembly_buffer {
	char *data;
	int total_size;
	int current_size;
};

struct handler {
	void (*process)(char *);
};

struct reassembly_buffer *active_buffer = NULL;
struct handler *current_handler = NULL;

void
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	fflush(stdout);
	exit(0);
}

void
default_process(char *data)
{
	printf("[+] Processing reassembled data of length %lu\n", strlen(data));
}

void
process_packet(char *buffer, int recv_len)
{
	if (recv_len < sizeof(struct proxy_header)) {
		return;
	}

	struct proxy_header *hdr = (struct proxy_header *)buffer;
	if (ntohl(hdr->magic) != MAGIC) {
		return;
	}

	uint16_t flags = ntohs(hdr->flags);
	uint16_t inner_len = ntohs(hdr->inner_len);
	uint16_t frag_offset = ntohs(hdr->frag_offset);
	char *payload = buffer + sizeof(struct proxy_header);

	if (flags & FLAG_FRAG) {
		if (active_buffer == NULL) {
			printf("[+] Starting new reassembly buffer...\n");
			active_buffer = malloc(sizeof(struct reassembly_buffer));
			active_buffer->total_size = 256;
			active_buffer->data = malloc(active_buffer->total_size);
			active_buffer->current_size = 0;

			current_handler = malloc(sizeof(struct handler));
			current_handler->process = default_process;
		}

		// VULNERABILITY: Heap Buffer Overflow during reassembly
		// we check if current_size + inner_len > total_size,
		// BUT we use frag_offset to write data. We don't check if
		// frag_offset + inner_len > total_size.

		if (active_buffer->current_size + inner_len > active_buffer->total_size) {
			printf("[-] Reassembly buffer full! Dropping fragment.\n");
			return;
		}

		printf("[*] Received fragment: offset %u, length %u\n", frag_offset, inner_len);

		// VULNERABILITY: Write fragment to the specified offset.
		// If frag_offset is large, this writes past the end of active_buffer->data,
		// potentially overwriting current_handler
		// active_buffer->data is a pointer. We are writing to the memory it points to.
		// So we are overwriting the heap chunk AFTER active_buffer->data.
		// What is allocated after active_buffer->data:
		//   - active_buffer = malloc(sizeof(struct reassembly_buffer)) (24 bytes -> 32 byte chunk)
		//   - active_buffer->data = malloc(256) (256 bytes -> 272 byte chunk)
		//   - current_handler = malloc(sizeof(struct handler)) (8 bytes -> 32 byte chunk)
		// So the layout is:
		// [active_buffer chunk]
		// [data chunk]
		// [current_handler chunk]
		// We are writing to (data + frag_offset).
		// If frag_offset = 272, we write exactly into the user data of the current_handler chunk.
		memcpy(active_buffer->data + frag_offset, payload, inner_len);
		active_buffer->current_size += inner_len;

		if (flags & FLAG_LAST) {
			printf("[+] Reassembly complete! Calling handler...\n");

			// FIX: null terminate for safety
			if (active_buffer->current_size < active_buffer->total_size) {
				active_buffer->data[active_buffer->current_size] = '\0';
			}

			if (current_handler && current_handler->process) {
				current_handler->process(active_buffer->data);
			}

			free(active_buffer->data);
			free(active_buffer);
			free(current_handler);
			active_buffer = NULL;
			current_handler = NULL;
		}
	}
}

int
main(void)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	
	bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	printf("[*] Proxy listening on UDP port %d...\n", PORT);
	fflush(stdout);
	
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	char buffer[2048];
	while (1) {
		int bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
		if (bytes > 0) {
			process_packet(buffer, bytes);
		}
	}

	close(sockfd);
	return 0;
}
