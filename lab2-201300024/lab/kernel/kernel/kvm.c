#include "x86.h"
#include "device.h"
#define PT_LOAD 1
#define ELF_MAGIC 0x464c457f

SegDesc gdt[NR_SEGMENTS];       // the new GDT, NR_SEGMENTS=7, defined in x86/memory.h
TSS tss;

void initSeg() { // setup kernel segements
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	//gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_USER);
	gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0x00200000,0x000fffff, DPL_USER);
	//gdt[SEG_UDATA] = SEG(STA_W,         0,       0xffffffff, DPL_USER);
	gdt[SEG_UDATA] = SEG(STA_W,         0x00200000,0x000fffff, DPL_USER);
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	setGdt(gdt, sizeof(gdt)); // gdt is set in bootloader, here reset gdt in kernel

	/*
	 * 初始化TSS
	 */
	tss.esp0 = 0x1fffff;
	tss.ss0 = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	asm volatile("movw %%ax,%%ds":: "a" (KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax,%%es":: "a" (KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax,%%fs":: "a" (KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax,%%gs":: "a" (KSEL(SEG_KDATA)));
	asm volatile("movw %%ax,%%ss":: "a" (KSEL(SEG_KDATA)));

	lLdt(0);
	
}

void enterUserSpace(uint32_t entry) {
	/*
	 * Before enter user space 
	 * you should set the right segment registers here
	 * and use 'iret' to jump to ring3
	 */
	uint32_t EFLAGS = 0;
	asm volatile("pushl %0":: "r" (USEL(SEG_UDATA))); // push ss
	asm volatile("pushl %0":: "r" (0x2fffff)); 
	asm volatile("pushfl"); //push eflags, sti
	asm volatile("popl %0":"=r" (EFLAGS));
	asm volatile("pushl %0"::"r"(EFLAGS|0x200));
	asm volatile("pushl %0":: "r" (USEL(SEG_UCODE))); // push cs
	asm volatile("pushl %0":: "r" (entry)); 
	asm volatile("iret");
}

/*
kernel is loaded to location 0x100000, i.e., 1MB
size of kernel is not greater than 200*512 bytes, i.e., 100KB
user program is loaded to location 0x200000, i.e., 2MB
size of user program is not greater than 200*512 bytes, i.e., 100KB
*/

void loadUMain(void) {
	// DONE: 参照bootloader加载内核的方式
	unsigned int phoff = 0x34; // program header offset
	unsigned int elf = 0x200000; // physical memory addr to load
	unsigned int uMainEntry = 0x0;

	for (int i = 0; i < 200; i++) {
		readSect((void*)(elf + i*512), 201+i);
	}

	struct ELFHeader *header = (struct ELFHeader *)elf;
	assert(header->magic == ELF_MAGIC);
	uMainEntry = header->entry;
	phoff = header->phoff;
	unsigned short phnum = header->phnum;
	struct ProgramHeader pheader[phnum];
	struct ProgramHeader *ph = (struct ProgramHeader *)(elf + phoff);

	for(int i = 0; i < phnum; i++) {
		pheader[i] = *(ph + i);
	}

	for(int i = 0; i < phnum; i++){
		if(pheader[i].type == PT_LOAD){
			unsigned int off = pheader[i].off;
			unsigned int addr = pheader[i].vaddr;
			unsigned int filesz = pheader[i].filesz;
			unsigned int memsz = pheader[i].memsz;
			for(int j = 0; j< filesz; j++){
				*(unsigned char *)(addr + j + elf) = *(unsigned char *)(elf + off + j);
			}
			for(int j = filesz; j < memsz; j++){
				*(unsigned char *)(addr + j + elf) = (unsigned char)0;
			}
		}
	}

	enterUserSpace(uMainEntry);
}
