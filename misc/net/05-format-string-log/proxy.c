#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define MAGIC 0xDEADBEEF
#define MAX_PACKET_SIZE 1024

struct proxy_header {
	uint32_t magic;
	uint32_t session_id;
	uint16_t inner_len;
	uint16_t flags;
};

void
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	fflush(stdout);
	exit(0);
}

void
log_error(const char *msg)
{
	// VULNERABILITY: Format String Bug
	// we pass the user-controlled message directly to printf
	// instead of using printf("%s", msg);
	printf("[-] ERROR: ");
	printf(msg);
	printf("\n");
	fflush(stdout);
}

void
process_packet(char *buffer, int recv_len)
{
	if (recv_len < sizeof(struct proxy_header)) {
		log_error("Packet too small");
		return;
	}

	struct proxy_header *hdr = (struct proxy_header *)buffer;

	if (ntohl(hdr->magic) != MAGIC) {
		log_error("Invalid magic");
		return;
	}

	uint16_t inner_len = ntohs(hdr->inner_len);
	if (inner_len > MAX_PACKET_SIZE - sizeof(struct proxy_header)) {
		log_error("Payload too large");
		return;
	}

	char payload[MAX_PACKET_SIZE];

	// FIX: ensure we don't read past recv_len
	int final_len = inner_len;
	if (recv_len - sizeof(struct proxy_header) < final_len) {
		final_len = recv_len - sizeof(struct proxy_header);
	}

	memcpy(payload, buffer + sizeof(struct proxy_header), final_len);
	payload[final_len] = '\0';  // FIX: null terminate for safety

	// VULNERABILITY: if a specific flag is set, log the payload
	if (ntohs(hdr->flags) == 0xFFFF) {
		// VULNERABILITY: the payload is passed to log_error, which uses it as a format string!
		log_error(payload);
		return;
	}

	printf("[+] Processing packet, session: %u, inner_len: %u\n", ntohl(hdr->session_id), inner_len);
	printf("[+] Payload processed successfully.\n");
}

int
main(void)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Socket creation failed");
		exit(1);
	}

	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		exit(1);
	}

	printf("[*] Proxy listening on UDP port %d...\n", PORT);
	fflush(stdout);

	char buffer[2048];
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	while (1) {
		int bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
		if (bytes > 0) {
			process_packet(buffer, bytes);
		}
	}

	close(sockfd);
	return 0;
}
