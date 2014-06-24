#ifndef SANITY_H
#define SANITY_H


#include "cpuinfo.h"
#include "hypercall.h"


int check_cpuinfo(struct cpuinfo *dest, int verbose);

int check_coreinfo(struct coreinfo *dest, int verbose);

int check_hypercall(int verbose);


#endif
