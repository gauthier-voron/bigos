#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "asm.h"
#include "hypercall.h"
#include "msr.h"
#include "vendor.h"



int main(void)
{
	char buffer[128];
	struct msr_perfinfo perfinfo;


	if (check_vendor() < 0) {
		if (probe_vendor(buffer, sizeof(buffer)) < 0) {
			fprintf(stderr, "unable to probe vendor id\n");
			return EXIT_FAILURE;
		}

		fprintf(stderr, "incompatible vendor id: '%s'\n", buffer);
		fprintf(stderr, "expected '%s', please recompile\n",
			expected_vendor());
		return EXIT_FAILURE;
	}


	if (probe_msr_perfinfo(&perfinfo) < 0) {
		fprintf(stderr, "cannot obtain performance counter info\n");
		return EXIT_FAILURE;
	}

	printf("performance counter detected\n");
	printf("  count per core:   %lu\n", perfinfo.perfcnt);
	printf("  size in bit:      %lu\n", perfinfo.perfsze);


	if (getuid() != 0) {
		fprintf(stderr, "only root can perform hypercall\n");
		return EXIT_FAILURE;
	}

	if (hypercall_channel_check() < 0) {
		fprintf(stderr, "cannot use the xen hypercall channel\n");
		fprintf(stderr, "please apply the xenperf patch to xen\n");
		return EXIT_FAILURE;
	}

	printf("detected xen hypercall channel successfully\n");

	printf("starting performance counting\n");
	enable_perfcnt(perfinfo.msraddr, MSR_EVT_LLCMISS,
		       MSR_FLAG_USER | MSR_FLAG_OS, 0);

	printf("performance counter = %lu\n",
	       read_perfcnt(perfinfo.msraddr));

	printf("stopping performance counting\n");
	disable_perfcnt(perfinfo.msraddr);

	
	return EXIT_SUCCESS;
}
