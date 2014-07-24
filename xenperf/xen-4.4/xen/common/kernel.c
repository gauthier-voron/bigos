/******************************************************************************
 * kernel.c
 * 
 * Copyright (c) 2002-2005 K A Fraser
 */

#include <xen/config.h>
#include <xen/init.h>
#include <xen/lib.h>
#include <xen/errno.h>
#include <xen/version.h>
#include <xen/sched.h>
#include <xen/paging.h>
#include <xen/nmi.h>
#include <xen/guest_access.h>
#include <xen/hypercall.h>
#include <asm/current.h>
#include <public/nmi.h>
#include <public/version.h>

#ifndef COMPAT

enum system_state system_state = SYS_STATE_early_boot;

int tainted;

xen_commandline_t saved_cmdline;

static void __init assign_integer_param(
    struct kernel_param *param, uint64_t val)
{
    switch ( param->len )
    {
    case sizeof(uint8_t):
        *(uint8_t *)param->var = val;
        break;
    case sizeof(uint16_t):
        *(uint16_t *)param->var = val;
        break;
    case sizeof(uint32_t):
        *(uint32_t *)param->var = val;
        break;
    case sizeof(uint64_t):
        *(uint64_t *)param->var = val;
        break;
    default:
        BUG();
    }
}

void __init cmdline_parse(const char *cmdline)
{
    char opt[100], *optval, *optkey, *q;
    const char *p = cmdline;
    struct kernel_param *param;
    int bool_assert;

    if ( cmdline == NULL )
        return;

    safe_strcpy(saved_cmdline, cmdline);

    for ( ; ; )
    {
        /* Skip whitespace. */
        while ( *p == ' ' )
            p++;
        if ( *p == '\0' )
            break;

        /* Grab the next whitespace-delimited option. */
        q = optkey = opt;
        while ( (*p != ' ') && (*p != '\0') )
        {
            if ( (q-opt) < (sizeof(opt)-1) ) /* avoid overflow */
                *q++ = *p;
            p++;
        }
        *q = '\0';

        /* Search for value part of a key=value option. */
        optval = strchr(opt, '=');
        if ( optval != NULL )
        {
            *optval++ = '\0'; /* nul-terminate the option value */
            q = strpbrk(opt, "([{<");
        }
        else
        {
            optval = q;       /* default option value is empty string */
            q = NULL;
        }

        /* Boolean parameters can be inverted with 'no-' prefix. */
        bool_assert = !!strncmp("no-", optkey, 3);
        if ( !bool_assert )
            optkey += 3;

        for ( param = &__setup_start; param < &__setup_end; param++ )
        {
            if ( strcmp(param->name, optkey) )
            {
                if ( param->type == OPT_CUSTOM && q &&
                     strlen(param->name) == q + 1 - opt &&
                     !strncmp(param->name, opt, q + 1 - opt) )
                {
                    optval[-1] = '=';
                    ((void (*)(const char *))param->var)(q);
                    optval[-1] = '\0';
                }
                continue;
            }

            switch ( param->type )
            {
            case OPT_STR:
                strlcpy(param->var, optval, param->len);
                break;
            case OPT_UINT:
                assign_integer_param(
                    param,
                    simple_strtoll(optval, NULL, 0));
                break;
            case OPT_BOOL:
            case OPT_INVBOOL:
                if ( !parse_bool(optval) )
                    bool_assert = !bool_assert;
                assign_integer_param(
                    param,
                    (param->type == OPT_BOOL) == bool_assert);
                break;
            case OPT_SIZE:
                assign_integer_param(
                    param,
                    parse_size_and_unit(optval, NULL));
                break;
            case OPT_CUSTOM:
                ((void (*)(const char *))param->var)(optval);
                break;
            default:
                BUG();
                break;
            }
        }
    }
}

int __init parse_bool(const char *s)
{
    if ( !strcmp("no", s) ||
         !strcmp("off", s) ||
         !strcmp("false", s) ||
         !strcmp("disable", s) ||
         !strcmp("0", s) )
        return 0;

    if ( !strcmp("yes", s) ||
         !strcmp("on", s) ||
         !strcmp("true", s) ||
         !strcmp("enable", s) ||
         !strcmp("1", s) )
        return 1;

    return -1;
}

/**
 *      print_tainted - return a string to represent the kernel taint state.
 *
 *  'S' - SMP with CPUs not designed for SMP.
 *  'M' - Machine had a machine check experience.
 *  'B' - System has hit bad_page.
 *
 *      The string is overwritten by the next call to print_taint().
 */
char *print_tainted(char *str)
{
    if ( tainted )
    {
        snprintf(str, TAINT_STRING_MAX_LEN, "Tainted: %c%c%c%c",
                 tainted & TAINT_UNSAFE_SMP ? 'S' : ' ',
                 tainted & TAINT_MACHINE_CHECK ? 'M' : ' ',
                 tainted & TAINT_BAD_PAGE ? 'B' : ' ',
                 tainted & TAINT_SYNC_CONSOLE ? 'C' : ' ');
    }
    else
    {
        snprintf(str, TAINT_STRING_MAX_LEN, "Not tainted");
    }

    return str;
}

void add_taint(unsigned flag)
{
    tainted |= flag;
}

extern const initcall_t __initcall_start[], __presmp_initcall_end[],
    __initcall_end[];

void __init do_presmp_initcalls(void)
{
    const initcall_t *call;
    for ( call = __initcall_start; call < __presmp_initcall_end; call++ )
        (*call)();
}

void __init do_initcalls(void)
{
    const initcall_t *call;
    for ( call = __presmp_initcall_end; call < __initcall_end; call++ )
        (*call)();
}

# define DO(fn) long do_##fn



/* extern unsigned int bigos_count_domain(void); */

extern void bigos_init_demux(unsigned long msr_addr);

extern void bigos_stop_demux(unsigned long msr_addr);

extern unsigned long bigos_read_demux(unsigned long msr_addr, 
                                       unsigned long domain); 


/* extern void bigos_write_demux(unsigned long msr_addr, */
/*                               unsigned long domain, */
/*                               unsigned long value); */


struct bigos_register_set
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
};


void bigos_do_rdmsr(struct bigos_register_set *regs)
{
/*    printk("RDMSR(ecx = 0x%08x, edx:eax = ", regs->ecx);
*/    asm volatile ("rdmsr"
                  : "=a" (regs->eax), "=d" (regs->edx)
                  : "c"  (regs->ecx));
/*    printk("0x%08x:%08x)\n", regs->edx, regs->eax);
*/
}

void bigos_do_wrmsr(const struct bigos_register_set *regs)
{
/*    printk("WRMSR(ecx = 0x%08x, edx:eax = 0x%08x:%08x)\n",
           regs->ecx, regs->edx, regs->eax);
*/    asm volatile ("wrmsr"
                  :
                  : "a" (regs->eax), "c" (regs->ecx), "d" (regs->edx));
}


unsigned int bigos_count_domain(void)
{
    return 2;
}
/*
void bigos_init_demux(unsigned long msr_addr)
{
}

void bigos_stop_demux(unsigned long msr_addr)
{
}

unsigned long bigos_read_demux(unsigned long msr_addr,
                               unsigned long domain)
{
    struct bigos_register_set regs;
    regs.ecx = msr_addr;
    bigos_do_rdmsr(&regs);
    return (((unsigned long) regs.edx) << 32) | (regs.eax & 0xffffffff);
}
*/

void bigos_write_demux(unsigned long msr_addr,
                       unsigned long domain,
                       unsigned long value)
{
    struct bigos_register_set regs;
    regs.ecx = msr_addr;
    regs.edx = value << 32;
    regs.eax = value & 0xffffffff;
    bigos_do_wrmsr(&regs);
}


int bigos_probe(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    return 0;
}

int bigos_demux(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    struct bigos_register_set regs;

    if (copy_from_guest(&regs, arg, 1))
        return -EFAULT;
    bigos_init_demux(regs.ecx);

    return 0;
}

int bigos_remux(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    struct bigos_register_set regs;

    if (copy_from_guest(&regs, arg, 1))
        return -EFAULT;
    bigos_stop_demux(regs.ecx);

    return 0;
}

int bigos_wrmsr(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    struct bigos_register_set regs;
    unsigned long domain;

    if (copy_from_guest(&regs, arg, 1))
        return -EFAULT;
    domain = regs.ebx;

    if (domain == 0) {
        bigos_do_wrmsr(&regs);
        return 0;
    }

    bigos_write_demux(regs.ecx, domain, (((unsigned long) regs.edx) << 32)
                      | (regs.eax & 0xffffffff));

    return 0;
}

int bigos_rdmsr(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    struct bigos_register_set regs;
    unsigned long domain;
    unsigned long value;

    if (copy_from_guest(&regs, arg, 1))
        return -EFAULT;
    domain = regs.ebx;

/*    if (domain == 0) {
        bigos_do_rdmsr(&regs);
    } else {
        value = bigos_read_demux(regs.ecx, domain);
        regs.edx = value >> 32;
        regs.eax = value & 0xffffffff;
    }
*/


    value = bigos_read_demux(regs.ecx, domain);
    regs.edx = value >> 32;
    regs.eax = value & 0xffffffff;
 

    if (copy_to_guest(arg, &regs, 1))
        return -EFAULT;
    return 0;
}

int bigos_count(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    unsigned long count = bigos_count_domain();

    if (copy_to_guest(arg, &count, 1))
        return -EFAULT;
    return 0;
}


#else

#define bigos_probe(cmd, arg)        (-1)

#define bigos_demux(cmd, arg)        (-1)

#define bigos_remux(cmd, arg)        (-1)

#define bigos_wrmsr(cmd, arg)        (-1)

#define bigos_rdmsr(cmd, arg)        (-1)

#define bigos_count(cmd, arg)        (-1)

#endif


/*
 * Simple hypercalls.
 */

#define BIGOS_HYPERCALL_PROBE_CMD   -1
#define BIGOS_HYPERCALL_WRMSR_CMD   -2
#define BIGOS_HYPERCALL_RDMSR_CMD   -3
#define BIGOS_HYPERCALL_COUNT_CMD   -4
#define BIGOS_HYPERCALL_DEMUX_CMD   -5
#define BIGOS_HYPERCALL_REMUX_CMD   -6

DO(xen_version)(int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    switch ( cmd )
    {

    case BIGOS_HYPERCALL_PROBE_CMD:
    {
        return bigos_probe(cmd, arg);
    }
    case BIGOS_HYPERCALL_WRMSR_CMD:
    {
        return bigos_wrmsr(cmd, arg);
    }
    case BIGOS_HYPERCALL_RDMSR_CMD:
    {
        return bigos_rdmsr(cmd, arg);
    }
    case BIGOS_HYPERCALL_COUNT_CMD:
    {
        return bigos_count(cmd, arg);
    }
    case BIGOS_HYPERCALL_DEMUX_CMD:
    {
        return bigos_demux(cmd, arg);
    }
    case BIGOS_HYPERCALL_REMUX_CMD:
    {
        return bigos_remux(cmd, arg);
    }

    case XENVER_version:
    {
        return (xen_major_version() << 16) | xen_minor_version();
    }

    case XENVER_extraversion:
    {
        xen_extraversion_t extraversion;
        safe_strcpy(extraversion, xen_extra_version());
        if ( copy_to_guest(arg, extraversion, ARRAY_SIZE(extraversion)) )
            return -EFAULT;
        return 0;
    }

    case XENVER_compile_info:
    {
        struct xen_compile_info info;
        safe_strcpy(info.compiler,       xen_compiler());
        safe_strcpy(info.compile_by,     xen_compile_by());
        safe_strcpy(info.compile_domain, xen_compile_domain());
        safe_strcpy(info.compile_date,   xen_compile_date());
        if ( copy_to_guest(arg, &info, 1) )
            return -EFAULT;
        return 0;
    }

    case XENVER_capabilities:
    {
        xen_capabilities_info_t info;

        memset(info, 0, sizeof(info));
        arch_get_xen_caps(&info);

        if ( copy_to_guest(arg, info, ARRAY_SIZE(info)) )
            return -EFAULT;
        return 0;
    }
    
    case XENVER_platform_parameters:
    {
        xen_platform_parameters_t params = {
            .virt_start = HYPERVISOR_VIRT_START
        };
        if ( copy_to_guest(arg, &params, 1) )
            return -EFAULT;
        return 0;
        
    }
    
    case XENVER_changeset:
    {
        xen_changeset_info_t chgset;
        safe_strcpy(chgset, xen_changeset());
        if ( copy_to_guest(arg, chgset, ARRAY_SIZE(chgset)) )
            return -EFAULT;
        return 0;
    }

    case XENVER_get_features:
    {
        xen_feature_info_t fi;
        struct domain *d = current->domain;

        if ( copy_from_guest(&fi, arg, 1) )
            return -EFAULT;

        switch ( fi.submap_idx )
        {
        case 0:
            fi.submap = 0;
            if ( VM_ASSIST(d, VMASST_TYPE_pae_extended_cr3) )
                fi.submap |= (1U << XENFEAT_pae_pgdir_above_4gb);
            if ( paging_mode_translate(current->domain) )
                fi.submap |= 
                    (1U << XENFEAT_writable_page_tables) |
                    (1U << XENFEAT_auto_translated_physmap);
            if ( supervisor_mode_kernel )
                fi.submap |= 1U << XENFEAT_supervisor_mode_kernel;
            if ( is_hardware_domain(current->domain) )
                fi.submap |= 1U << XENFEAT_dom0;
#ifdef CONFIG_X86
            switch ( d->guest_type )
            {
            case guest_type_pv:
                fi.submap |= (1U << XENFEAT_mmu_pt_update_preserve_ad) |
                             (1U << XENFEAT_highmem_assist) |
                             (1U << XENFEAT_gnttab_map_avail_bits);
                break;
            case guest_type_pvh:
                fi.submap |= (1U << XENFEAT_hvm_safe_pvclock) |
                             (1U << XENFEAT_supervisor_mode_kernel) |
                             (1U << XENFEAT_hvm_callback_vector);
                break;
            case guest_type_hvm:
                fi.submap |= (1U << XENFEAT_hvm_safe_pvclock) |
                             (1U << XENFEAT_hvm_callback_vector) |
                             (1U << XENFEAT_hvm_pirqs);
                break;
            }
#endif
            break;
        default:
            return -EINVAL;
        }

        if ( copy_to_guest(arg, &fi, 1) )
            return -EFAULT;
        return 0;
    }

    case XENVER_pagesize:
    {
        return (!guest_handle_is_null(arg) ? -EINVAL : PAGE_SIZE);
    }

    case XENVER_guest_handle:
    {
        if ( copy_to_guest(arg, current->domain->handle,
                           ARRAY_SIZE(current->domain->handle)) )
            return -EFAULT;
        return 0;
    }

    case XENVER_commandline:
    {
        if ( copy_to_guest(arg, saved_cmdline, ARRAY_SIZE(saved_cmdline)) )
            return -EFAULT;
        return 0;
    }
    }

    return -ENOSYS;
}

DO(nmi_op)(unsigned int cmd, XEN_GUEST_HANDLE_PARAM(void) arg)
{
    struct xennmi_callback cb;
    long rc = 0;

    switch ( cmd )
    {
    case XENNMI_register_callback:
        rc = -EFAULT;
        if ( copy_from_guest(&cb, arg, 1) )
            break;
        rc = register_guest_nmi_callback(cb.handler_address);
        break;
    case XENNMI_unregister_callback:
        rc = unregister_guest_nmi_callback();
        break;
    default:
        rc = -ENOSYS;
        break;
    }

    return rc;
}

DO(vm_assist)(unsigned int cmd, unsigned int type)
{
    return vm_assist(current->domain, cmd, type);
}

DO(ni_hypercall)(void)
{
    /* No-op hypercall. */
    return -ENOSYS;
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
