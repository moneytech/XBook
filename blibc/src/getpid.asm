;----
;file:		user/lib/log.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"


; uint32_t getpid();
global getpid
getpid:
	

	mov eax, SYS_GETPID
	int INT_VECTOR_SYS_CALL
	
	
	ret