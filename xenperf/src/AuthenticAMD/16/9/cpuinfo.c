#include "cpuinfo.h"


int probe_nodeinfo(struct coreinfo *dest)
{
	int i;
	unsigned long count = 0;
	static unsigned long nodemap[48] = {
		0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
		0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};

	dest->node_current = nodemap[dest->core_current];

	for (i=0; i<48; i++)
		count = (count < nodemap[i]) ? nodemap[i] : count;
	dest->node_count = count + 1;

	return 0;
}
