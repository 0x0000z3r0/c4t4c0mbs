.section .text
.globl shellcode
shellcode:
	csrci mstatus, 8
	li a0, 0x10000000
	lla a1, message
next_byte:
	lbu a2, 0(a1)
	beqz a2, hang
wait_tx:
	lbu a3, 5(a0)
	andi a3, a3, 0x20
	beqz a3, wait_tx
	sb a2, 0(a0)
	addi a1, a1, 1
	j next_byte
hang:
	j hang

message:
	.asciz "\r\n  +------------------------------------------+\r\n  |  shellcode executed from the task stack  |\r\n  |  FreeRTOS did not stop it                |\r\n  +------------------------------------------+\r\n"
