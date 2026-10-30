/* SPDX-License-Identifier: GPL-2.0-only */
OUTPUT_ARCH(aarch64)

SECTIONS {
	. = 0x86400000;

	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.text.vectab : { *(.text.vectab) }
}
