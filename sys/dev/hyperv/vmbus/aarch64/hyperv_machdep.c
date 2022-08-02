/*-
 * Copyright (c) 2016-2017 Microsoft Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/vdso.h>

#include <machine/cpufunc.h>
#include <machine/md_var.h>

#include <vm/vm.h>

#include <dev/hyperv/include/hyperv.h>
#include <dev/hyperv/include/hyperv_busdma.h>
#include <dev/hyperv/vmbus/hyperv_machdep.h>
#include <dev/hyperv/vmbus/hyperv_reg.h>
#include <dev/hyperv/vmbus/hyperv_var.h>
#include <dev/psci/smccc.h>

#define HVCALL_SET_VP_REGISTERS 0x0051
#define HVCALL_GET_VP_REGISTERS         0x0050
#define BIT(A) (1 << (A))
#define HV_HYPERCALL_FAST_BIT BIT(16)
#define BIT_ULL(a) (1ULL << (a))
#define HV_HYPERCALL_REP_COMP_1     BIT_ULL(32)
#define HV_PARTITION_ID_SELF        ((u64)-1)
#define HV_VP_INDEX_SELF        ((u32)-2)
#define HV_SMCCC_FUNC_NUMBER    1

#define HV_FUNC_ID SMCCC_FUNC_ID(SMCCC_YIELDING_CALL, SMCCC_64BIT_CALL,    \
    SMCCC_VENDOR_HYP_SERVICE_CALLS, (HV_SMCCC_FUNC_NUMBER))


void arm_hv_set_vreg(u32, u64);
void hv_get_vpreg_128(u32 , struct hv_get_vp_registers_output *);
u64 arm_hv_get_vreg(u32 msr); 
void arm_hv_set_vreg(u32 msr, u64 value)
{
    struct arm_smccc_res res;
	printf("inside arm_hv_set_vreg\n");
	int64_t hv_func_id;
	hv_func_id = SMCCC_FUNC_ID(SMCCC_YIELDING_CALL, SMCCC_64BIT_CALL,
							SMCCC_VENDOR_HYP_SERVICE_CALLS, (HV_SMCCC_FUNC_NUMBER));
	printf("inside arm_hv_set_vreg hv_func_id set hv_func_id %lu \n",hv_func_id);
    arm_smccc_hvc (hv_func_id,
        HVCALL_SET_VP_REGISTERS | HV_HYPERCALL_FAST_BIT |
            HV_HYPERCALL_REP_COMP_1,
        HV_PARTITION_ID_SELF,
        HV_VP_INDEX_SELF,
        msr,
        0,
        value,
        0,
        &res);
}



void hv_get_vpreg_128(u32 msr, struct hv_get_vp_registers_output *result)
{
    struct arm_smccc_1_2_regs args;
    struct arm_smccc_1_2_regs res;

    args.a0 = HV_FUNC_ID;
    args.a1 = HVCALL_GET_VP_REGISTERS | HV_HYPERCALL_FAST_BIT |
            HV_HYPERCALL_REP_COMP_1;
    args.a2 = HV_PARTITION_ID_SELF;
    args.a3 = HV_VP_INDEX_SELF;
    args.a4 = msr;

    /*
     * Use the SMCCC 1.2 interface because the results are in registers
     * beyond X0-X3.
     */
    arm_smccc_1_2_hvc(&args, &res);

    result->as64.low = res.a6;
    result->as64.high = res.a7;
}

u64 arm_hv_get_vreg(u32 msr)
{
    struct hv_get_vp_registers_output output;

    hv_get_vpreg_128(msr, &output);

    return output.as64.low;
}

uint64_t
hypercall_md(volatile void *hc_addr, uint64_t in_val,
    uint64_t in_paddr, uint64_t out_paddr)
{
	struct arm_smccc_res res;
	int64_t hv_func_id;
	hv_func_id = SMCCC_FUNC_ID(SMCCC_YIELDING_CALL, SMCCC_64BIT_CALL,
							SMCCC_VENDOR_HYP_SERVICE_CALLS, (HV_SMCCC_FUNC_NUMBER));
	arm_smccc_hvc (hv_func_id,
		in_val,
		in_paddr,
		out_paddr,
		0,
		0,
		0,
		0,
		&res);

	return (res.a0);
}
