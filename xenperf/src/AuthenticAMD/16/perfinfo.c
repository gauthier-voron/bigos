#include "common.h"
#include "cpuinfo.h"
#include "hypercall.h"
#include "perfinfo.h"


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
	case EVENT_INSRET:
	case EVENT_BRINST:
	case EVENT_BRMISS:
	case EVENT_NBREQT:
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

	return hypercall_wrmsr(&regs);
}

int __legacy_disable(const struct perfcnt *this, int core)
{
	struct register_set regs;

	regs.ecx = ((struct legacy_perfcnt *) this)->select;
	regs.eax = 0;
	regs.edx = 0;
	
	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}

unsigned long __legacy_read(const struct perfcnt *this, int core)
{
	struct register_set regs;

	regs.ecx = ((struct legacy_perfcnt *) this)->counter;
	
	if (setcore(core) < 0)
		return -1;
	if (hypercall_rdmsr(&regs) < 0)
		return -1;

	return ((unsigned long) regs.edx << 32) | (regs.eax & 0xffffffff);
}

int __legacy_write(const struct perfcnt *this, unsigned long val, int core)
{
	struct register_set regs;

	regs.ecx = ((struct legacy_perfcnt *) this)->counter;
	regs.eax = val & 0xffffffff;
	regs.edx = val >> 32;
	
	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}

#define LEGACY_SUPER { __legacy_bitsize, __legacy_hasevt, __legacy_enable, \
			__legacy_disable, __legacy_read, __legacy_write }

struct legacy_perfcnt legacy_perfcnt[] = {
	{ LEGACY_SUPER, 0xc0010000, 0xc0010004, 48 },
	{ LEGACY_SUPER, 0xc0010001, 0xc0010005, 48 },
	{ LEGACY_SUPER, 0xc0010002, 0xc0010006, 48 },
	{ LEGACY_SUPER, 0xc0010003, 0xc0010007, 48 }
};


size_t __probe_perfcnt_vendor(const struct perfcnt **arr, size_t size,
			      unsigned long evt)
{
	size_t i, len = 4, count = 0;

	for (i=0; i<len; i++) {
		if (count >= size)
			break;
		if (!perfcnt_hasevt(&legacy_perfcnt[i].super, evt))
			continue;

		arr[count++] = &legacy_perfcnt[i].super;
	}

	return count;
}
