#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "asm.h"
#include "cpuinfo.h"
#include "stringify.h"
#include "perfinfo.h"

#define INITIAL_NANOSLEEP      (1 << 16)
#define MAXIMAL_SECSLEEP       (1)
#define GIGA                   (1000000000)
#define COUNTER_CEIL           (1 << 28)


struct cpuinfo cpuinfo;
struct coreinfo coreinfo;
struct perfinfo perfinfo;


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

	printf("core count         %d\n", coreinfo.count);
	printf("current core       %d\n", coreinfo.current);

	return 0;
}

static int check_perfinfo(void)
{
	if (probe_perfinfo(&perfinfo) < 0) {
		fprintf(stderr, "xenperf: cannot probe perfinfo\n");
		return -1;
	}

	printf("hypercall          %d\n", perfinfo.hyper);
	printf("perfcnt count      %lu\n", perfinfo.count);

	if (!perfinfo.hyper) {
		fprintf(stderr, "xenperf: hypercall support not enabled\n");
		fprintf(stderr, "xenperf: please patch your xen\n");
		return -1;
	}

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


static int run_counters(unsigned long evt, unsigned long umask)
{
	int updown = 0;
	unsigned int i, cnt;
	unsigned long pmc;
	unsigned long sec = 0, nsec = 0;
	struct timespec ts, tc;

	for (cnt=0; cnt<perfinfo.count; cnt++)
		if (perfcnt_has_event(perfinfo.list[cnt], evt))
			break;
	if (cnt == perfinfo.count) {
		fprintf(stderr, "xenperf: unsupported event\n");
		return -1;
	}

	for (i=0; i<coreinfo.count; i++) {
		if (perfcnt_write(perfinfo.list[cnt], 0, i) < 0)
			return -1;
		if (perfcnt_enable(perfinfo.list[cnt], evt, umask, i) < 0)
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
		for (i=0; i<coreinfo.count; i++) {
			pmc = perfcnt_read(perfinfo.list[cnt], i);

			printf("\t%10lu", pmc);

			if (pmc > (COUNTER_CEIL / 2))
				updown++;                     /* loop faster */
			else if (pmc < (COUNTER_CEIL / 4))
				updown--;                     /* loop slower */

			perfcnt_write(perfinfo.list[cnt], 0, i);
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

	for (i=0; i<coreinfo.count; i++)
		if (perfcnt_disable(perfinfo.list[cnt], i) < 0)
			return -1;

	return 0;
}


int main(void)
{
	if (check_cpuinfo() < 0)
		return EXIT_FAILURE;
	if (check_coreinfo() < 0)
		return EXIT_FAILURE;
	if (check_perfinfo() < 0)
		return EXIT_FAILURE;

	if (configure_sighandler() < 0) {
		fprintf(stderr, "xenperf: cannot set signal handler up\n");
		perror("xenperf");
		return EXIT_FAILURE;
	}

	if (run_counters(EVT_UNHCCYC, 0) < 0)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}
