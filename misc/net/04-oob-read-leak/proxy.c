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
authenticate_admin(void)
{
	// SIMULATION: we use a large buffer here so it overlaps with where process_packet will allocate its payload buffer
	char stack_frame[1024];
	
	// SIMULATION: fill it with junk so we can see it in the dump easily
	memset(stack_frame, 'C', sizeof(stack_frame));
	
	strcpy(stack_frame + 128, "SECRET_SESSION_TOKEN_9988776655");
	strcpy(stack_frame + 256, "ADMIN_API_KEY=1337_h4x0r_k3y");
	
	// SIMULATION: we must do something with the buffer so the compiler doesn't optimize it away entirely
	// A simple printf to /dev/null or similar would work, but this is cleaner
	if (stack_frame[0] == 'X') {
		printf("Never happens, %s\n", stack_frame);
	}
}

void
process_packet(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer, int recv_len)
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

	// FIX: secure integer math
	uint16_t inner_len = ntohs(hdr->inner_len);
	if (inner_len > MAX_PACKET_SIZE - sizeof(struct proxy_header)) {
		printf("[-] Payload too large! Dropping packet.\n");
		return;
	}

	// VULNERABILITY: OOB Read (Heartbleed style)
	// We allocate a buffer on the stack and leave it uninitialized.
	// We copy the payload from the received buffer, but we trust inner_len
	// WITHOUT checking if recv_len actually contains that much data

	char payload[MAX_PACKET_SIZE];

	printf("[+] Processing packet, session: %u, inner_len: %u\n", ntohl(hdr->session_id), inner_len);

	// FIX: copy data from the received buffer, if inner_len > recv_len, it reads past the end of the received data
	// into uninitialized stack memory (which might contain the secret_token).
	// We only copy what we actually received to avoid segfaults, but we still send back inner_len
	int copy_len = recv_len - sizeof(struct proxy_header);
	if (copy_len > 0) {
		memcpy(payload, buffer + sizeof(struct proxy_header), copy_len);
	}

	// VULNERABILITY: echo the packet back to the sender (e.g., an ACK or response)
	// it sends back inner_len bytes from the payload.
	sendto(sockfd, payload, inner_len, 0, (struct sockaddr *)client_addr, client_len);

	printf("[+] Response sent.\n");
}

void
handle_client(int sockfd)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	// SIMULATION: simulate an admin authenticating right before our malicious packet arrives
	authenticate_admin();

	char buffer[2048];
	int bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
	if (bytes > 0) {
		process_packet(sockfd, &client_addr, client_len, buffer, bytes);
	}
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
	while (1) {
		handle_client(sockfd);
	}

	close(sockfd);
	return 0;
}
