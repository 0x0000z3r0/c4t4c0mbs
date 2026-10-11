#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define MAGIC 0xDEADBEEF
#define MAX_PACKET_SIZE 256

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

	// FIX: we fixed signedness by using uint16_t.
	uint16_t inner_len = ntohs(hdr->inner_len);

	// FIX: we want to ensure the total packet size doesn't exceed MAX_PACKET_SIZE
	uint16_t total_len = inner_len + sizeof(struct proxy_header);
	if (total_len > MAX_PACKET_SIZE) {
		printf("[-] Packet too large! Dropping packet. total_len: %u\n", total_len);
		return;
	}

	// FIX: the payload buffer is sized based on MAX_PACKET_SIZE - header size
	char payload[MAX_PACKET_SIZE - sizeof(struct proxy_header)];
	printf("[+] Processing packet, session: %u, inner_len: %u, total_len: %u\n",
	       ntohl(hdr->session_id), inner_len, total_len);
	fflush(stdout);

	// VULNERABILITY: memcpy uses inner_len, which can be huge if total_len overflowed.
	// To prevent a crash before returning, we bounds it by recv_len.
	// However, recv_len can be up to 2048, which is still much larger than the 244-byte buffer
	uint16_t final_len = inner_len;
	if (recv_len - sizeof(struct proxy_header) < final_len) {
		final_len = recv_len - sizeof(struct proxy_header);
	}

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
	char buffer[2048];
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
