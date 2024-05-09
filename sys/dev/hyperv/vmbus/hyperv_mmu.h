#ifndef _HYPERV_MMU_H_
#define _HYPERV_MMU_H_

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
	uint64_t gva_list[];
} __packed;

#endif
