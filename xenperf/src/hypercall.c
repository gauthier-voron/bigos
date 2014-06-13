#include <stdio.h>
#include <stdlib.h>

#include <xenctrl.h>
#include <xenguest.h>
#include <xc_private.h>

#include "hypercall.h"


#define HYPERCALL_PROBE_CMD    -1
#define HYPERCALL_WRMSR_CMD    -2
#define HYPERCALL_RDMSR_CMD    -3


static int __hypercall_perform(unsigned long cmd, struct register_set *regs)
{
	int ret;
	xc_interface *xch = xc_interface_open(0, 0, 0);

	DECLARE_HYPERCALL;

	if (xch == NULL)
		goto err;

	hypercall.op = __HYPERVISOR_xen_version;
	hypercall.arg[0] = cmd;
	hypercall.arg[1] = (unsigned long) regs;

	ret = do_xen_hypercall(xch, &hypercall);
	xc_interface_close(xch);

	if (ret != 0)
		goto err;
 out:
	return ret;
 err:
	ret = -1;
	goto out;
}

int hypercall_channel_check(void)
{
	return __hypercall_perform(HYPERCALL_PROBE_CMD, NULL);
}

int hypercall_wrmsr(struct register_set *regs)
{
	return __hypercall_perform(HYPERCALL_WRMSR_CMD, regs);
}

int hypercall_rdmsr(struct register_set *regs)
{
	return __hypercall_perform(HYPERCALL_RDMSR_CMD, regs);
}
