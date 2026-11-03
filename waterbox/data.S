/* data.S - ScummVM's data files (build/engine-data.zip, made by
 * make-data.py from the list scummvm-objs.mk writes), built into the core.
 * chimera-machine.cpp reads them as a zip archive in memory. */
	.section .rodata.chimera_engine_data, "a"
	.balign 16
	.globl chimera_engine_data
chimera_engine_data:
	.incbin DATA_ZIP
	.globl chimera_engine_data_end
chimera_engine_data_end:
	.byte 0
	.section .note.GNU-stack, "", @progbits
