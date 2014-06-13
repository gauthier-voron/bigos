#ifndef VENDOR_H
#define VENDOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"


#define VENDOR_CPUID_CMD   0x00000000

#define VENDOR_OLDAMD      "AMDisbetter!"
#define VENDOR_AMD         "AuthenticAMD"
#define VENDOR_INTEL       "GenuineIntel"


const char *expected_vendor(void);


static inline int probe_vendor(char *dest, size_t size)
{
	struct register_set regs;

	if (size < 12)
		return -1;

	regs.eax = VENDOR_CPUID_CMD;
	cpuid(&regs, &regs);

	((uint32_t *) dest)[0] = regs.ebx;
	((uint32_t *) dest)[1] = regs.edx;
	((uint32_t *) dest)[2] = regs.ecx;

	if (size >= 13)
		dest[12] = 0;

	return 13;
}

static inline int check_vendor(void)
{
	char buf[13];

	if (probe_vendor(buf, sizeof(buf)) < 0)
		return -1;
	if (strcmp(buf, expected_vendor()))
		return -1;
	return 0;
}


#endif
