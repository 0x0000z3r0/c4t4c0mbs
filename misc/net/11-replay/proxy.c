#include <arpa/inet.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAGIC 0xDEADBEEF
#define KEY_LEN 16
#define IV_LEN 16
#define TAG_LEN 32
#define MAX_BODY 256

struct secure_header {
	uint32_t magic;
	uint32_t session_id;
	uint32_t seq;
	uint16_t flags;
	uint16_t inner_len;
	uint8_t iv[IV_LEN];
};

static const uint8_t shared_key[KEY_LEN] = "lab-proxy-key!!!";
static uint32_t next_seq = 1;

static int
crypt_payload(int encrypt, const uint8_t *iv, const uint8_t *input, int len, uint8_t *output)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		return -1;
	}

	int ok;
	if (encrypt) {
		ok = EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, shared_key, iv);
	} else {
		ok = EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, shared_key, iv);
	}
	int out_len = 0;
	int final_len = 0;
	if (ok == 1 && encrypt) {
		ok = EVP_EncryptUpdate(ctx, output, &out_len, input, len);
		if (ok == 1) {
			ok = EVP_EncryptFinal_ex(ctx, output + out_len, &final_len);
		}
	} else if (ok == 1) {
		ok = EVP_DecryptUpdate(ctx, output, &out_len, input, len);
		if (ok == 1) {
			ok = EVP_DecryptFinal_ex(ctx, output + out_len, &final_len);
		}
	}
	EVP_CIPHER_CTX_free(ctx);
	if (ok != 1) {
		return -1;
	}
	return out_len + final_len;
}

static int
seal_payload(const uint8_t *iv, const uint8_t *plain, int len, uint8_t *cipher)
{
	return crypt_payload(1, iv, plain, len, cipher);
}

static int
open_payload(const uint8_t *iv, const uint8_t *cipher, int len, uint8_t *plain)
{
	return crypt_payload(0, iv, cipher, len, plain);
}

static void
compute_tag(const uint8_t *data, int len, uint8_t *tag)
{
	unsigned int tag_len = TAG_LEN;
	HMAC(EVP_sha256(), shared_key, KEY_LEN, data, (size_t)len, tag, &tag_len);
}

static void
fill_tag(const uint8_t *header, int header_len, const uint8_t *cipher, int cipher_len, uint8_t *tag)
{
	uint8_t covered[sizeof(struct secure_header) + MAX_BODY];
	memcpy(covered, header, (size_t)header_len);
	memcpy(covered + header_len, cipher, (size_t)cipher_len);
	compute_tag(covered, header_len + cipher_len, tag);
}

static int
tag_is_valid(const uint8_t *frame, int body_len)
{
	uint8_t expect[TAG_LEN];
	const uint8_t *cipher = frame + sizeof(struct secure_header);
	const uint8_t *tag = cipher + body_len;

	fill_tag(frame, (int)sizeof(struct secure_header), cipher, body_len, expect);
	return memcmp(expect, tag, TAG_LEN) == 0;
}

static void
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	fflush(stdout);
}

static void
apply_frame(const struct secure_header *header, const uint8_t *plain, int body_len)
{
	char command[MAX_BODY + 1];
	memcpy(command, plain, (size_t)body_len);
	command[body_len] = '\0';
	uint16_t flags = ntohs(header->flags);
	printf("[*] command: %s flags: %u seq: %u\n", command, flags, ntohl(header->seq));
	fflush(stdout);

	if (body_len == 6 && memcmp(command, "UNLOCK", 6) == 0) {
		// VULNERABILITY: seq is inside the HMAC but is never remembered.
		// The same datagram is accepted again.
		unlock_firmware();
		// FIX:
		// static uint32_t last_seq;
		// uint32_t seq = ntohl(header->seq);
		// if (seq <= last_seq) return;
		// last_seq = seq;
		// unlock_firmware();
	} else {
		printf("[+] command ignored\n");
	}
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
	if (body_len < 0 || body_len > MAX_BODY) {
		printf("[-] bad length\n");
		return -1;
	}
	if (frame_len < (int)sizeof(header) + body_len + TAG_LEN) {
		printf("[-] truncated frame\n");
		return -1;
	}
	if (!tag_is_valid(frame, body_len)) {
		printf("[-] bad tag\n");
		return -1;
	}
	if (open_payload(header.iv, frame + sizeof(header), body_len, plain_out) != body_len) {
		printf("[-] decrypt failed\n");
		return -1;
	}
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
	header.seq = htonl(next_seq++);
	header.inner_len = htons((uint16_t)packet_len);
	if (RAND_bytes(header.iv, IV_LEN) != 1) {
		printf("[-] RAND_bytes failed\n");
		return;
	}
	uint8_t cipher[MAX_BODY];
	if (seal_payload(header.iv, packet, packet_len, cipher) != packet_len) {
		printf("[-] encrypt failed\n");
		return;
	}

	uint8_t frame[sizeof(header) + MAX_BODY + TAG_LEN];
	memcpy(frame, &header, sizeof(header));
	memcpy(frame + sizeof(header), cipher, (size_t)packet_len);
	int frame_len = (int)sizeof(header) + packet_len;
	uint8_t tag[TAG_LEN];
	fill_tag(frame, (int)sizeof(header), cipher, packet_len, tag);
	memcpy(frame + frame_len, tag, TAG_LEN);
	frame_len += TAG_LEN;
	if (sendto(hop_fd, frame, (size_t)frame_len, 0, (const struct sockaddr *)hop, sizeof(*hop)) < 0) {
		perror("sendto");
		return;
	}
	printf("[*] wrapped %d byte command, seq %u\n", packet_len, next_seq - 1);
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
			if (body_len >= 0 && sendto(dest_fd, plain, (size_t)body_len, 0,
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
