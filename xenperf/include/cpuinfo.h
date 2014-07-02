#ifndef CPUINFO_H
#define CPUINFO_H


#include <stdlib.h>


struct cpuinfo
{
	char            vendor[13];
	unsigned int    stepping;
	unsigned int    family;
	unsigned int    model;
};

int probe_cpuinfo(struct cpuinfo *dest);


struct coreinfo
{
	unsigned int  core_count;
	unsigned int  core_current;
	unsigned int  node_count;
	unsigned int  node_current;
	unsigned int  vdom_count;
};

int probe_coreinfo(struct coreinfo *dest);

int probe_coremap(unsigned int *coremap, size_t size);


int getcore(void);

int setcore(int core);


#endif
