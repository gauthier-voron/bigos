#ifndef PERFINFO_H
#define PERFINFO_H


#include <stdlib.h>


typedef struct perfcnt *perfcnt_t;

struct perfinfo
{
	int                hyper;
	size_t             count;
	const perfcnt_t   *list;
};


int probe_perfinfo(struct perfinfo *dest);


#define EVT_UNHCCYC            0     /* unhalted core cycles */
#define EVT_INSTRET            1     /* intruction retired */
#define EVT_UNHRCYC            2     /* unhalted reference cycles */
#define EVT_LLCREFR            3     /* last level cache reference */
#define EVT_LLCMISS            4     /* last level cache miss */
#define EVT_BRINSRT            5     /* branch instruction retired */
#define EVT_BRMISRT            6     /* branch miss retired */

#define EVT_NUMA_REQPATH       0xe9  /* NUMA CPU/IO request to Memory/IO */
#define UMASK_FROM_LOCAL      (1 << 7)
#define UMASK_FROM_REMOTE     (1 << 6)
#define UMASK_TO_LOCAL        (1 << 5)
#define UMASK_TO_REMOTE       (1 << 4)
#define UMASK_CPU_TO_MEM      (1 << 3)
#define UMASK_CPU_TO_IO       (1 << 2)
#define UMASK_IO_TO_MEM       (1 << 1)
#define UMASK_IO_TO_IO        (1 << 0)

int perfcnt_has_event(perfcnt_t perfcnt, unsigned long event);


int perfcnt_enable(perfcnt_t perfcnt, unsigned long event,
		   unsigned long umask, int core);

int perfcnt_disable(perfcnt_t perfcnt, int core);


unsigned long perfcnt_read(perfcnt_t perfcnt, int core);

int perfcnt_write(perfcnt_t perfcnt, unsigned long val, int core);


#endif
