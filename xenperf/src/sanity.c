#include <stdio.h>
#include <string.h>
#include "sanity.h"
#include "stringify.h"


int check_cpuinfo(struct cpuinfo *dest, int verbose)
{
	if (probe_cpuinfo(dest) < 0) {
		fprintf(stderr, "xenperf: cannot probe cpuinfo\n");
		return -1;
	}

	if (verbose)
		fprintf(stderr, "vendor %s\n", dest->vendor);

	if (strcmp(dest->vendor, stringify(VENDOR))) {
		fprintf(stderr, "xenperf: vendor mismatch, expected %s\n",
			stringify(VENDOR));
		fprintf(stderr, "xenperf: please recompile this program\n");
		return -1;
	}

	if (verbose)
		fprintf(stderr, "family %d\n", dest->family);

	if (dest->family != FAMILY) {
		fprintf(stderr, "xenperf: family mismatch, expected %d\n",
			FAMILY);
		fprintf(stderr, "xenperf: please recompile this program\n");
		return -1;
	}

	if (verbose)
		fprintf(stderr, "model %d\n", dest->model);

	if (dest->model != MODEL) {
		fprintf(stderr, "xenperf: model mismatch, expected %d\n",
			MODEL);
		fprintf(stderr, "xenperf: please recompile this program\n");
		return -1;
	}

	return 0;
}

int check_coreinfo(struct coreinfo *dest, int verbose)
{
	if (probe_coreinfo(dest) < 0) {
		fprintf(stderr, "xenperf: cannot probe coreinfo\n");
		return -1;
	}

	if (verbose) {
		fprintf(stderr, "core count is %d\n", dest->core_count);
		fprintf(stderr, "node count is %d\n", dest->node_count);
	}

	return 0;
}

int check_hypercall(int verbose)
{
	if (hypercall_channel_check() < 0) {
		if (verbose)
			fprintf(stderr, "hypercall disabled\n");

		fprintf(stderr, "xenperf: hypercall not enabled\n");
		fprintf(stderr, "xenperf: please be sure to be in xen dom0\n");
		fprintf(stderr, "xenperf: please be sure to patch your xen\n");
		return -1;
	}

	if (verbose)
		fprintf(stderr, "hypercall enabled\n");

	return 0;
}
