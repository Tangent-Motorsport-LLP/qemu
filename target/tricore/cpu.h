/*
 *  TriCore emulation for qemu: main CPU struct.
 *
 *  Copyright (c) 2012-2014 Bastian Koppelmann C-Lab/University Paderborn
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRICORE_CPU_H
#define TRICORE_CPU_H

#include "cpu-qom.h"
#include "exec/cpu-defs.h"
#include "tricore-defs.h"

struct tricore_boot_info;

typedef struct tricore_def_t tricore_def_t;

typedef struct CPUTriCoreState CPUTriCoreState;
struct CPUTriCoreState {
	uint32_t semihost_encoding;
	uint32_t semihost;
    /* GPR Register */
    uint32_t gpr_a[16];
    uint32_t gpr_d[16];
    /* CSFR Register */
    uint32_t PCXI;
/* Frequently accessed PSW_USB bits are stored separately for efficiency.
       This contains all the other bits.  Use psw_{read,write} to access
       the whole PSW.  */
    uint32_t PSW;

    /* PSW flag cache for faster execution
    */
    uint32_t PSW_USB_BIT31;
    uint32_t PSW_USB_BIT30;   /* Only if bit 31 set, then flag is set  */
    uint32_t PSW_USB_BIT29;  /* Only if bit 31 set, then flag is set  */
    uint32_t PSW_USB_BIT28;  /* Only if bit 31 set, then flag is set. */
    uint32_t PSW_USB_BIT27; /* Only if bit 31 set, then flag is set. */
    uint32_t PSW_USB_BIT26; /* Only if bit 31 set, then flag is set. */
    uint32_t PSW_USB_BIT25; /* Only if bit 31 set, then flag is set. */
    uint32_t PSW_USB_BIT24; /* Only if bit 31 set, then flag is set. */

    uint32_t PC;
    uint32_t SYSCON;
    uint32_t CPU_ID;
    uint32_t CORE_ID;
    uint32_t BIV;
    uint32_t BTV;
    uint32_t ISP;
    uint32_t ICR;
    uint32_t FCX;
    uint32_t LCX;
    uint32_t COMPAT;

    /* Mem Protection Register */
    uint32_t DPR0_0L;
    uint32_t DPR0_0U;
    uint32_t DPR0_1L;
    uint32_t DPR0_1U;
    uint32_t DPR0_2L;
    uint32_t DPR0_2U;
    uint32_t DPR0_3L;
    uint32_t DPR0_3U;

    uint32_t DPR1_0L;
    uint32_t DPR1_0U;
    uint32_t DPR1_1L;
    uint32_t DPR1_1U;
    uint32_t DPR1_2L;
    uint32_t DPR1_2U;
    uint32_t DPR1_3L;
    uint32_t DPR1_3U;

    uint32_t DPR2_0L;
    uint32_t DPR2_0U;
    uint32_t DPR2_1L;
    uint32_t DPR2_1U;
    uint32_t DPR2_2L;
    uint32_t DPR2_2U;
    uint32_t DPR2_3L;
    uint32_t DPR2_3U;

    uint32_t DPR3_0L;
    uint32_t DPR3_0U;
    uint32_t DPR3_1L;
    uint32_t DPR3_1U;
    uint32_t DPR3_2L;
    uint32_t DPR3_2U;
    uint32_t DPR3_3L;
    uint32_t DPR3_3U;

    uint32_t CPR0_0L;
    uint32_t CPR0_0U;
    uint32_t CPR0_1L;
    uint32_t CPR0_1U;
    uint32_t CPR0_2L;
    uint32_t CPR0_2U;
    uint32_t CPR0_3L;
    uint32_t CPR0_3U;

    uint32_t CPR1_0L;
    uint32_t CPR1_0U;
    uint32_t CPR1_1L;
    uint32_t CPR1_1U;
    uint32_t CPR1_2L;
    uint32_t CPR1_2U;
    uint32_t CPR1_3L;
    uint32_t CPR1_3U;

    uint32_t CPR2_0L;
    uint32_t CPR2_0U;
    uint32_t CPR2_1L;
    uint32_t CPR2_1U;
    uint32_t CPR2_2L;
    uint32_t CPR2_2U;
    uint32_t CPR2_3L;
    uint32_t CPR2_3U;

    uint32_t CPR3_0L;
    uint32_t CPR3_0U;
    uint32_t CPR3_1L;
    uint32_t CPR3_1U;
    uint32_t CPR3_2L;
    uint32_t CPR3_2U;
    uint32_t CPR3_3L;
    uint32_t CPR3_3U;

    uint32_t DPM0;
    uint32_t DPM1;
    uint32_t DPM2;
    uint32_t DPM3;

    uint32_t CPM0;
    uint32_t CPM1;
    uint32_t CPM2;
    uint32_t CPM3;

    /* Memory Management Registers */
    uint32_t MMU_CON;
    uint32_t MMU_ASI;
    uint32_t MMU_TVA;
    uint32_t MMU_TPA;
    uint32_t MMU_TPX;
    uint32_t MMU_TFA;
    /* {1.3.1 only */
    uint32_t BMACON;
    uint32_t SMACON;
    uint32_t DIEAR;
    uint32_t DIETR;
    uint32_t CCDIER;
    uint32_t MIECON;
    uint32_t PIEAR;
    uint32_t PIETR;
    uint32_t CCPIER;
    /*} */
    /* Debug Registers */
    uint32_t DBGSR;
    uint32_t EXEVT;
    uint32_t CREVT;
    uint32_t SWEVT;
    uint32_t TR0EVT;
    uint32_t TR1EVT;
    uint32_t DMS;
    uint32_t DCX;
    uint32_t DBGTCR;
    uint32_t CCTRL;
    uint32_t CCNT;
    uint32_t ICNT;
    uint32_t M1CNT;
    uint32_t M2CNT;
    uint32_t M3CNT;
    /* Floating Point Registers */
    float_status fp_status;
    /* QEMU */
    int error_code;
    uint32_t hflags;    /* CPU State */

    /* Internal CPU feature flags.  */
    uint64_t features;

    const tricore_def_t *cpu_model;
    void *irq[8];
    struct QEMUTimer *timer; /* Internal timer */
    uint32_t main_addr_entry;
    uint32_t main_addr_next;
    uint32_t main_sp_correction;
    const char *kernel_filename;
};

/**
 * TriCoreCPU:
 * @env: #CPUTriCoreState
 *
 * A TriCore CPU.
 */
struct TriCoreCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPUTriCoreState env;
};


hwaddr tricore_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
void tricore_cpu_dump_state(CPUState *cpu, FILE *f, int flags);
void  tricore_cpu_do_unaligned_access(CPUState *cs, vaddr addr,
                                    MMUAccessType access_type, int mmu_idx,
                                    uintptr_t retaddr) QEMU_NORETURN;


#define MASK_PCXI_PCPN_TC13 	0xff000000
#define MASK_PCXI_PCPN_TC131 	0xff000000
#define MASK_PCXI_PCPN_TC16 	0xff000000
#define MASK_PCXI_PCPN_TC161 	0x3fc00000
#define MASK_PCXI_PCPN_TC162 	0x3fc00000
#define MASK_PCXI_PIE_TC13		0x00800000
#define MASK_PCXI_PIE_TC131		0x00800000
#define MASK_PCXI_PIE_TC16		0x00800000
#define MASK_PCXI_PIE_TC161		0x00200000
#define MASK_PCXI_PIE_TC162		0x00200000
#define MASK_PCXI_UL_TC13		0x00400000
#define MASK_PCXI_UL_TC131		0x00400000
#define MASK_PCXI_UL_TC16		0x00400000
#define MASK_PCXI_UL_TC161		0x00100000
#define MASK_PCXI_UL_TC162		0x00100000

#define MASK_PCXI_PCXS 0x000f0000
#define MASK_PCXI_PCXO 0x0000ffff

#define MASK_ICR_IE_TC13   0x00000100
#define MASK_ICR_IE_TC131  0x00000100
#define MASK_ICR_IE_TC16   0x00008000
#define MASK_ICR_IE_TC161  0x00008000
#define MASK_ICR_IE_TC162  0x00008000
#define MASK_ICR_PIPN 0x00ff0000
#define MASK_ICR_CCPN 0x000000ff


#define MASK_PSW_USB 0xff000000
#define MASK_USB_BIT31 0x80000000
#define MASK_USB_BIT30 0x40000000
#define MASK_USB_BIT29 0x20000000
#define MASK_USB_BIT28 0x10000000
#define MASK_USB_BIT27 0x08000000
#define MASK_USB_BIT26 0x04000000
#define MASK_USB_BIT25 0x02000000
#define MASK_USB_BIT24 0x01000000
#define MASK_PSW_PRS 0x00003000
#define MASK_PSW_IO  0x00000c00
#define MASK_PSW_IS  0x00000200
#define MASK_PSW_GW  0x00000100
#define MASK_PSW_CDE 0x00000080
#define MASK_PSW_CDC 0x0000007f
#define MASK_PSW_FPU_RM 0x3000000

#define MASK_SYSCON_PRO_TEN 0x2
#define MASK_SYSCON_FCD_SF  0x1

#define MASK_CPUID_MOD     0xffff0000
#define MASK_CPUID_MOD_32B 0x0000ff00
#define MASK_CPUID_REV     0x000000ff


#define MASK_FCX_FCXS 0x000f0000
#define MASK_FCX_FCXO 0x0000ffff

#define MASK_LCX_LCXS 0x000f0000
#define MASK_LCX_LCX0 0x0000ffff

#define MASK_DBGSR_DE 0x1
#define MASK_DBGSR_HALT 0x6
#define MASK_DBGSR_SUSP 0x10
#define MASK_DBGSR_PREVSUSP 0x20
#define MASK_DBGSR_PEVT 0x40
#define MASK_DBGSR_EVTSRC 0x1f00

#define TRICORE_HFLAG_KUU     0x3
#define TRICORE_HFLAG_UM0     0x00002 /* user mode-0 flag          */
#define TRICORE_HFLAG_UM1     0x00001 /* user mode-1 flag          */
#define TRICORE_HFLAG_SM      0x00000 /* kernel mode flag          */

enum tricore_features {
    TRICORE_FEATURE_13  = 0x00000001,
    TRICORE_FEATURE_131 = 0x00000002,
    TRICORE_FEATURE_16  = 0x00000004,
    TRICORE_FEATURE_161 = 0x00000008,
    TRICORE_FEATURE_162 = 0x00000010,
};


/* Some handy definitions for upward/downward compatibility of insns.  */
#define TRICORE_V1_6_2_UP (TRICORE_FEATURE_162)
#define TRICORE_V1_6_1_UP (TRICORE_FEATURE_161 | TRICORE_V1_6_2_UP)
#define TRICORE_V1_6_UP   (TRICORE_FEATURE_16 | TRICORE_V1_6_1_UP)
#define TRICORE_V1_3_1_UP (TRICORE_FEATURE_131 | TRICORE_V1_6_UP)
#define TRICORE_V1_3_UP   (TRICORE_FEATURE_13 | TRICORE_V1_3_1_UP)

#define TRICORE_V1_3_DN    TRICORE_FEATURE_13
#define TRICORE_V1_3_1_DN (TRICORE_FEATURE_131 | TRICORE_V1_3_DN)
#define TRICORE_V1_6_DN   (TRICORE_FEATURE_16 | TRICORE_V1_3_1_DN)
#define TRICORE_V1_6_1_DN (TRICORE_FEATURE_161 | TRICORE_V1_6_DN)
#define TRICORE_V1_6_2_DN (TRICORE_FEATURE_162 | TRICORE_V1_6_1_DN)


/* Instruction set architecture V1.1.  */
#define EF_TRICORE_V1_1	        0x00000001
#define EF_EABI_TRICORE_V1_1	0x80000000

/* Instruction set architecture V1.2.  */
#define EF_TRICORE_V1_2	        0x00000002
#define EF_EABI_TRICORE_V1_2	0x40000000

/* Instruction set architecture V1.3  */
#define EF_TRICORE_V1_3		0x00000004
#define EF_EABI_TRICORE_V1_3	0x20000000

/* Instruction set architecture V1.3.1  */

#define EF_TRICORE_V1_3_1	0x00000100
#define EF_EABI_TRICORE_V1_3_1	0x00800000

/* Instruction set architecture V1.6.  */

#define EF_TRICORE_V1_6		0x00000200
#define EF_EABI_TRICORE_V1_6	0x00400000

/* Instruction set architecture V1.6.1.  */

#define EF_TRICORE_V1_6_1	0x00000400
#define EF_EABI_TRICORE_V1_6_1	0x00200000

/* Instruction set architecture V1.6.2.  */

#define EF_TRICORE_V1_6_2	0x00000800
#define EF_EABI_TRICORE_V1_6_2	0x00100000

/* PCP co-processor.  */

#define EF_TRICORE_PCP		0x00000010
#define EF_EABI_TRICORE_PCP	0x01000000

/* PCP co-processor, version 2.  */

#define EF_TRICORE_PCP2		0x00000020
#define EF_EABI_TRICORE_PCP2	0x02000000

#define EF_TRICORE_CORE_MASK    	0x00000f0f
#define EF_EABI_TRICORE_CORE_MASK	0xf0f00000


#define SEC_PCP 0x10000000
static inline int tricore_feature(CPUTriCoreState *env, int feature)
{
    return (env->features & feature);
}



/* TriCore Traps Classes*/
enum {
    TRAPC_NONE     = -1,
    TRAPC_MMU      = 0,
    TRAPC_PROT     = 1,
    TRAPC_INSN_ERR = 2,
    TRAPC_CTX_MNG  = 3,
    TRAPC_SYSBUS   = 4,
    TRAPC_ASSERT   = 5,
    TRAPC_SYSCALL  = 6,
    TRAPC_NMI      = 7,
    TRAPC_IRQ      = 8
};

/* Class 0 TIN */
enum {
    TIN0_VAF = 0,
    TIN0_VAP = 1,
};

/* Class 1 TIN */
enum {
    TIN1_PRIV = 1,
    TIN1_MPR  = 2,
    TIN1_MPW  = 3,
    TIN1_MPX  = 4,
    TIN1_MPP  = 5,
    TIN1_MPN  = 6,
    TIN1_GRWP = 7,
};

/* Class 2 TIN */
enum {
    TIN2_IOPC = 1,
    TIN2_UOPC = 2,
    TIN2_OPD  = 3,
    TIN2_ALN  = 4,
    TIN2_MEM  = 5,
};

/* Class 3 TIN */
enum {
    TIN3_FCD  = 1,
    TIN3_CDO  = 2,
    TIN3_CDU  = 3,
    TIN3_FCU  = 4,
    TIN3_CSU  = 5,
    TIN3_CTYP = 6,
    TIN3_NEST = 7,
};

/* Class 4 TIN */
enum {
    TIN4_PSE = 1,
    TIN4_DSE = 2,
    TIN4_DAE = 3,
    TIN4_CAE = 4,
    TIN4_PIE = 5,
    TIN4_DIE = 6,
};

/* Class 5 TIN */
enum {
    TIN5_OVF  = 1,
    TIN5_SOVF = 1,
};

/* Class 6 TIN
 *
 * Is always TIN6_SYS
 */

/* Class 7 TIN */
enum {
    TIN7_NMI = 0,
};

uint32_t psw_read(CPUTriCoreState *env);
void psw_write(CPUTriCoreState *env, uint32_t val);
int tricore_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n);
int tricore_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n);

void fpu_set_state(CPUTriCoreState *env);

#define MMU_USER_IDX 2

void tricore_cpu_list(void);

#define cpu_list tricore_cpu_list

static inline int cpu_mmu_index(CPUTriCoreState *env, bool ifetch)
{
    return 0;
}

typedef CPUTriCoreState CPUArchState;
typedef TriCoreCPU ArchCPU;

#include "exec/cpu-all.h"

void cpu_state_reset(CPUTriCoreState *s);
void tricore_tcg_init(void);

static inline void cpu_get_tb_cpu_state(CPUTriCoreState *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->PC;
    *cs_base = 0;
    *flags = 0;
}

#define TRICORE_CPU_TYPE_SUFFIX "-" TYPE_TRICORE_CPU
#define TRICORE_CPU_TYPE_NAME(model) model TRICORE_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_TRICORE_CPU

/* helpers.c */
bool tricore_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                          MMUAccessType access_type, int mmu_idx,
                          bool probe, uintptr_t retaddr);
void tricore_cpu_do_interrupt(CPUState *cpu);
bool tricore_cpu_exec_interrupt(CPUState *cpu, int int_req);
void tricore_raise_exception_sync(CPUTriCoreState *env, uint32_t class,uint32_t tin);

#define INSNOPCODE_DEBUG16 0xA000
#define GCC_VIRTIO_MARKERPCM2 0x6f69
#define GCC_VIRTIO_MARKERPCM4 0x765f
#define GCC_VIRTIO_MARKEREXITPCM2 0x0E60
#define GCC_VIRTIO_MARKEREXITPCM2_MASK 0x0FFF
#define EXCP_EXIT 0x1000
#define EXCP_SEMIHOST 0x1001
#define EXCP_UNALIGN 0x1002
#define __VIRTUAL_IO__
void tricore_vio_init (void);
int do_tricore_semihosting (CPUState *cs);

#endif /* TRICORE_CPU_H */
