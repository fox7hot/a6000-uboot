/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * qhypstub with Nokia TrustZone SMC Emulation
 * Intercepts and satisfies all TrustZone SMC calls from Lumia 640 UEFI
 */
.cpu	cortex-a53

.equ	STATE_INITIAL,	0
.equ	STATE_AARCH32,	1
.equ	STATE_AARCH64,	2

/* Hypervisor Configuration Register (EL2) */
.equ	HCR_EL2_RW,	1 << 31		/* register width, EL1 is aarch64 */
.equ	HCR_EL2_TSC,	1 << 19		/* trap all SMC instructions to EL2! */
.equ	HCR_EL2_VM,	1 << 0		/* enable stage 2 address translation */

/* Saved Program Status Register (EL2) */
.equ	SPSR_EL2_AARCH64_D,	1 << 9	/* debug exception mask (aarch64) */
.equ	SPSR_EL2_A,		1 << 8	/* SError interrupt mask */
.equ	SPSR_EL2_I,		1 << 7	/* IRQ interrupt mask */
.equ	SPSR_EL2_F,		1 << 6	/* FIQ interrupt mask */
.equ	SPSR_EL2_AIF,		SPSR_EL2_A | SPSR_EL2_I | SPSR_EL2_F
.equ	SPSR_EL2_AARCH32_SVC,	0b10011		/* aarch32 supervisor mode */
.equ	SPSR_EL2_AARCH64_EL1H,	0b00101		/* aarch64 EL1h mode */

/* Counter-Timer Hypervisor Control Register (EL2) */
.equ	CNTHCTL_EL2_EL1PCEN,	1 << 1	/* allow EL0/EL1 timer access */
.equ	CNTHCTL_EL2_EL1PCTEN,	1 << 0	/* allow EL0/EL1 counter access */

/* Architectural Feature Trap Register (EL2) */
.equ	CPTR_EL2_RES1,		1 << 13 | 1 << 12 | 1 << 9 | 1 << 8 | 0xff

/* Virtualization Translation Control Register */
.equ	VTCR_EL2_RES1,		1 << 31
.equ	VTCR_EL2_32BIT_4KB_L1,	0 << 16 | 0 << 14 | 1 << 6 | 32

/* System Control Register (EL1) */
.equ	SCTLR_EL1_AARCH32_RES1_MSB,	1 << 23 | 1 << 22
.equ	SCTLR_EL1_AARCH32_RES1_LSB,	1 << 11 | 1 << 4 | 1 << 3
.equ	SCTLR_EL1_CP15BEN,		1 << 5	/* enable CP15 barrier */

/* SMC Calling Convention return codes */
.equ	SMCCC_SUCCESS,			 0
.equ	SMCCC_NOT_SUPPORTED,		-1
.equ	SMCCC_INVALID_PARAMETER,	-3

.global _start
_start:
	mov	lr, x0		/* save entry address to link register */

	.macro clrregs
		mov	x0, xzr
		mov	x1, xzr
		mov	x2, xzr
		mov	x3, xzr
	.endm

	adr	x0, execution_state
	ldr	w2, [x0]
	cbnz	w2, skip_init
	str	w1, [x0]

	/* Bring RPM out of reset */
	mov	x0, 0x1860000	/* GCC_APSS_MISC */
	ldr	w3, [x0]
	and	w3, w3, ~0b1	/* RPM_RESET_REMOVAL */
	str	w3, [x0]

	/* Set exception vector table for EL2 */
	adr	x0, el2_vector_table
	msr	vbar_el2, x0

	cmp	x1, STATE_AARCH64
	beq	bootup_smc_switch_aarch64
	b	not_aarch64

skip_init:
	cmp	x1, STATE_AARCH64
	bne	not_aarch64

	cmp	w2, STATE_AARCH64
	bne	aarch64_el1

	clrregs
	ret

aarch64_el1:
	mov	x0, HCR_EL2_RW
	msr	hcr_el2, x0
	mov	x3, SPSR_EL2_AARCH64_D | SPSR_EL2_AIF | SPSR_EL2_AARCH64_EL1H
	b	prepare_eret

not_aarch64:
	cmp	x1, STATE_AARCH32
	bne	panic

	/* aarch32 EL1 setup */
	mov	x0, SCTLR_EL1_AARCH32_RES1_MSB
	movk	x0, SCTLR_EL1_AARCH32_RES1_LSB | SCTLR_EL1_CP15BEN
	msr	sctlr_el1, x0

	/* Trap all SMC instructions from EL1 to EL2 (HCR_EL2.TSC = 1)! */
	mov	x0, HCR_EL2_TSC
	msr	hcr_el2, x0

	mov	x3, SPSR_EL2_AIF | SPSR_EL2_AARCH32_SVC

prepare_eret:
	msr	spsr_el2, x3

	/* Allow EL1 to access timer/counter */
	mov	x0, CNTHCTL_EL2_EL1PCEN | CNTHCTL_EL2_EL1PCTEN
	msr	cnthctl_el2, x0
	msr	cntvoff_el2, xzr

	/* Disable coprocessor traps */
	mov	x3, CPTR_EL2_RES1
	msr	cptr_el2, x3
	msr	hstr_el2, xzr

	/* Configure EL1 return address and drop to EL1! */
	msr	elr_el2, lr
	clrregs
	eret

panic:
	b	panic

bootup_smc_switch_aarch64:
	mov	x0, 0x2000000
	movk	x0, 0x10f
	mov	x1, 0x12
	adr	x2, scm_jump_aarch64_args
	mov	x3, scm_jump_aarch64_args_end - scm_jump_aarch64_args
	str	lr, [x2, scm_jump_aarch64_args_end - scm_jump_aarch64_args - 8]

smc_switch_aarch64:
	mov	x15, VTCR_EL2_RES1
	movk	x15, VTCR_EL2_32BIT_4KB_L1
	msr	vtcr_el2, x15
	mov	x15, 1 << 32
	msr	vttbr_el2, x15

	mov	x15, HCR_EL2_VM
	msr	hcr_el2, x15

	smc	0

	msr	hcr_el2, xzr
	eret

finish_smc_switch_aarch64:
	msr	hcr_el2, xzr
	msr	vbar_el2, xzr

	adr	x30, execution_state
	mov	w29, STATE_AARCH64
	str	w29, [x30]
	mov	w29, wzr

	mrs	lr, elr_el2
	ret

/* Exception Vectors */
.section .text.vectab
.macro excvec label
	.align 7
\label:
.endm

.align	11
el2_vector_table:
	excvec	el2_sp0_sync
	b	panic
	excvec	el2_sp0_irq
	b	panic
	excvec	el2_sp0_fiq
	b	panic
	excvec	el2_sp0_serror
	b	panic

	excvec	el2_sp2_sync
	b	panic
	excvec	el2_sp2_irq
	b	panic
	excvec	el2_sp2_fiq
	b	panic
	excvec	el2_sp2_serror
	b	panic

	excvec	el1_aarch64_sync
	mrs	x30, esr_el2
	lsr	x30, x30, 26
	cmp	x30, 0b100000
	beq	finish_smc_switch_aarch64
	b	panic
	excvec	el1_aarch64_irq
	b	panic
	excvec	el1_aarch64_fiq
	b	panic
	excvec	el1_aarch64_serror
	b	panic

	excvec	el1_aarch32_sync
	mrs	x15, esr_el2
	lsr	x15, x15, 26
	cmp	x15, 0b010011	/* Trapped SMC32 from EL1? */
	beq	handle_smc32
	cmp	x15, 0b010010	/* HVC32? */
	beq	hvc32_stub
	b	panic
	excvec	el1_aarch32_irq
	b	panic
	excvec	el1_aarch32_fiq
	b	panic
	excvec	el1_aarch32_serror
	b	panic

	excvec	el2_vector_table_end

/* Nokia TrustZone SMC Emulator */
handle_smc32:
	/* Advance elr_el2 past the 4-byte smc instruction */
	mrs	x15, elr_el2
	add	x15, x15, #4
	msr	elr_el2, x15

	/* Return 0 (SMCCC_SUCCESS) in w0 */
	mov	x0, xzr
	eret

hvc32_stub:
	mov	x0, SMCCC_NOT_SUPPORTED
	eret

.data
execution_state:
	.word	0

	.align	3
scm_jump_aarch64_args:
	.quad	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
scm_jump_aarch64_args_end:

.section .rodata
.align	3
.ascii	"qhypstub-nokia-tz"
