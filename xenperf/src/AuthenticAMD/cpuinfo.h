#include "asm.h"
#include "cpuinfo.h"


int __cpuinfo_family(int family, int efamily)
{
	if (family < 0x0f)
		return family;
	return efamily + family;
}

int __cpuinfo_model(int family, int model, int emodel)
{
	if (family < 0x0f)
		return model;
	return (emodel << 4) + model;
}
