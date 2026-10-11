#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define MAGIC 0xDEADBEEF
#define MAX_PAYLOAD 256

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
}

void
process_packet(char *buffer, int recv_len)
{
	if (recv_len < sizeof(struct proxy_header)) {
		printf("[-] Packet too small\n");
		return;
	}

	struct proxy_header *hdr = (struct proxy_header *)buffer;
	if (ntohl(hdr->magic) != MAGIC) {
		printf("[-] Invalid magic\n");
		return;
	}

	// VULNERABILITY: signedness bypass + unsigned comparison due to sizeof
	int16_t inner_len = (int16_t)ntohs(hdr->inner_len);
	if (inner_len > MAX_PAYLOAD) {
		printf("[-] Payload too large! Dropping packet.\n");
		return;
	}

	char payload[MAX_PAYLOAD];
	int16_t final_len = inner_len;

	// FIX: developer tries to be safe by not copying more than what was received.
	// However, sizeof(struct proxy_header) is size_t (unsigned).
	// This forces the comparison to be unsigned.
	// If final_len is -1, it is cast to a huge unsigned number, so the condition is true.
	if (recv_len - sizeof(struct proxy_header) < final_len) {
		final_len = recv_len - sizeof(struct proxy_header);
	}

	printf("[+] Processing packet, session: %u, inner_len: %d, copying: %d bytes\n",
	       ntohl(hdr->session_id), inner_len, final_len);
	fflush(stdout);

	memcpy(payload, buffer + sizeof(struct proxy_header), final_len);

	printf("[+] Payload copied successfully.\n");
	fflush(stdout);
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
