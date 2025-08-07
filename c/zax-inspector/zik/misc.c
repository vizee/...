#include "misc.h"

//�ر�д����
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

//����д����
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

//���pae�Ƿ���

ULONG check_pae()
{
	UCHAR IsPAE;

	//ͨ��������ʵ��mov eax, cr4
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

//��ȡidt
VOID load_idt(PIDTR pIDTR)
{
	IDTR idtr;

	//__asm cli
	__asm sidt idtr

	pIDTR->IDTBase = idtr.IDTBase;
	pIDTR->IDTLimit = idtr.IDTLimit;
	//__asm sti
}

