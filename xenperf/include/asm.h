#ifndef ASM_H
#define ASM_H

#include <stdint.h>


struct register_set
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};


static inline void cpuid(struct register_set *out,
			 const struct register_set *in)
{
	asm volatile ("cpuid" : "=a" (out->eax), "=b" (out->ebx),
		      "=c" (out->ecx), "=d" (out->edx) : "a" (in->eax),
		      "b" (in->ebx), "c" (in->ecx), "d" (in->edx));
}


#endif
