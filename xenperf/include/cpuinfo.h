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
	unsigned int    count;
	unsigned int    current;
};

int probe_coreinfo(struct coreinfo *dest);


int getcore(void);

int setcore(int core);


#endif
