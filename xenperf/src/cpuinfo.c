#define _GNU_SOURCE

#include <dirent.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include "asm.h"
#include "common.h"
#include "cpuinfo.h"

extern int sched_getcpu(void);


int __weak __cpuinfo_family(int family __unused, int efamily __unused)
{
	return -1;
}

int __weak __cpuinfo_model(int family __unused, int model __unused,
			   int emodel __unused)
{
	return -1;
}

int probe_cpuinfo(struct cpuinfo *dest)
{
	int family, efamily;
	int model, emodel;
	struct register_set regs;

	regs.eax = 0x80000000;
	cpuid(&regs, &regs);
	if (regs.eax < 1)
		return -1;

	regs.eax = 0x0;
	cpuid(&regs, &regs);
	((uint32_t *) dest->vendor)[0] = regs.ebx;
	((uint32_t *) dest->vendor)[1] = regs.edx;
	((uint32_t *) dest->vendor)[2] = regs.ecx;
	dest->vendor[12] = 0;

	regs.eax = 0x1;
	cpuid(&regs, &regs);
	dest->stepping = regs.eax & 0xf;

	family = (regs.eax >> 8) & 0xf ;
	efamily = (regs.eax >> 20) & 0xff;
	model = (regs.eax >> 4) & 0xf;
	emodel = (regs.eax >> 16) & 0xf;

	dest->family = __cpuinfo_family(family, efamily);
	dest->model = __cpuinfo_model(family, model, emodel);

	return 0;
}



/* Will not work under xen since it hides the NUMA topology */
__weak int probe_nodeinfo(struct coreinfo *dest)
{
	FILE *f;
	size_t size;
	unsigned int core, ret;
	char buffer[1024];
	unsigned long count = 0;
	const char *pattern = "/sys/devices/system/cpu/cpu%d/topology/core_id";

	for (core=0; core<dest->core_count; core++) {
		ret = snprintf(buffer, sizeof(buffer), pattern, core);
		if (ret >= sizeof(buffer))
			return -1;
		
		f = fopen(buffer, "r");
		if (f == NULL)
			return -1;

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);

		fread(buffer, sizeof(char), size, f);
		fclose(f);

		ret = atoi(buffer);
		if (core == dest->core_current)
			dest->node_current = ret;
		if (ret > count)
			count = count;
	}
	
	dest->node_count = count + 1;
	return 0;
}

int __weak probe_coreinfo(struct coreinfo *dest)
{
	DIR *dir = opendir("/sys/devices/system/cpu");
	struct dirent *dirent;

	dest->core_count = 1;
	dest->core_current = getcore();

	if (dir == NULL)
		return -1;

	dest->core_count = 0;
	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] != 'c')
			continue;
		if (dirent->d_name[1] != 'p')
			continue;
		if (dirent->d_name[2] != 'u')
			continue;
		if (dirent->d_name[3] < '0' || dirent->d_name[3] > '9')
			continue;
		dest->core_count++;
	}

	if (probe_nodeinfo(dest) < 0)
		return -1;

	return 0;
}


/* Will not work under xen since it hides the NUMA topology */
int __weak probe_coremap(unsigned int *coremap, size_t size)
{
	FILE *f;
	size_t len;
	unsigned int core, ret;
	char buffer[1024];
	const char *pattern = "/sys/devices/system/cpu/cpu%d/topology/core_id";

	for (core=0; core<size; core++) {
		ret = snprintf(buffer, sizeof(buffer), pattern, core);
		if (ret >= sizeof(buffer))
			return -1;
		
		f = fopen(buffer, "r");
		if (f == NULL)
			return -1;

		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);

		fread(buffer, sizeof(char), len, f);
		fclose(f);

		ret = atoi(buffer);
		coremap[core] = (unsigned int) ret;
	}
	
	return 0;
}


int __weak getcore(void)
{
	return sched_getcpu();
}

int __weak setcore(int core)
{
	int try = 30;
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(core, &mask);

	while (core != sched_getcpu()) {
		sched_setaffinity(0, sizeof(mask), &mask);
		if (try-- == 0)
			return -1;
	}

	return 0;
}
