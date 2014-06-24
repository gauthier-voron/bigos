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
#include "sanity.h"
#include "perfinfo.h"

#define INITIAL_NANOSLEEP      (50000)
#define MAXIMAL_SECSLEEP       (1)
#define GIGA                   (1000000000)
#define COUNTER_CEIL           (1 << 28)


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


static void display_events(void)
{
	const struct perfcnt *cnt;
	struct perfevt *index = all_events;
	unsigned long *umasks;
	const char **umasks_desc;
	const char *title;

	while (index->event) {
		if (!probe_perfcnt(&cnt, 1, index->event))
			goto next;
		
		printf("event: 0x%02lx\t%s\n", index->event,
		       index->event_desc);

		umasks = index->umasks;
		umasks_desc = index->umasks_desc;
		title = "umask:";
		
		while (*umasks_desc) {
			printf("%s  0x%02lx\t%s\n", title, *umasks,
			       *umasks_desc);
			title = "      ";
			umasks++;
			umasks_desc++;
		}

		printf("scope:     \t");
		switch (index->scope) {
		case SCOPE_CORE:  printf("core\n");  break;
		case SCOPE_CPU:   printf("cpu\n"); break;
		case SCOPE_NODE:  printf("node\n");  break;
		default:          printf("error\n"); break;
		}

		printf("\n");

	next:
		index++;
	}
}

static struct perfevt *event_entry(unsigned long event)
{
	struct perfevt *index = all_events;
	const struct perfcnt *cnt;

	while (index->event) {
		if (!probe_perfcnt(&cnt, 1, index->event))
			goto next;
		if (index->event == event)
			return index;
	next:
		index++;
	}

	return NULL;
}


static int parse_event(const char *argv, unsigned long *event,
		       unsigned long *umask)
{
	char *end;

	if (argv[0] == '0' && argv[1] == 'x')
		argv += 2;
	*event = (unsigned) strtol(argv, &end, 16);
	*umask = 0;

	if (*end == '\0')
		return 0;
	if (*end != ':')
		return -1;
	argv = end + 1;

	if (argv[0] == '0' && argv[1] == 'x')
		argv += 2;
	*umask = (unsigned) strtol(argv, &end, 16);

	if (*end == '\0')
		return 0;
	return -1;
}

static int parse_events(int argc, const char **argv, unsigned long *events,
			unsigned long *umasks)
{
	struct perfevt *perfevt;
	int i;

	for (i=1; i<argc; i++) {
		if (parse_event(argv[i], &events[i-1], &umasks[i-1]) < 0) {
			fprintf(stderr, "xenperf: invalid operand: '%s'\n",
				argv[i]);
			return -1;
		}

		perfevt = event_entry(events[i-1]);
		if (perfevt == NULL) {
			fprintf(stderr, "xenperf: unsupported event: '%s'\n",
				argv[i]);
			return -1;
		}

		fprintf(stderr, "selected event 0x%02lx (%s)\n",
			perfevt->event, perfevt->event_desc);
	}

	return 0;
}


static int assign_counters(const struct perfcnt **dest, size_t size,
			   const unsigned long *events)
{
	const struct perfcnt **probing;
	size_t i, j, k, probed;

	probing = alloca(size * sizeof(*probing));

	for(i=0; i<size; i++) {
		probed = probe_perfcnt(probing, size, events[i]);
		for (j=0; j<probed; j++) {
			for (k=0; k<i; k++)
				if (probing[j] == dest[k])
					break;
			if (k == i) {
				dest[i] = probing[j];
				break;
			}
		}

		if (j == probed) {
			fprintf(stderr, "xenperf: unable to assign a counter "
				"for event 0x%02lx\n", events[i]);
			return -1;
		}

		fprintf(stderr, "assigned counter %p to event 0x%02lx\n",
			dest[i], events[i]);
	}

	return 0;
}


static int assign_cores(unsigned long *cpumasks, size_t size,
			const unsigned long *events,
			const struct coreinfo *coreinfo)
{
	size_t i, j, core_count = coreinfo->core_count;
	struct perfevt *entry;
	unsigned int *coremap;
	unsigned long scopecore_mask=0, scopecpu_mask=0, scopenode_mask=0;

	coremap = alloca(core_count * sizeof(*coremap));
	if (probe_coremap(coremap, core_count) < 0)
		return -1;

	for (i=0; i<core_count; i++)
		scopecore_mask |= (1 << i);

	/* scopecpu_mask = ??? */
	
	for (i=0; i<coreinfo->node_count; i++)
		for (j=0; j<core_count; j++)
			if (coremap[j] == i) {
				scopenode_mask |= (1 << j);
				break;
			}

	for (i=0; i<size; i++) {
		entry = event_entry(events[i]);
		if (entry == NULL)
			return -1;

		switch (entry->scope) {
		case SCOPE_CORE:  cpumasks[i] = scopecore_mask;  break;
		case SCOPE_CPU:   cpumasks[i] = scopecpu_mask;   break;
		case SCOPE_NODE:  cpumasks[i] = scopenode_mask;  break;
		}

		fprintf(stderr, "assigned cores ");
		for (j=0; j<core_count; j++)
			if (cpumasks[i] & (1L << j))
				fprintf(stderr, "%lu ", j);
		fprintf(stderr, "to event 0x%02lx\n", events[i]);
	}

	return 0;
}


static void display_header(const unsigned long *events,
			   const unsigned long *umasks,
			   const unsigned long *cpumasks, size_t size,
			   const struct coreinfo *coreinfo)
{
	size_t i, j;

	printf("# time      ");

	for (i=0; i<size; i++)
		for (j=0; j<coreinfo->core_count; j++) {
			if (!(cpumasks[i] & (1L << j)))
				continue;
			printf("\t0x%02lx:%02lx(%lu)", events[i],umasks[i],j);
		}
	
	printf("\n");
}


static int initialize_counters(const struct perfcnt **perfcnt,
			       const unsigned long *events,
			       const unsigned long *umasks,
			       const unsigned long *cpumasks, size_t size,
			       const struct coreinfo *coreinfo)
{
	size_t i, j;
	size_t core_count = coreinfo->core_count;

	for (i=0; i<size; i++)
		for (j=0; j<core_count; j++) {
			if (!(cpumasks[i] & (1L << j)))
				continue;
			if (perfcnt_write(perfcnt[i], 0, j) < 0)
				return -1;
			if (perfcnt_enable(perfcnt[i], events[i], umasks[i],
					   j) < 0)
				return -1;
		}

	return 0;
}

static int run_counters(const struct perfcnt **perfcnt,
			const unsigned long *events,
			const unsigned long *umasks,
			const unsigned long *cpumasks, size_t size,
			const struct coreinfo *coreinfo)
{
	int up, down;
	unsigned int i, j;
	unsigned long pmc;
	unsigned long sec = 0, nsec = 0;
	struct timespec ts, tc;

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

		up = down = 0;

		printf("%lu.%09lu", sec, nsec);
		for (i=0; i<size; i++)
			for (j=0; j<coreinfo->core_count; j++) {
				if (!(cpumasks[i] & (1L << j)))
					continue;
				
				pmc = perfcnt_read(perfcnt[i], j);

				if (pmc == (unsigned long) -1) {
					fprintf(stderr, "xenperf: cannot read "
						"counter %p on core %u\n",
						perfcnt[i], j);
					printf("\t          ");
					continue;
				}

				printf("\t%-10lu", pmc);

				if (pmc > (COUNTER_CEIL / 2))
					up = 1;               /* loop faster */
				else if (pmc < (COUNTER_CEIL / 4))
					down = 1;             /* loop slower */

				if (perfcnt_write(perfcnt[i], 0, j) < 0) {
					fprintf(stderr, "xenperf: cannot write"
						" counter %p on core %u\n",
						perfcnt[i], j);
				}

				if (pmc == 0) {
					fprintf(stderr, "suspecting counter "
						"alteration for %p on core "
						"%u\n",
						perfcnt[i], j);
					perfcnt_enable(perfcnt[i], events[i],
						       umasks[i], j);
				}
			}
		printf("\n");
			
		if (up) {
			ts.tv_nsec = ((ts.tv_sec * GIGA) / 2) % GIGA
				+ ts.tv_nsec / 2;
			ts.tv_sec /= 2;
		} else if (down) {
			ts.tv_sec = ts.tv_sec * 2 + ((ts.tv_nsec * 2) / GIGA);
			ts.tv_nsec = (ts.tv_nsec * 2) % GIGA;

			if (ts.tv_sec >= MAXIMAL_SECSLEEP) {
				ts.tv_sec = MAXIMAL_SECSLEEP;
				ts.tv_nsec = 0;
			}
		}

		tc = ts;
	}

	return 0;
}

static int finalize_counters(const struct perfcnt **perfcnt,
			     const unsigned long *cpumasks, size_t size,
			     const struct coreinfo *coreinfo)
{
	size_t i, j;
	size_t core_count = coreinfo->core_count;

	for (i=0; i<size; i++)
		for (j=0; j<core_count; j++) {
			if (!(cpumasks[i] & (1L << j)))
				continue;
			if (perfcnt_write(perfcnt[i], 0, j) < 0)
				return -1;
			if (perfcnt_disable(perfcnt[i], j) < 0)
				return -1;
		}

	return 0;
}


int main(int argc, const char **argv)
{
	struct cpuinfo cpuinfo;
	struct coreinfo coreinfo;
	unsigned long *events, *umasks, *cpumasks;
	const struct perfcnt **perfcnt;

	if (check_cpuinfo(&cpuinfo, 0) < 0)
		return EXIT_FAILURE;

	if (argc == 1) {
		printf("usage: %s [event[:umask]]...\n\n", argv[0]);
		display_events();
		return EXIT_SUCCESS;
	}

	if (check_cpuinfo(&cpuinfo, 1) < 0)
		return EXIT_FAILURE;
	if (check_coreinfo(&coreinfo, 1) < 0)
		return EXIT_FAILURE;
	if (check_hypercall(1) < 0)
		return EXIT_FAILURE;

	events = alloca((argc - 1) * sizeof(*events));
	umasks = alloca((argc - 1) * sizeof(*umasks));
	if (parse_events(argc, argv, events, umasks) < 0)
		return EXIT_FAILURE;

	perfcnt = alloca((argc - 1) * sizeof(*perfcnt));
	if (assign_counters(perfcnt, argc-1, events) < 0)
		return EXIT_FAILURE;

	cpumasks = alloca((argc - 1) * sizeof(*cpumasks));
	if (assign_cores(cpumasks, (argc - 1), events, &coreinfo) < 0)
		return EXIT_FAILURE;

	if (configure_sighandler() < 0) {
		perror("xenperf: failed to install sighandler");
		return EXIT_FAILURE;
	}
	fprintf(stderr, "sighandler installed\n");



	display_header(events, umasks, cpumasks, (argc-1), &coreinfo);

	if (initialize_counters(perfcnt, events, umasks, cpumasks, (argc - 1),
				&coreinfo) < 0)
		return EXIT_FAILURE;

	if (run_counters(perfcnt, events, umasks, cpumasks, (argc - 1),
			 &coreinfo) < 0)
		return EXIT_FAILURE;

	if (finalize_counters(perfcnt, cpumasks, (argc - 1), &coreinfo) < 0)
		return EXIT_FAILURE;

	/* const struct perfcnt *perfcnt; */
	/* unsigned int *coremap; */
	/* unsigned int *cores; */
	/* size_t i, j; */

	/* if (check_cpuinfo(&cpuinfo, 1) < 0) */
	/* 	return EXIT_FAILURE; */
	/* if (check_coreinfo(&coreinfo, 1) < 0) */
	/* 	return EXIT_FAILURE; */
	/* if (check_perfcnt(&perfcnt, 1) == 0) */
	/* 	return EXIT_FAILURE; */
	/* if (check_hypercall(1) < 0) */
	/* 	return EXIT_FAILURE; */

	/* if (configure_sighandler() < 0) { */
	/* 	fprintf(stderr, "xenperf: cannot set signal handler up\n"); */
	/* 	perror("xenperf"); */
	/* 	return EXIT_FAILURE; */
	/* } */

	/* coremap = alloca(sizeof(unsigned int) * coreinfo.core_count); */
	/* probe_coremap(coremap, coreinfo.core_count); */

	/* /\* cores = alloca(sizeof(unsigned int) * coreinfo.core_count); *\/ */
	/* /\* for (i=0; i<coreinfo.core_count; i++) *\/ */
	/* /\* 	cores[i] = i; *\/ */

	/* cores = alloca(sizeof(unsigned int) * coreinfo.node_count); */
	/* for (i=0, j=0; i<coreinfo.core_count; i++) */
	/* 	if (coremap[i] == j) */
	/* 		cores[j++] = i; */


	/* display_events(); */

	/* if (run_counters(perfcnt, EVENT, UMASK, */
	/* 		 cores, coreinfo.node_count) < 0) */
	/* 	return EXIT_FAILURE; */
	
	return EXIT_SUCCESS;
}
