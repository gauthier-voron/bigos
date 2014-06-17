#include "cpuinfo.h"
#include "hypercall.h"
#include "perfinfo.h"


struct perfcnt
{
	void           *pmc;
	void           *perfevtsel;
	unsigned int    avail;
};


int probe_perfinfo(struct perfinfo *dest)
{
	struct register_set regs;
	static struct perfcnt perf[128];
	static perfcnt_t addr[128];
	unsigned int avail;
	size_t evtlen, i;

	regs.eax = 0xa;
	cpuid(&regs, &regs);

	dest->count = ((regs.eax >>  8) & 0x7f);
	dest->list = addr;
	dest->hyper = hypercall_channel_check() >= 0;	

	if (dest->count == 0 || addr[0] != NULL)
		goto out;

	evtlen = (regs.eax >> 24) & 0x7f;
	avail = (~regs.ebx) & ((1 << evtlen) - 1);

	for (i=0; i<dest->count; i++) {
		addr[i] = &perf[i];
		perf[i].pmc = (void *) (0xc1 + i);
		perf[i].perfevtsel = (void *) (0x186 + i);
		perf[i].avail = avail;
	}
	
 out:
	return 0;
}


int perfcnt_has_event(perfcnt_t perfcnt, unsigned long event)
{
	if (event > EVT_BRMISRT)
		return 0;
	return perfcnt->avail & (1 << event);
}


static long perfevt_umask_encode(int event)
{
	switch (event) {
	case EVT_UNHCCYC:  return 0x003c;
	case EVT_INSTRET:  return 0x00c0;
	case EVT_UNHRCYC:  return 0x013c;
	case EVT_LLCREFR:  return 0x4f2e;
	case EVT_LLCMISS:  return 0x412e;
	case EVT_BRINSRT:  return 0x00C4;
	case EVT_BRMISRT:  return 0x00c5;
	default:               return -1;
	}
}


#define FLAG_USER          (1 << 16)        /* event is counted in user mode */
#define FLAG_OS            (1 << 17)      /* event is counted in kernel mode */
#define FLAG_EDGE          (1 << 18)         /* detect edge instead of state */
#define FLAG_PIN           (1 << 19)             /* toogle PMi in each event */
#define FLAG_INT           (1 << 20)              /* exception when overflow */
#define FLAG_EN            (1 << 22)                       /* enable counter */
#define FLAG_INV           (1 << 23)        /* invert counter mask condition */

int perfcnt_enable(perfcnt_t perfcnt, unsigned long event,
		   unsigned long umask __attribute__((unused)), int core)
{
	struct register_set regs;
	long umask_encode;

	if ((umask_encode = perfevt_umask_encode(event)) == -1)
		return -1;

	regs.ecx = (unsigned long) perfcnt->perfevtsel;
	regs.edx = 0;
	regs.eax = umask_encode | FLAG_USER | FLAG_OS | FLAG_EN;

	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
 }

int perfcnt_disable(perfcnt_t perfcnt, int core)
{
	struct register_set regs;

	regs.ecx = (unsigned long) perfcnt->perfevtsel;
	regs.edx = 0;
	regs.eax = 0;

	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}


unsigned long perfcnt_read(perfcnt_t perfcnt, int core)
{
	struct register_set regs;

	regs.ecx = (unsigned long) perfcnt->pmc;

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

	regs.eax = val & 0xffffffff;
	regs.ecx = (unsigned long) perfcnt->pmc;
	regs.eax = val >> 32;

	if (setcore(core) < 0)
		return -1;
	return hypercall_wrmsr(&regs);
}
