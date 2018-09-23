OUTPUT_ARCH(arm)

ENTRY(_start)

MEMORY {
    ram : ORIGIN = 0x00102180, LENGTH = 0xBA80
    rom : ORIGIN = 0x000C1000, LENGTH = 0x3F000
}

ramBase = ORIGIN(ram);
romBase = ORIGIN(rom);

SECTIONS {

    . = romBase;
    .start ALIGN(4) : {	
        *(.text.start)
    } >rom
    
    . = romBase + 0x01FC;
    .rom_info ALIGN(4) : {	    	    	    	     
    	*(.data.rom_info)
    } >rom 

    .text ALIGN(4) : {
        *(.text)
        *(.text.*)        
    } >rom

    .rodata ALIGN(4) : {
        *(.rodata)
        *(.rodata.*)        
    } >rom

    .data ALIGN(4) : {
        *(.data)
        *(.data.*)        
    } >rom

    .got ALIGN(4) : {
        *(.got)
        *(.got.*)        
    } >rom

    __boot_end = .;

    . = ramBase;
    .bss ALIGN(16) : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } >ram
	
	.secbuf ALIGN(4) : {
		_secbuf_start = .;
		*(.secbuf)
		_secbuf_end = .;
	} >ram

}
