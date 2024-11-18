#include <linux/fprobe.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

struct my_data{
	uint8_t ready;
	uint64_t latency;
	uint64_t extra_data;
};

DEFINE_PER_CPU(struct my_data, core_data);

//taken from /tools/perf/arch/x86/util/tsc.c
/*static uint64_t rdtsc(void)
{
	unsigned int low, high;

	asm volatile("rdtsc" : "=a" (low), "=d" (high));

	return low | ((u64)high) << 32;
}*/

static int measure_latency_in(struct fprobe *fp, unsigned long entry_ip, 
	unsigned long ret_ip, struct pt_regs *regs, void *entry_data){

	*(uint64_t*)entry_data = rdtsc();

	return 0;
}


static void measure_latency_out(struct fprobe *fp, unsigned long entry_ip, 
	unsigned long ret_ip, struct pt_regs *regs, void *entry_data){

	struct my_data *data;
	int cpu = smp_processor_id();

	uint64_t latency = rdtsc() - *(uint64_t*)entry_data;

	data = &per_cpu(core_data, cpu);

	data->ready = 0;
	data->latency = latency;
	data->extra_data = regs->ax;
	data->ready = 1;
	

}
struct fprobe fp = {

	.entry_handler 		= measure_latency_in,
	.exit_handler 		= measure_latency_out,
	.entry_data_size	= sizeof(uint64_t)
};
	
//char *func = "net_rx_action";
const char *funcs[] = {"ice_clean_rx_irq"};
static int my_proc_show(struct seq_file *m,void *v){

	int i;

	for_each_possible_cpu(i){
		struct my_data data = per_cpu(core_data, i);
		
		seq_printf(m, "%d %llu %llu\n", i, (data.ready)? data.latency : 0, (data.ready)? data.extra_data : 0);
	}

	return 0;
}




static int my_proc_open(struct inode *inode,struct file *file){
	return single_open(file,my_proc_show,NULL);
}

static struct proc_ops my_fops={
//	.owner = THIS_MODULE,
	.proc_open = my_proc_open,
	.proc_release = single_release,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek
};


static int create_proc(void){

	struct proc_dir_entry *entry;

	entry = proc_create("latency_module",0777,NULL,&my_fops);
	if(!entry){
		return -1;
	}else{
		printk(KERN_INFO "create proc file successfully\n");
	}
	return 0;

}
static int __init init_latency_module(void){
	
	int i;
	struct my_data *temp;
	int ret;

	for_each_possible_cpu(i){
		temp = &per_cpu(core_data, i);
		temp->ready = 0;
		temp->latency = 0;
		temp->extra_data = 0;
	}



	ret = register_fprobe_syms(&fp, funcs, 1);
	if(ret < 0){
		printk(KERN_ERR "module could not register fprobe (%d)\n", ret);
		return ret;
	}
	
	ret = create_proc();
	if(ret < 0){
		printk(KERN_ERR "pkt_steer_module could not setup proc (%d)\n", ret);
		unregister_fprobe(&fp);
		return ret;
	}

	return ret;

}

static void __exit exit_latency_module(void){

	unregister_fprobe(&fp);
	remove_proc_entry("latency_module", NULL);

}


module_init(init_latency_module)
module_exit(exit_latency_module)
MODULE_LICENSE("GPL");

