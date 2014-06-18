#ifndef CPUINFO_H
#define CPUINFO_H


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
};

int probe_coreinfo(struct coreinfo *dest);


int getcore(void);

int setcore(int core);


#endif
