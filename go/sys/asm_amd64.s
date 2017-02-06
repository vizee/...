#include "textflag.h"

// func lockOr32(addr *uint32, val uint32)
TEXT ·lockOr32(SB),NOSPLIT,$0-12
	MOVQ addr+0(FP), BP
	MOVL val+8(FP), AX
	LOCK
	ORL AX, 0(BP)
	RET

// func lockCmpxchg8(addr *byte, cmp byte, val byte) (ok bool)
TEXT ·lockCmpxchg8(SB),NOSPLIT,$0-17
	MOVQ addr+0(FP), BP
	MOVB cmp+8(FP), AL
	MOVB val+9(FP), AH
	LOCK
	CMPXCHGB AH, (BP)
	SETEQ ok+16(FP)
	RET
