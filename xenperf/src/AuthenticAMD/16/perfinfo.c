#include "cpuinfo.h"
#include "hypercall.h"
#include "perfinfo.h"


struct percnt_nb
{
	void      *select;
	void      *counter;
};

#define PERFCNT_TYPE_CORE   0
#define PERFCNT_TYPE_L2     1
#define PERFCNT_TYPE_NB     2

struct perfcnt
{
	int                        type;
	union
	{
		struct percnt_nb   nb;
	};
};


#define PERFCNT_NB_COUNT            4
#define PERFCNT_NB_SELECT_BASE      0xc0010240
#define PERFCNT_NB_COUNTER_BASE     0xc0010241

static int __probe_perfinfo_nb(struct perfcnt *arr, size_t size)
{
	size_t i;
	struct register_set regs;

	regs.eax = 0x80000001;
	cpuid(&regs, &regs);

	if (!(regs.ecx & (1 << 24)))
		return -1;

	for (i=0; i<PERFCNT_NB_COUNT && i<size; i++) {
		arr[i].type = PERFCNT_TYPE_NB;
		arr[i].nb.select = (void *) (PERFCNT_NB_SELECT_BASE + i*2);
		arr[i].nb.counter = (void *) (PERFCNT_NB_COUNTER_BASE + i*2);
	}

	return (int) i;
}


#define PERFCNT_MAX_COUNT    128

int probe_perfinfo(struct perfinfo *dest)
{
	int ret;
	size_t i;
	static size_t size = 0;
	static struct perfcnt perf[PERFCNT_MAX_COUNT];
	static perfcnt_t addr[PERFCNT_MAX_COUNT];

	dest->count = size;
	dest->list = addr;
	dest->hyper = hypercall_channel_check() >= 0;	

	if (size > 0)
		goto out;

	ret = __probe_perfinfo_nb(perf + size, PERFCNT_MAX_COUNT - size);
	if (ret > 0)
		size += (size_t) ret;

	dest->count = size;
	for (i=0; i<size; i++)
		addr[i] = perf + i;
 out:
	return 0;
}


int perfcnt_has_event(perfcnt_t perfcnt, unsigned long event)
{
	if (perfcnt->type != PERFCNT_TYPE_NB)
		return 0;
	if (event != EVT_NUMA_REQPATH)
		return 0;
	return 1;
}



static unsigned long perfcnt_nb_encode(unsigned long event,
				       unsigned long umask)
{
	unsigned long encode = 0;
	
	encode |= (event & 0xff);
	encode |= (event & 0xf00) << 32;
	encode |= (umask & 0xff) << 8;
	encode |= 1 << 22;

	return encode;
}


int perfcnt_enable(perfcnt_t perfcnt, unsigned long event,
		   unsigned long umask, int core)
{
	struct register_set regs;
	unsigned long encode;

	if (perfcnt->type == PERFCNT_TYPE_NB) {
		encode = perfcnt_nb_encode(event, umask);
		regs.ecx = (unsigned long) perfcnt->nb.select;
		regs.eax = encode & 0xffffffff;
		regs.edx = encode >> 32;
	} else {
		return -1;
	}

	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}

int perfcnt_disable(perfcnt_t perfcnt, int core)
{
	struct register_set regs;

	if (perfcnt->type == PERFCNT_TYPE_NB)
		regs.ecx = (unsigned long) perfcnt->nb.select;
	else
		return -1;

	regs.edx = 0;
	regs.eax = 0;

	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}


unsigned long perfcnt_read(perfcnt_t perfcnt, int core)
{
	struct register_set regs;
	
	if (perfcnt->type == PERFCNT_TYPE_NB)
		regs.ecx = (unsigned long) perfcnt->nb.counter;
	else
		return (unsigned long) -1;

	if (setcore(core) < 0)
		return -1;
	if (hypercall_rdmsr(&regs) < 0)
		return -1;

	return (((unsigned long) (regs.edx & 0xffffffff)) << 32)
		| (regs.eax & 0xffffffff);
}

int perfcnt_write(perfcnt_t perfcnt, unsigned long val, int core)
{
	struct register_set regs;

	if (perfcnt->type == PERFCNT_TYPE_NB)
		regs.ecx = (unsigned long) perfcnt->nb.counter;
	else
		return -1;

	regs.eax = val & 0xffffffff;
	regs.eax = val >> 32;

	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);

}
