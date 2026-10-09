CC = gcc
CFLAGS_VULN = -fno-stack-protector -no-pie -z execstack -g
CFLAGS_SECURE = -fstack-protector-all -pie -fPIE -Wl,-z,relro,-z,now -O2 -g

LABS = 01-naive-proxy 02-signedness-bypass 03-integer-overflow 04-oob-read-leak 05-format-string-log 06-state-confusion 07-heap-reassembly 08-secure-proxy

.PHONY: all clean $(LABS)

all: $(LABS)

01-naive-proxy:
	$(CC) $(CFLAGS_VULN) 01-naive-proxy/proxy.c -o 01-naive-proxy/proxy

02-signedness-bypass:
	$(CC) $(CFLAGS_VULN) 02-signedness-bypass/proxy.c -o 02-signedness-bypass/proxy

03-integer-overflow:
	$(CC) $(CFLAGS_VULN) 03-integer-overflow/proxy.c -o 03-integer-overflow/proxy

04-oob-read-leak:
	$(CC) $(CFLAGS_VULN) 04-oob-read-leak/proxy.c -o 04-oob-read-leak/proxy

05-format-string-log:
	# Format string lab might need slightly different flags if we want to overwrite GOT, so we ensure no RELRO
	$(CC) $(CFLAGS_VULN) -Wl,-z,norelro 05-format-string-log/proxy.c -o 05-format-string-log/proxy

06-state-confusion:
	$(CC) $(CFLAGS_VULN) 06-state-confusion/proxy.c -o 06-state-confusion/proxy

07-heap-reassembly:
	$(CC) $(CFLAGS_VULN) 07-heap-reassembly/proxy.c -o 07-heap-reassembly/proxy

08-secure-proxy:
	$(CC) $(CFLAGS_SECURE) 08-secure-proxy/proxy.c -o 08-secure-proxy/proxy

clean:
	rm -f */proxy
