#define _GNU_SOURCE

#include <dirent.h>
#include <sched.h>
#include <stdlib.h>
#include "asm.h"
#include "cpuinfo.h"

extern int sched_getcpu(void);


int __attribute__((weak)) __cpuinfo_family(int family __attribute__((unused)),
					   int efamily __attribute__((unused)))
{
	return -1;
}

int __attribute__((weak)) __cpuinfo_model(int family __attribute__((unused)),
					  int model __attribute__((unused)),
					  int emodel __attribute__((unused)))
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




int __attribute__((weak)) probe_coreinfo(struct coreinfo *dest)
{
	DIR *dir = opendir("/sys/devices/system/cpu");
	struct dirent *dirent;

	dest->count = -1;
	dest->current = getcore();

	if (dir == NULL)
		return -1;

	dest->count = 0;
	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] != 'c')
			continue;
		if (dirent->d_name[1] != 'p')
			continue;
		if (dirent->d_name[2] != 'u')
			continue;
		if (dirent->d_name[3] < '0' || dirent->d_name[3] > '9')
			continue;
		dest->count++;
	}

	return 0;
}


int __attribute__((weak)) getcore(void)
{
	return sched_getcpu();
}

int __attribute__((weak)) setcore(int core)
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
