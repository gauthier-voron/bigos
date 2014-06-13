#include <stdint.h>
#include "asm.h"
#include "hypercall.h"
#include "msr.h"


#define MSR_ARCHPERF_CPUID_CMD   0x0000000a

#define MSR_MAX_PERFCOUNTER      128
#define MSR_FIRST_PMC            0xc1
#define MSR_FIRST_PERFEVTSEL     0x186


struct perfcnt
{
	void   *pmc;
	void   *perfevtsel;
};


int probe_msr_perfinfo(struct msr_perfinfo *dest)
{
	struct register_set regs;
	static struct perfcnt addr[MSR_MAX_PERFCOUNTER];
	size_t evtlen, i;

	regs.eax = MSR_ARCHPERF_CPUID_CMD;
	cpuid(&regs, &regs);

	dest->perfcnt = ((regs.eax >>  8) & 0x7f);
	dest->perfsze = ((regs.eax >> 16) & 0x7f);
	dest->msraddr = addr;
	
	evtlen = (regs.eax >> 24) & 0x7f;
	dest->perfevt = (~regs.ebx) & ((1 << evtlen) - 1);

	if (dest->perfcnt == 0 || addr[0].pmc != NULL)
		goto out;
	for (i=0; i<dest->perfcnt; i++) {
		addr[i].pmc = (void *) (MSR_FIRST_PMC + i);
		addr[i].perfevtsel = (void *) (MSR_FIRST_PERFEVTSEL + i);
	}
	
 out:
	return 0;
}


static long perfevt_umask_encode(int event)
{
	switch (event) {
	case MSR_EVT_UNHCCYC:  return 0x003c;
	case MSR_EVT_INSTRET:  return 0x00c0;
	case MSR_EVT_UNHRCYC:  return 0x013c;
	case MSR_EVT_LLCREFR:  return 0x4f2e;
	case MSR_EVT_LLCMISS:  return 0x412e;
	case MSR_EVT_BRINSRT:  return 0x00C4;
	case MSR_EVT_BRMISRT:  return 0x00c5;
	default:               return -1;
	}
}

int enable_perfcnt(const perfcnt_t *perfcnt, int event, int flags, int cmask)
{
	struct register_set regs;
	long umask_encode;

	if ((umask_encode = perfevt_umask_encode(event)) == -1)
		return -1;

	regs.ecx = (unsigned long) perfcnt->pmc;
	regs.edx = 0;
	regs.eax = 0;
	if (hypercall_wrmsr(&regs) < 0)
		return -1;

	regs.ecx = (unsigned long) perfcnt->perfevtsel;
	regs.edx = 0;
	regs.eax = umask_encode | (flags & ~((1 << 16) - 1)) | MSR_FLAG_EN
		| ((cmask & 0x7f) << 24);
	return hypercall_wrmsr(&regs);
}

int disable_perfcnt(const perfcnt_t *perfcnt)
{
	struct register_set regs;

	regs.ecx = (unsigned long) perfcnt->perfevtsel;
	regs.edx = 0;
	regs.eax = 0;

	return hypercall_wrmsr(&regs);
}

unsigned long read_perfcnt(const perfcnt_t *perfcnt)
{
	struct register_set regs;

	regs.ecx = (unsigned long) perfcnt->pmc;
	if (hypercall_rdmsr(&regs) < 0)
		return -1;

	return (((long) (regs.edx & 0xffffffff)) << 32)
		| (regs.eax & 0xffffffff);
}
