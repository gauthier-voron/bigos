#include "asm.h"
#include "common.h"
#include "perfinfo.h"


struct perfevt all_events[] = {
	{ EVENT_LLC, "last level cache", SCOPE_LLC,
	  (unsigned long []) { UMASK_LLC_MISS, UMASK_LLC_REF, 0 },
	  (const char *[]) { "cache miss", "cache reference", NULL }
	},

	{ EVENT_UNHCYC, "unhalted cpu cycle", SCOPE_UNHCYC,
	  (unsigned long []) { UMASK_UNHCYC_CORE, UMASK_UNHCYC_REF, 0 },
	  (const char *[]) { "core cycle", "reference cycle", NULL }
	},

	{ EVENT_INSRET, "instruction retired", SCOPE_INSRET,
	  (unsigned long []) { 0 },
	  (const char *[]) { NULL }
	},

	{ EVENT_BRINST, "branch instruction retired", SCOPE_BRINST,
	  (unsigned long []) { 0 },
	  (const char *[]) { NULL }
	},

	{ EVENT_BRMISS, "branch misspredition retired", SCOPE_BRMISS,
	  (unsigned long []) { 0 },
	  (const char *[]) { NULL }
	},

	{ EVENT_NBREQT, "northbridge transfert type", SCOPE_NBREQT,
	  (unsigned long []) { UMASK_NBREQT_IOIO, UMASK_NBREQT_IOMEM,
			       UMASK_NBREQT_CPUIO, UMASK_NBREQT_CPUMEM,
			       UMASK_NBREQT_REMLOC, UMASK_NBREQT_LOCREM,
			       UMASK_NBREQT_LOCLOC, 0 },
	  (const char *[]) { "IO to IO", "IO to Mem", "CPU to IO",
			     "CPU to Mem", "remote to local",
			     "local to remote", "local to local", NULL }
	},

	{ 0, NULL, 0, NULL, NULL }
};



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
