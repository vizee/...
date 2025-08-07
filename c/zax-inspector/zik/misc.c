#include "misc.h"

//关闭写保护
VOID wp_off()
{
	__asm
	{
		cli
		mov eax, cr0
		and eax, 0fffeffffh
		mov cr0, eax
	}
}

//开启写保护
VOID wp_on()
{
	__asm
	{
		mov eax, cr0
		or eax, 10000h
		mov cr0, eax
		sti
	}
}

//检查pae是否开启

ULONG check_pae()
{
	UCHAR IsPAE;

	//通过机器码实现mov eax, cr4
	__asm
	{
		_emit 0x0f
		_emit 0x20
		_emit 0xe0
		test eax, 20h
		setne al
		mov IsPAE, al
	}
	return (ULONG)IsPAE;
}

//读取idt
VOID load_idt(PIDTR pIDTR)
{
	IDTR idtr;

	//__asm cli
	__asm sidt idtr

	pIDTR->IDTBase = idtr.IDTBase;
	pIDTR->IDTLimit = idtr.IDTLimit;
	//__asm sti
}

