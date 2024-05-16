
#ifndef _HYPERV_MMU_H_
#define _HYPERV_MMU_H_

#include "vmbus_var.h"

#define HV_VCPUS_PER_SPARSE_BANK (64)
#define HV_MAX_SPARSE_VCPU_BANKS (64)

#define GENMASK_ULL(h, l)  (((~0ULL) >> (64 - (h) - 1)) & ((~0ULL) << (l)))

struct hyperv_tlb_flush {
	uint64_t address_space;
	uint64_t flags;
	uint64_t processor_mask;
	uint64_t gva_list[];
}__packed;

struct hv_vpset {
	uint64_t format;
	uint64_t valid_bank_mask;
	uint64_t bank_contents[];
} __packed;

struct hv_tlb_flush_ex {
	uint64_t address_space;
	uint64_t flags;
	struct hv_vpset hv_vp_set;
	//uint64_t gva_list[];
} __packed;

#endif
