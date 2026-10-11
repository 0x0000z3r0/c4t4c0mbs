#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define MAGIC 0xDEADBEEF

#define FLAG_INIT 0x0001
#define FLAG_AUTH 0x0002
#define FLAG_DATA 0x0004

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
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	fflush(stdout);
	exit(0);
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

	uint32_t session_id = ntohl(hdr->session_id);
	uint16_t flags = ntohs(hdr->flags);
	uint16_t inner_len = ntohs(hdr->inner_len);

	printf("[*] Received packet => Session: %u, Flags: 0x%04X\n", session_id, flags);

	if (flags & FLAG_INIT) {
		printf("[+] Initializing new session: %u\n", session_id);
		current_session.id = session_id;
		current_session.state = STATE_INIT;
		return;
	}

	if (flags & FLAG_AUTH) {
		if (current_session.id != session_id) {
			printf("[-] Auth failed: Invalid session ID\n");
			return;
		}

		// SIMULATION: simulate checking a password in the payload
		char *payload = buffer + sizeof(struct proxy_header);
		if (inner_len >= 8 && strncmp(payload, "secret12", 8) == 0) {
			printf("[+] Session %u authenticated successfully!\n", session_id);
			current_session.state = STATE_AUTHED;
		} else {
			printf("[-] Auth failed: Incorrect password\n");
			current_session.state = STATE_NONE;
		}
		return;
	}

	if (flags & FLAG_DATA) {
		// VULNERABILITY: State Machine Confusion
		// we check if the session is authenticated, BUT we check
		// the state of `current_session` without verifying if the packet's `session_id`
		// matches `current_session.id`

		if (current_session.state != STATE_AUTHED) {
			printf("[-] Data rejected: Session not authenticated.\n");
			return;
		}

		// FIX: if (current_session.id != session_id) { drop_packet(); }

		printf("[+] Processing data for session %u...\n", session_id);

		char *payload = buffer + sizeof(struct proxy_header);
		if (inner_len >= 4 && strncmp(payload, "ROOT", 4) == 0) {
			printf("[!] Root command received!\n");
			unlock_firmware();
		}
	}
}

int
main()
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

	// SIMULATION: simulate an already authenticated session from a legitimate user
	current_session.id = 9999;
	current_session.state = STATE_AUTHED;
	printf("[*] Simulated legitimate user session (ID: 9999) is currently AUTHED.\n");
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
