#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define MAGIC 0xDEADBEEF
#define MAX_PACKET_SIZE 1024

// Protocol Flags
#define FLAG_INIT 0x0001
#define FLAG_AUTH 0x0002
#define FLAG_DATA 0x0004

// Session States
#define STATE_NONE 0
#define STATE_INIT 1
#define STATE_AUTHED 2

struct proxy_header {
	uint32_t magic;
	uint32_t session_id;
	uint16_t inner_len;
	uint16_t flags;
};

struct session {
	uint32_t id;
	int state;
};

struct session current_session = {0, STATE_NONE};

void
log_error(const char *msg)
{
	// SECURE: use format string specifier
	printf("[-] ERROR: %s\n", msg);
}

void
process_packet(char *buffer, int recv_len)
{
	// SECURE: ensure we received at least a full header
	if (recv_len < sizeof(struct proxy_header)) {
		log_error("Packet too small");
		return;
	}

	struct proxy_header *hdr = (struct proxy_header *)buffer;

	// SECURE: validate magic bytes
	if (ntohl(hdr->magic) != MAGIC) {
		log_error("Invalid magic");
		return;
	}

	uint32_t session_id = ntohl(hdr->session_id);
	uint16_t flags = ntohs(hdr->flags);

	// SECURE: use unsigned types for lengths
	uint16_t inner_len = ntohs(hdr->inner_len);

	// SECURE: check if inner_len exceeds the maximum allowed payload size
	if (inner_len > MAX_PACKET_SIZE - sizeof(struct proxy_header)) {
		log_error("Payload too large");
		return;
	}

	// SECURE: verify that the actual received data is at least as large as claimed
	// This prevents Out-of-Bounds reads (Heartbleed style)
	if (recv_len - sizeof(struct proxy_header) < inner_len) {
		log_error("Claimed length exceeds actual received data");
		return;
	}

	char payload_buffer[MAX_PACKET_SIZE];

	// SECURE: safe copy using the validated inner_len
	memcpy(payload_buffer, buffer + sizeof(struct proxy_header), inner_len);
	payload_buffer[inner_len] = '\0';  // Null terminate for string operations

	if (flags & FLAG_INIT) {
		printf("[+] Initializing new session: %u\n", session_id);
		current_session.id = session_id;
		current_session.state = STATE_INIT;
		return;
	}

	if (flags & FLAG_AUTH) {
		// SECURE: ensure session ID matches
		if (current_session.id != session_id) {
			log_error("Auth failed: Invalid session ID");
			return;
		}

		if (inner_len >= 8 && strncmp(payload_buffer, "secret12", 8) == 0) {
			printf("[+] Session %u authenticated successfully!\n", session_id);
			current_session.state = STATE_AUTHED;
		} else {
			log_error("Auth failed: Incorrect password");
			current_session.state = STATE_NONE;
		}
		return;
	}

	if (flags & FLAG_DATA) {
		// SECURE: strict state enforcement AND session ID validation
		if (current_session.state != STATE_AUTHED) {
			log_error("Data rejected: Session not authenticated");
			return;
		}

		if (current_session.id != session_id) {
			log_error("Data rejected: Session ID mismatch");
			return;
		}

		printf("[+] Processing data for session %u...\n", session_id);
		printf("[+] Data: %s\n", payload_buffer);
	}
}

int
main()
{
	int sockfd;
	struct sockaddr_in server_addr, client_addr;
	char buffer[2048];
	socklen_t client_len = sizeof(client_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Socket creation failed");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		exit(1);
	}

	printf("[*] Secure Proxy listening on UDP port %d...\n", PORT);

	while (1) {
		int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
		if (n > 0) {
			process_packet(buffer, n);
		}
	}

	close(sockfd);
	return 0;
}
