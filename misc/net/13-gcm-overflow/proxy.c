#include <arpa/inet.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAGIC 0xDEADBEEF
#define KEY_LEN 16
#define NONCE_LEN 12
#define TAG_LEN 16
#define MAX_BODY 1024

struct secure_header {
	uint32_t magic;
	uint32_t session_id;
	uint32_t seq;
	uint16_t flags;
	uint16_t inner_len;
	uint8_t nonce[NONCE_LEN];
};

static const uint8_t shared_key[KEY_LEN] = "lab-proxy-key!!!";
static uint32_t next_seq = 1;
static uint32_t last_seq = 0;

static int
seal_payload(const uint8_t *nonce, const uint8_t *aad, int aad_len, const uint8_t *plain, int len, uint8_t *cipher, uint8_t *tag)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		return -1;
	}

	int status = EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
	if (status == 1) {
		status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_LEN, NULL);
	}
	if (status == 1) {
		status = EVP_EncryptInit_ex(ctx, NULL, NULL, shared_key, nonce);
	}
	int out_len = 0;
	if (status == 1) {
		status = EVP_EncryptUpdate(ctx, NULL, &out_len, aad, aad_len);
	}
	if (status == 1) {
		status = EVP_EncryptUpdate(ctx, cipher, &out_len, plain, len);
	}
	int final_len = 0;
	if (status == 1) {
		status = EVP_EncryptFinal_ex(ctx, cipher + out_len, &final_len);
	}
	if (status == 1) {
		status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);
	}
	EVP_CIPHER_CTX_free(ctx);
	if (status != 1) {
		return -1;
	}
	return out_len + final_len;
}

static int
open_payload(const uint8_t *nonce, const uint8_t *aad, int aad_len, const uint8_t *cipher, int len, const uint8_t *tag, uint8_t *plain)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		return -1;
	}

	int status = EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
	if (status == 1) {
		status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_LEN, NULL);
	}
	if (status == 1) {
		status = EVP_DecryptInit_ex(ctx, NULL, NULL, shared_key, nonce);
	}
	int out_len = 0;
	if (status == 1) {
		status = EVP_DecryptUpdate(ctx, NULL, &out_len, aad, aad_len);
	}
	if (status == 1) {
		status = EVP_DecryptUpdate(ctx, plain, &out_len, cipher, len);
	}
	if (status == 1) {
		status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, (void *)tag);
	}
	int final_len = 0;
	if (status == 1) {
		status = EVP_DecryptFinal_ex(ctx, plain + out_len, &final_len);
	}
	EVP_CIPHER_CTX_free(ctx);
	if (status != 1) {
		return -1;
	}
	return out_len + final_len;
}

static void
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	fflush(stdout);
	exit(0);
}

static void
apply_frame(const struct secure_header *header, const uint8_t *plain, int body_len)
{
	uint16_t flags = ntohs(header->flags);
	printf("[*] command (%d bytes) flags: %u seq: %u\n", body_len, flags, ntohl(header->seq));
	fflush(stdout);

	char command[64];
	// VULNERABILITY: AES-GCM already accepted this plaintext. body_len is
	// authentic, and it is still used as the copy length into a 64-byte
	// stack buffer, so a long authentic command overwrites the return address.
	memcpy(command, plain, (size_t)body_len);
	// FIX: reject a command that does not fit, before the copy.
	// if (body_len >= (int)sizeof(command)) {
	// 	printf("[-] command too long\n");
	// 	return;
	// }
	// memcpy(command, plain, (size_t)body_len);

	if (body_len == 6 && memcmp(command, "UNLOCK", 6) == 0) {
		unlock_firmware();
	} else if (body_len < (int)sizeof(command)) {
		printf("[+] command ignored\n");
	}
	fflush(stdout);
}

static int
unwrap_datagram(const uint8_t *frame, int frame_len, uint8_t *plain_out)
{
	if (frame_len < (int)sizeof(struct secure_header)) {
		printf("[-] frame too small\n");
		return -1;
	}
	struct secure_header header;
	memcpy(&header, frame, sizeof(header));
	if (ntohl(header.magic) != MAGIC) {
		printf("[-] invalid magic\n");
		return -1;
	}
	int body_len = ntohs(header.inner_len);
	if (body_len <= 0 || body_len > MAX_BODY) {
		printf("[-] bad length\n");
		return -1;
	}
	if (frame_len < (int)sizeof(header) + body_len + TAG_LEN) {
		printf("[-] truncated frame\n");
		return -1;
	}
	const uint8_t *tag = frame + sizeof(header) + body_len;
	if (open_payload(header.nonce, frame, (int)sizeof(header), frame + sizeof(header), body_len, tag, plain_out) != body_len) {
		printf("[-] bad tag\n");
		return -1;
	}
	uint32_t seq = ntohl(header.seq);
	if (seq <= last_seq) {
		printf("[-] replayed seq %u\n", seq);
		return -1;
	}
	last_seq = seq;
	apply_frame(&header, plain_out, body_len);
	return body_len;
}

static void
wrap_datagram(const uint8_t *packet, int packet_len, int hop_fd, const struct sockaddr_in *hop)
{
	if (packet_len <= 0 || packet_len > MAX_BODY) {
		printf("[-] bad cleartext length\n");
		return;
	}

	struct secure_header header;
	memset(&header, 0, sizeof(header));
	header.magic = htonl(MAGIC);
	uint32_t seq = next_seq++;
	header.seq = htonl(seq);
	header.inner_len = htons((uint16_t)packet_len);
	uint32_t nonce_seq = htonl(seq);
	memcpy(header.nonce + (NONCE_LEN - sizeof(nonce_seq)), &nonce_seq, sizeof(nonce_seq));

	uint8_t cipher[MAX_BODY];
	uint8_t tag[TAG_LEN];
	if (seal_payload(header.nonce, (const uint8_t *)&header, (int)sizeof(header), packet, packet_len, cipher, tag) != packet_len) {
		printf("[-] encrypt failed\n");
		return;
	}

	uint8_t frame[sizeof(header) + MAX_BODY + TAG_LEN];
	memcpy(frame, &header, sizeof(header));
	memcpy(frame + sizeof(header), cipher, (size_t)packet_len);
	int frame_len = (int)sizeof(header) + packet_len;
	memcpy(frame + frame_len, tag, TAG_LEN);
	frame_len += TAG_LEN;
	if (sendto(hop_fd, frame, (size_t)frame_len, 0, (const struct sockaddr *)hop, sizeof(*hop)) < 0) {
		perror("sendto");
		return;
	}
	printf("[*] sealed %d byte command, seq %u\n", packet_len, seq);
	fflush(stdout);
}

static int
bind_udp(const char *host, int port)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	int reuse = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
		fprintf(stderr, "bad address %s\n", host);
		exit(1);
	}
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}
	return fd;
}

static int
hop_socket(const char *host, int port, struct sockaddr_in *dest)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	memset(dest, 0, sizeof(*dest));
	dest->sin_family = AF_INET;
	dest->sin_port = htons((uint16_t)port);
	if (inet_pton(AF_INET, host, &dest->sin_addr) != 1) {
		fprintf(stderr, "bad address %s\n", host);
		exit(1);
	}
	return fd;
}

static void
run_unwrap(const char *listen_host, int listen_port, const char *dest_host, int dest_port)
{
	int listen_fd = bind_udp(listen_host, listen_port);
	struct sockaddr_in dest;
	int dest_fd = hop_socket(dest_host, dest_port, &dest);

	printf("[*] unwrap listening on %s:%d, forwarding cleartext to %s:%d\n",
		listen_host, listen_port, dest_host, dest_port);
	fflush(stdout);
	while (1) {
		uint8_t buffer[2048];
		int nbytes = recvfrom(listen_fd, buffer, sizeof(buffer), 0, NULL, NULL);
		if (nbytes > 0) {
			uint8_t plain[MAX_BODY];
			int body_len = unwrap_datagram(buffer, nbytes, plain);
			if (body_len >= 0 && body_len < 64 && sendto(dest_fd, plain, (size_t)body_len, 0,
				(struct sockaddr *)&dest, sizeof(dest)) < 0) {
				perror("sendto");
			}
		}
	}
}

static void
run_wrap(const char *listen_host, int listen_port, const char *dest_host, int dest_port)
{
	int listen_fd = bind_udp(listen_host, listen_port);
	struct sockaddr_in hop;
	int hop_fd = hop_socket(dest_host, dest_port, &hop);

	printf("[*] wrap listening on %s:%d, forwarding encrypted frames to %s:%d\n",
		listen_host, listen_port, dest_host, dest_port);
	fflush(stdout);
	while (1) {
		uint8_t buffer[2048];
		int nbytes = recvfrom(listen_fd, buffer, sizeof(buffer), 0, NULL, NULL);
		if (nbytes > 0) {
			wrap_datagram(buffer, nbytes, hop_fd, &hop);
		}
	}
}

int
main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	if (argc == 6 && (strcmp(argv[1], "wrap") == 0 || strcmp(argv[1], "unwrap") == 0)) {
		if (strcmp(argv[1], "wrap") == 0) {
			run_wrap(argv[2], atoi(argv[3]), argv[4], atoi(argv[5]));
		} else {
			run_unwrap(argv[2], atoi(argv[3]), argv[4], atoi(argv[5]));
		}
	}
	fprintf(stderr, "usage: %s wrap <listen_host> <listen_port> <dest_host> <dest_port>\n"
		"       %s unwrap <listen_host> <listen_port> <dest_host> <dest_port>\n",
		argv[0], argv[0]);
	return 1;
}
