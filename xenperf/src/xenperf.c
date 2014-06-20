#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "asm.h"
#include "common.h"
#include "cpuinfo.h"
#include "hypercall.h"
#include "stringify.h"
#include "perfinfo.h"

#define INITIAL_NANOSLEEP      (1 << 16)
#define MAXIMAL_SECSLEEP       (1)
#define GIGA                   (1000000000)
#define COUNTER_CEIL           (1 << 28)

#define EVENT                  (EVENT_NBREQT)
#define UMASK                  (UMASK_NBREQT_CPUMEM | UMASK_NBREQT_LOCLOC)
/* #define EVENT                  (EVENT_LLC) */
/* #define UMASK                  (UMASK_LLC_MISS) */


struct cpuinfo cpuinfo;
struct coreinfo coreinfo;


static int check_cpuinfo(void)
{
	if (probe_cpuinfo(&cpuinfo) < 0) {
		fprintf(stderr, "xenperf: cannot probe cpuinfo\n");
		return -1;
	}

	printf("vendor             %s\n", cpuinfo.vendor);

	if (strcmp(cpuinfo.vendor, stringify(VENDOR))) {
		fprintf(stderr, "xenperf: vendor mismatch, expected %s\n",
			stringify(VENDOR));
		fprintf(stderr, "xenperf: please recompile this program\n");
		return -1;
	}

	printf("family             %d\n", cpuinfo.family);

	if (cpuinfo.family != FAMILY) {
		fprintf(stderr, "xenperf: family mismatch, expected %d\n",
			FAMILY);
		fprintf(stderr, "xenperf: please recompile this program\n");
		return -1;
	}

	printf("model              %d\n", cpuinfo.model);

	if (cpuinfo.model != MODEL) {
		fprintf(stderr, "xenperf: model mismatch, expected %d\n",
			MODEL);
		fprintf(stderr, "xenperf: please recompile this program\n");
		return -1;
	}

	return 0;
}

static int check_coreinfo(void)
{
	if (probe_coreinfo(&coreinfo) < 0) {
		fprintf(stderr, "xenperf: cannot probe coreinfo\n");
		return -1;
	}

	printf("core count         %d\n", coreinfo.core_count);
	printf("current core       %d\n", coreinfo.core_current);
	printf("node count         %d\n", coreinfo.node_count);
	printf("current node       %d\n", coreinfo.node_current);

	return 0;
}

static size_t check_perfcnt(const struct perfcnt **arr, size_t size)
{
	size = probe_perfcnt(arr, size, EVENT);

	if (size == 0) {
		fprintf(stderr, "xenperf: no available performance counter\n");
		return 0;
	}

	printf("found counters     %lu\n", size);

	return size;
}

static int check_hypercall(void)
{
	if (hypercall_channel_check() < 0) {
		fprintf(stderr, "xenperf: hypercall not enabled\n");
		fprintf(stderr, "xenperf: please be sure to be in xen dom0\n");
		fprintf(stderr, "xenperf: please be sure to patch your xen\n");
		return -1;
	}

	printf("hypercall          1\n");

	return 0;
}


static int __signal_received = 0;

static void __sighandler(int signum __attribute__((unused)))
{
	__signal_received = 1;
}

static int configure_sighandler(void)
{
	struct sigaction sa;

	sa.sa_handler = __sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) < 0)
		return -1;
	if (sigaction(SIGTERM, &sa, NULL) < 0)
		return -1;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		return -1;

	return 0;
}

static int check_continue(void)
{
	return __signal_received == 0;
}


static int run_counters(const struct perfcnt *perfcnt, unsigned long event,
			unsigned long umask, unsigned int *cores,
			size_t size)
{
	int updown = 0;
	unsigned int i;
	unsigned long pmc;
	unsigned long sec = 0, nsec = 0;
	struct timespec ts, tc;

	for (i=0; i<size; i++) {
		if (perfcnt_write(perfcnt, 0, cores[i]) < 0)
			return -1;
		if (perfcnt_enable(perfcnt, event, umask, cores[i]) < 0)
			return -1;
	}

	ts.tv_sec = INITIAL_NANOSLEEP / GIGA;
	ts.tv_nsec = INITIAL_NANOSLEEP % GIGA;
	tc = ts;

	while (check_continue()) {
		if (nanosleep(&tc, &tc) < 0)
			continue;
		
		nsec += ts.tv_nsec;
		sec += ts.tv_sec;
		while (nsec >= GIGA) {
			nsec -= GIGA;
			sec++;
		}

		printf("%lu.%09lu", sec, nsec);
		for (i=0; i<size; i++) {
			pmc = perfcnt_read(perfcnt, cores[i]);

			printf("\t%10lu", pmc);

			if (pmc > (COUNTER_CEIL / 2))
				updown++;                     /* loop faster */
			else if (pmc < (COUNTER_CEIL / 4))
				updown--;                     /* loop slower */

			perfcnt_write(perfcnt, 0, cores[i]);
		}
		printf("\n");
			
		if (updown > 0) {
			ts.tv_nsec = ((ts.tv_sec * GIGA) / 2) % GIGA
				+ ts.tv_nsec / 2;
			ts.tv_sec /= 2;
		} else if (updown < 0) {
			ts.tv_sec = ts.tv_sec * 2 + ((ts.tv_nsec * 2) / GIGA);
			ts.tv_nsec = (ts.tv_nsec * 2) % GIGA;

			if (ts.tv_sec >= MAXIMAL_SECSLEEP) {
				ts.tv_sec = MAXIMAL_SECSLEEP;
				ts.tv_nsec = 0;
			}
		}

		updown = 0;
		tc = ts;
	}

	for (i=0; i<size; i++)
		if (perfcnt_disable(perfcnt, cores[i]) < 0)
			return -1;

	return 0;
}


int main(void)
{
	const struct perfcnt *perfcnt;
	unsigned int *coremap;
	unsigned int *cores;
	size_t i, j;

	if (check_cpuinfo() < 0)
		return EXIT_FAILURE;
	if (check_coreinfo() < 0)
		return EXIT_FAILURE;
	if (check_perfcnt(&perfcnt, 1) == 0)
		return EXIT_FAILURE;
	if (check_hypercall() < 0)
		return EXIT_FAILURE;

	if (configure_sighandler() < 0) {
		fprintf(stderr, "xenperf: cannot set signal handler up\n");
		perror("xenperf");
		return EXIT_FAILURE;
	}

	coremap = alloca(sizeof(unsigned int) * coreinfo.core_count);
	probe_coremap(coremap, coreinfo.core_count);

	/* cores = alloca(sizeof(unsigned int) * coreinfo.core_count); */
	/* for (i=0; i<coreinfo.core_count; i++) */
	/* 	cores[i] = i; */

	cores = alloca(sizeof(unsigned int) * coreinfo.node_count);
	for (i=0, j=0; i<coreinfo.core_count; i++)
		if (coremap[i] == j)
			cores[j++] = i;

	if (run_counters(perfcnt, EVENT, UMASK,
			 cores, coreinfo.node_count) < 0)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}
