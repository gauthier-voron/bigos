#include "common.h"
#include "cpuinfo.h"
#include "hypercall.h"
#include "perfinfo.h"


#define LEGACY_PMC_FLAG_USR         (1 << 16)
#define LEGACY_PMC_FLAG_OS          (1 << 17)
#define LEGACY_PMC_FLAG_ENABLE      (1 << 22)
#define LEGACY_PMC_BITSIZE          (48)


struct legacy_perfcnt
{
	struct perfcnt  super;
	unsigned long   select;
	unsigned long   counter;
	unsigned long   bitmask;
};



unsigned long __legacy_bitsize(const struct perfcnt *this __unused)
{
	return LEGACY_PMC_BITSIZE;
}

int __legacy_hasevt(const struct perfcnt *this __unused, unsigned long evt)
{
	switch (evt) {
	case EVENT_INSRET:
	case EVENT_BRINST:
	case EVENT_BRMISS:
	case EVENT_NBREQT:
        case EVENT_DTLBL1MISS:
        case EVENT_DTLBL2MISS:
        case EVENT_ITLBL1MISS:
        case EVENT_ITLBL2MISS:
		return 1;
	default:
		return 0;
	}
}

int __legacy_enable(struct legacy_perfcnt *this, unsigned long evt,
		    unsigned long umask, int core)
{
	struct register_set regs;
	unsigned long mask = LEGACY_PMC_FLAG_USR | LEGACY_PMC_FLAG_OS
		| LEGACY_PMC_FLAG_ENABLE | (evt & 0xff)
		| ((umask & 0xff) << 8);
	int ret;

	regs.ecx = this->select;
	regs.eax = mask;
	regs.edx = 0;
	
	if (setcore(core) < 0)
		return -1;
	if ((ret = hypercall_wrmsr(&regs)) < 0)
		return ret;

	this->bitmask = mask;
	return ret;
}

int __legacy_disable(struct legacy_perfcnt *this, int core)
{
	struct register_set regs;
	int ret;

	regs.ecx = this->select;
	regs.eax = 0;
	regs.edx = 0;
	
	if (setcore(core) < 0)
		return -1;
	if ((ret = hypercall_wrmsr(&regs)) < 0)
		return ret;

	this->bitmask = 0;
	return ret;
}

unsigned long __legacy_read(const struct legacy_perfcnt *this, int core)
{
	struct register_set regs;
	unsigned long mask;

	regs.ecx = this->select;

	if (setcore(core) < 0)
		return -1;
	if (hypercall_rdmsr(&regs) < 0)
		return -1;

	mask = ((unsigned long) regs.edx << 32) | (regs.eax & 0xffffffff);
	if (this->bitmask != mask) {
		regs.eax = this->bitmask & 0xffffffff;
		regs.ecx = this->select;
		regs.edx = this->bitmask >> 32;
		if (setcore(core) < 0)
			return -1;
		hypercall_wrmsr(&regs);

		return -1;
	}

	regs.ecx = this->counter;
	
	if (setcore(core) < 0)
		return -1;
	if (hypercall_rdmsr(&regs) < 0)
		return -1;

	return ((unsigned long) regs.edx << 32) | (regs.eax & 0xffffffff);
}

int __legacy_write(const struct legacy_perfcnt *this, unsigned long val,
		   int core)
{
	struct register_set regs;

	regs.ecx = this->counter;
	regs.eax = val & 0xffffffff;
	regs.edx = val >> 32;
	
	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}

#define LEGACY_SUPER { (perfcnt_bitsize_t)  __legacy_bitsize,		\
			(perfcnt_hasevt_t)  __legacy_hasevt,		\
			(perfcnt_enable_t)  __legacy_enable,		\
			(perfcnt_disable_t) __legacy_disable,		\
			(perfcnt_read_t)    __legacy_read,		\
			(perfcnt_write_t)   __legacy_write }

struct legacy_perfcnt legacy_perfcnt[] = {
	{ LEGACY_SUPER, 0xc0010000, 0xc0010004, 0 },
	{ LEGACY_SUPER, 0xc0010001, 0xc0010005, 0 },
	{ LEGACY_SUPER, 0xc0010002, 0xc0010006, 0 },
	{ LEGACY_SUPER, 0xc0010003, 0xc0010007, 0 }
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
