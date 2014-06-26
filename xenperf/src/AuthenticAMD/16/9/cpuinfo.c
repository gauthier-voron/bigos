#include <stdlib.h>
#include "common.h"
#include "cpuinfo.h"


static unsigned int coremap[48] = {
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7
};


int probe_nodeinfo(struct coreinfo *dest)
{
	unsigned int i;
	unsigned long count = 0;

	dest->node_current = coremap[dest->core_current];

	for (i=0; i<ARRAY_SIZE(coremap); i++)
		count = (count < coremap[i]) ? coremap[i] : count;
	dest->node_count = count + 1;

	return 0;
}

int probe_coremap(unsigned int *map, size_t size)
{
	size_t i;

	if (size > ARRAY_SIZE(coremap))
		return -1;
	for (i=0; i<size; i++)
		map[i] = coremap[i];

	return 0;
}
