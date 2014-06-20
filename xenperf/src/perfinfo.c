#include "asm.h"
#include "common.h"
#include "perfinfo.h"


size_t __weak __probe_perfcnt_vendor(const struct perfcnt **arr __unused,
				     size_t size __unused,
				     unsigned long evt __unused)
{
	return 0;
}

size_t __weak __probe_perfcnt_family(const struct perfcnt **arr __unused,
				     size_t size __unused,
				     unsigned long evt __unused)
{
	return 0;
}

size_t __weak __probe_perfcnt_model(const struct perfcnt **arr __unused,
				    size_t size __unused,
				    unsigned long evt __unused)
{
	return 0;
}


#define CPUID_FEATURE              (0x00000001)
#define CPUID_FEATURE_EDX_MSR      (1 << 5)

size_t probe_perfcnt(const struct perfcnt **arr, size_t size,
		     unsigned long evt)
{
	size_t len = 0;
	struct register_set regs;

	regs.eax = CPUID_FEATURE;
	cpuid(&regs, &regs);
	if (!(regs.edx & CPUID_FEATURE_EDX_MSR))
		goto out;

	len += __probe_perfcnt_model(arr, size - len, evt);
	len += __probe_perfcnt_family(arr, size - len, evt);
	len += __probe_perfcnt_vendor(arr, size - len, evt);

 out:
	return len;
}
