#include "common.h"
#include "x86.h"
#include "device.h"

void kEntry(void) {

	// Interruption is disabled in bootloader
	assert(0);
	initSerial();	// initialize serial port
	putStr("Here!\n\n");
	// DONE: 做一系列初始化
	initIdt();      // initialize idt
	initIntr();		// iniialize 8259a
	initSeg();	 	// initialize gdt, tss
	initVga(); 		// initialize vga device
	initKeyTable(); // initialize keyboard device
	loadUMain(); 	// load user program, enter user space

	while(1);
	assert(0);
}
