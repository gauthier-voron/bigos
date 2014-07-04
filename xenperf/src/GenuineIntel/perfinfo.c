#include "common.h"
#include "cpuinfo.h"
#include "hypercall.h"
#include "perfinfo.h"


#define CPUID_PMC                   (0x0000000a)
#define CPUID_PMC_EAX_COUNT(eax)    (((eax) >> 8) & 0xff)
#define CPUID_PMC_EAX_BITSZ(eax)    (((eax) >> 16) & 0xff)

#define LEGACY_PMC_FLAG_USR         (1 << 16)
#define LEGACY_PMC_FLAG_OS          (1 << 17)
#define LEGACY_PMC_FLAG_ENABLE      (1 << 22)


struct legacy_perfcnt
{
	struct perfcnt  super;
	unsigned long   select;
	unsigned long   counter;
	unsigned long   bitsize;
};



unsigned long __legacy_bitsize(const struct perfcnt *this)
{
	return ((const struct legacy_perfcnt *) this)->bitsize;
}

int __legacy_hasevt(const struct perfcnt *this __unused, unsigned long evt)
{
	switch (evt) {
	case EVENT_LLC:
	case EVENT_UNHCYC:
	case EVENT_INSRET:
	case EVENT_BRINST:
	case EVENT_BRMISS:
		return 1;
	default:
		return 0;
	}
}

int __legacy_enable(const struct perfcnt *this, unsigned long evt,
		    unsigned long umask, int core)
{
	struct register_set regs;
	unsigned long mask = LEGACY_PMC_FLAG_USR | LEGACY_PMC_FLAG_OS
		| LEGACY_PMC_FLAG_ENABLE | (evt & 0xff)
		| ((umask & 0xff) << 8);

	regs.ecx = ((struct legacy_perfcnt *) this)->select;
	regs.eax = mask;
	regs.edx = 0;
	
	if (setcore(core) < 0)
		return -1;
	if (hypercall_wrmsr(&regs, 0) < 0)
		return -1;
	regs.ecx = ((struct legacy_perfcnt *) this)->counter;
	return hypercall_demux(&regs);
}

int __legacy_disable(const struct perfcnt *this, int core)
{
	struct register_set regs;

	regs.ecx = ((struct legacy_perfcnt *) this)->select;
	regs.eax = 0;
	regs.edx = 0;
	
	if (setcore(core) < 0)
		return -1;
	if (hypercall_wrmsr(&regs, 0) < 0)
		return -1;
	regs.ecx = ((struct legacy_perfcnt *) this)->counter;
	return hypercall_remux(&regs);
}

unsigned long __legacy_read(const struct perfcnt *this, int core, int vdom)
{
	struct register_set regs;

	regs.ecx = ((struct legacy_perfcnt *) this)->counter;
	
	if (setcore(core) < 0)
		return -1;
	if (hypercall_rdmsr(&regs, vdom) < 0)
		return -1;

	return ((unsigned long) regs.edx << 32) | (regs.eax & 0xffffffff);
}

int __legacy_write(const struct perfcnt *this, unsigned long val, int core,
		   int vdom)
{
	struct register_set regs;

	regs.ecx = ((struct legacy_perfcnt *) this)->counter;
	regs.eax = val & 0xffffffff;
	regs.edx = val >> 32;
	
	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs, vdom);
}

#define LEGACY_SUPER { __legacy_bitsize, __legacy_hasevt, __legacy_enable, \
			__legacy_disable, __legacy_read, __legacy_write }

struct legacy_perfcnt legacy_perfcnt[] = {
	{ LEGACY_SUPER, 0x186, 0x0c1, 0 },
	{ LEGACY_SUPER, 0x187, 0x0c2, 0 },
	{ LEGACY_SUPER, 0x188, 0x0c3, 0 },
	{ LEGACY_SUPER, 0x189, 0x0c4, 0 }
};


size_t __probe_perfcnt_vendor(const struct perfcnt **arr, size_t size,
			      unsigned long evt)
{
	size_t i, len, count = 0;
	struct register_set regs;


	regs.eax = CPUID_PMC;
	cpuid(&regs, &regs);
	len = CPUID_PMC_EAX_COUNT(regs.eax);
	if (len > ARRAY_SIZE(legacy_perfcnt))
		len = ARRAY_SIZE(legacy_perfcnt);

	for (i=0; i<len; i++) {
		if (count >= size)
			break;
		if (!perfcnt_hasevt(&legacy_perfcnt[i].super, evt))
			continue;

		if (legacy_perfcnt[i].bitsize == 0)
			legacy_perfcnt[i].bitsize =
				CPUID_PMC_EAX_BITSZ(regs.eax);

		arr[count++] = &legacy_perfcnt[i].super;
	}

	return count;
}
