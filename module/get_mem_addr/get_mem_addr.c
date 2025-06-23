#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/bpf.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#include <net/pkt_steer_ops.h>
#include <net/rps.h>
#include <net/netdev_rx_queue.h>
#include <linux/cpu_rmap.h>
#include <linux/percpu.h>










#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kallsyms.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

static struct perf_event * __percpu *sample_hbp;

static char ksym_name[KSYM_NAME_LEN] = "jiffies";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);
MODULE_PARM_DESC(ksym, "Kernel symbol to monitor; this module will report any"
			" write operations on the kernel symbol");

static void sample_hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	printk(KERN_INFO "RIP: %04x:%pS\n", (int)regs->cs, (void *)regs->ip);
	//dump_stack();
}

static int __init hw_break_module_init(void)
{
	int ret;
	struct perf_event_attr attr;
	void *addr = __symbol_get(ksym_name);
	
	int i;
	printk("Inserting Module");

	
	for_each_possible_cpu(i){
		printk("CPU %d: %p (%lu)", i, &(per_cpu(softnet_data, i).backlog.state), (per_cpu(softnet_data, i).backlog.state));
		if (i == 1) addr = &(per_cpu(softnet_data, i).backlog.state);
	}

	if (!addr)
		return -ENXIO;

	hw_breakpoint_init(&attr);
	attr.bp_addr = (unsigned long)addr;
	attr.bp_len = HW_BREAKPOINT_LEN_8;
	attr.bp_type = HW_BREAKPOINT_W;

	sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
	if (IS_ERR((void __force*)sample_hbp)) {
		ret = PTR_ERR((void __force*)sample_hbp);
		goto fail;
	}

	printk(KERN_INFO "HW Breakpoint for %s write installed\n", ksym_name);

	return 0;

fail:
	printk(KERN_INFO "Breakpoint registration failed\n");

	return ret;
}

static void __exit hw_break_module_exit(void)
{
	unregister_wide_hw_breakpoint(sample_hbp);
#ifdef CONFIG_MODULE_UNLOAD
	__symbol_put(ksym_name);
#endif
	printk(KERN_INFO "HW Breakpoint for %s write uninstalled\n", ksym_name);
}

module_init(hw_break_module_init);
module_exit(hw_break_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("K.Prasad");
MODULE_DESCRIPTION("ksym breakpoint");

