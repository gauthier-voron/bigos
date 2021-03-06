#ifndef PERFINFO_H
#define PERFINFO_H


#include <stdlib.h>


#define SCOPE_CORE                (0)
#define SCOPE_CPU                 (1)
#define SCOPE_NODE                (2)


#define EVENT_LLC                 (0x2e)  /* last level cache */
#define UMASK_LLC_MISS            (0x41)  /*   miss */
#define UMASK_LLC_REF             (0x4f)  /*   reference */
#define SCOPE_LLC                 SCOPE_CORE

#define EVENT_UNHCYC              (0x3c)  /* unhalted cycle */
#define UMASK_UNHCYC_CORE         (0x00)  /*   core */
#define UMASK_UNHCYC_REF          (0x01)  /*   reference */
#define SCOPE_UNHCYC              SCOPE_CORE

#define EVENT_INSRET              (0xc0)  /* instruction retired */
#define SCOPE_INSRET              SCOPE_CORE

#define EVENT_BRINST              (0xc4)  /* branch instruction retired */
#define SCOPE_BRINST              SCOPE_CORE

#define EVENT_BRMISS              (0xc5)  /* branch miss retired */
#define SCOPE_BRMISS              SCOPE_CORE

#define EVENT_NBREQT              (0xe9)  /* northbridge requests type */
#define UMASK_NBREQT_IOIO         (0x01)  /*   IO to IO */
#define UMASK_NBREQT_IOMEM        (0x02)  /*   IO to Mem */
#define UMASK_NBREQT_CPUIO        (0x04)  /*   CPU to IO */
#define UMASK_NBREQT_CPUMEM       (0x08)  /*   CPU to Mem */
#define UMASK_NBREQT_REMLOC       (0x60)  /*   remote to local */
#define UMASK_NBREQT_LOCREM       (0x90)  /*   local to remote */
#define UMASK_NBREQT_LOCLOC       (0xa0)  /*   local to local */
#define SCOPE_NBREQT              SCOPE_NODE



struct perfevt
{
	unsigned long    event;
	const char      *event_desc;
	unsigned long    scope;
	unsigned long   *umasks;
	const char     **umasks_desc;
};

extern struct perfevt all_events[];


struct perfcnt;

typedef unsigned long  (*perfcnt_bitsize_t)(const struct perfcnt *this);
typedef int            (*perfcnt_hasevt_t)(const struct perfcnt *this,
					   unsigned long evt);

typedef int            (*perfcnt_enable_t)(const struct perfcnt *this,
					   unsigned long evt,
					   unsigned long umask, int core);
typedef int            (*perfcnt_disable_t)(const struct perfcnt *this,
					    int core);
typedef unsigned long  (*perfcnt_read_t)(const struct perfcnt *this, int core);
typedef int            (*perfcnt_write_t)(const struct perfcnt *this,
					  unsigned long val, int core);

struct perfcnt
{
	perfcnt_bitsize_t   bitsize;
	perfcnt_hasevt_t    hasevt;
	perfcnt_enable_t    enable;
	perfcnt_disable_t   disable;
	perfcnt_read_t      read;
	perfcnt_write_t     write;
};


size_t probe_perfcnt(const struct perfcnt **arr, size_t size,
		     unsigned long evt);


static inline unsigned long perfcnt_bitsize(const struct perfcnt *this)
{
	return this->bitsize(this);
}

static inline int perfcnt_hasevt(const struct perfcnt *this, unsigned long evt)
{
	return this->hasevt(this, evt);
}


static inline int perfcnt_enable(const struct perfcnt *this, unsigned long evt,
				 unsigned long umask, int core)
{
	return this->enable(this, evt, umask, core);
}

static inline int perfcnt_disable(const struct perfcnt *this, int core)
{
	return this->disable(this, core);
}


static inline unsigned long perfcnt_read(const struct perfcnt *this, int core)
{
	return this->read(this, core);
}

static inline int perfcnt_write(const struct perfcnt *this, unsigned long val,
				int core)
{
	return this->write(this, val, core);
}


typedef struct perfcnt *perfcnt_t;

struct perfinfo
{
	int                hyper;
	size_t             count;
	const perfcnt_t   *list;
};


int probe_perfinfo(struct perfinfo *dest);


#endif
