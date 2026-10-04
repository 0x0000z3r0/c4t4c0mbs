CC = gcc
CFLAGS_BASE = -O0 -Wall -Wno-deprecated-declarations
CFLAGS_VULN = $(CFLAGS_BASE) -fno-stack-protector -z execstack -no-pie
CFLAGS_NX = $(CFLAGS_BASE) -fno-stack-protector -z noexecstack -no-pie
CFLAGS_HARD = $(CFLAGS_BASE) -fstack-protector-all -z noexecstack -pie -fPIE -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=2 -O2
CFLAGS_RE = -O0 -Wall -fno-stack-protector -no-pie -fcf-protection=none

.PHONY: all clean 01-system-monitor 02-auth-bypass 03-firmware 04-sensor 05-log-fmt 06-http-rop 07-session-uaf 08-mitigations 09-secure \
	10-plaintext 11-hashed 12-patch-branch 13-anti-debug 14-integrity

all: 01-system-monitor 02-auth-bypass 03-firmware 04-sensor 05-log-fmt 06-http-rop 07-session-uaf 08-mitigations 09-secure \
	10-plaintext 11-hashed 12-patch-branch 13-anti-debug 14-integrity

01-system-monitor:
	$(CC) $(CFLAGS_BASE) 01-system-monitor-elf/main.c -o 01-system-monitor-elf/system-monitor

02-auth-bypass:
	$(CC) $(CFLAGS_VULN) 02-auth-bypass-struct/main.c -o 02-auth-bypass-struct/auth-bypass

03-firmware:
	$(CC) $(CFLAGS_VULN) 03-firmware-ret2win/main.c -o 03-firmware-ret2win/firmware-update

04-sensor:
	$(CC) $(CFLAGS_VULN) 04-sensor-shellcode/main.c -o 04-sensor-shellcode/sensor-parser

05-log-fmt:
	$(CC) $(CFLAGS_VULN) 05-log-fmt-string/main.c -o 05-log-fmt-string/log-service

06-http-rop:
	$(CC) $(CFLAGS_NX) -fcf-protection=none 06-http-rop/main.c -o 06-http-rop/web-server

07-session-uaf:
	$(CC) $(CFLAGS_VULN) 07-session-uaf/main.c -o 07-session-uaf/session-manager

08-mitigations:
	$(CC) $(CFLAGS_VULN) 08-mitigations/main.c -o 08-mitigations/vuln-none
	$(CC) $(CFLAGS_BASE) -fno-stack-protector -z noexecstack -no-pie 08-mitigations/main.c -o 08-mitigations/vuln-nx
	$(CC) $(CFLAGS_BASE) -fstack-protector -z noexecstack -no-pie 08-mitigations/main.c -o 08-mitigations/vuln-canary
	$(CC) $(CFLAGS_BASE) -fstack-protector -z noexecstack -pie -fPIE 08-mitigations/main.c -o 08-mitigations/vuln-pie

09-secure:
	$(CC) $(CFLAGS_HARD) 09-secure-code/main.c -o 09-secure-code/secure

10-plaintext:
	$(CC) $(CFLAGS_RE) 10-plaintext-password/main.c -o 10-plaintext-password/plaintext-password

11-hashed:
	$(CC) $(CFLAGS_RE) 11-hashed-password/main.c -o 11-hashed-password/hashed-password

12-patch-branch:
	$(CC) $(CFLAGS_RE) 12-patch-branch/main.c -o 12-patch-branch/patch-branch

13-anti-debug:
	$(CC) $(CFLAGS_RE) 13-anti-debug/main.c -o 13-anti-debug/anti-debug

14-integrity:
	$(CC) $(CFLAGS_RE) 14-integrity-check/main.c -o 14-integrity-check/integrity-check
	python3 14-integrity-check/stamp.py 14-integrity-check/integrity-check

clean:
	rm -f 01-system-monitor-elf/system-monitor \
	      02-auth-bypass-struct/auth-bypass \
	      03-firmware-ret2win/firmware-update \
	      04-sensor-shellcode/sensor-parser \
	      05-log-fmt-string/log-service \
	      06-http-rop/web-server \
	      07-session-uaf/session-manager \
	      08-mitigations/vuln-* \
	      09-secure-code/secure \
	      10-plaintext-password/plaintext-password \
	      11-hashed-password/hashed-password \
	      12-patch-branch/patch-branch \
	      13-anti-debug/anti-debug \
	      14-integrity-check/integrity-check
