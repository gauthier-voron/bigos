#ifndef HYPERCALL_H
#define HYPERCALL_H


#include "asm.h"


int hypercall_channel_check(void);

int hypercall_wrmsr(struct register_set *regs, int vdom);

int hypercall_rdmsr(struct register_set *regs, int vdom);

int hypercall_domcount(unsigned int *count);

int hypercall_demux(struct register_set *regs);

int hypercall_remux(struct register_set *regs);


#endif
