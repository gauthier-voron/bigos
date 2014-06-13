#ifndef MSR_H
#define MSR_H


#include <stdlib.h>


#define MSR_EVT_UNHCCYC        (1 << 0)     /* unhalted core cycles */
#define MSR_EVT_INSTRET        (1 << 1)     /* intruction retired */
#define MSR_EVT_UNHRCYC        (1 << 2)     /* unhalted reference cycles */
#define MSR_EVT_LLCREFR        (1 << 3)     /* last level cache reference */
#define MSR_EVT_LLCMISS        (1 << 4)     /* last level cache miss */
#define MSR_EVT_BRINSRT        (1 << 5)     /* branch instruction retired */
#define MSR_EVT_BRMISRT        (1 << 6)     /* branch miss retired */


typedef struct perfcnt perfcnt_t;       /* performance counter opaque type */

struct msr_perfinfo
{
	size_t            perfcnt;     /* performance counter per processor */
	size_t            perfsze;            /* size (bit) of each counter */
	const perfcnt_t  *msraddr;             /* addresses of each counter */
	int               perfevt;          /* available performance events */
};

int probe_msr_perfinfo(struct msr_perfinfo *dest);


#define MSR_FLAG_USER          (1 << 16)    /* event is counted in user mode */
#define MSR_FLAG_OS            (1 << 17)  /* event is counted in kernel mode */
#define MSR_FLAG_EDGE          (1 << 18)     /* detect edge instead of state */
#define MSR_FLAG_PIN           (1 << 19)         /* toogle PMi in each event */
#define MSR_FLAG_INT           (1 << 20)          /* exception when overflow */
#define MSR_FLAG_EN            (1 << 22)      /* enable (overrided by func ) */
#define MSR_FLAG_INV           (1 << 23)    /* invert counter mask condition */

int enable_perfcnt(const perfcnt_t *perfcnt, int event, int flags, int cmask);

int disable_perfcnt(const perfcnt_t *perfcnt);

unsigned long read_perfcnt(const perfcnt_t *perfcnt);

#endif
