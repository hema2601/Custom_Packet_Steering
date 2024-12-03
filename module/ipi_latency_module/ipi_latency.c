#include <linux/fprobe.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

struct my_data{
	uint8_t ready;
	uint64_t latency;
	uint64_t extra_data;
};

DEFINE_PER_CPU(struct my_data, core_data);


static int measure_latency_in(struct fprobe *fp, unsigned long entry_ip, 
	unsigned long ret_ip, struct pt_regs *regs, void *entry_data){

	struct softnet_data *sd = (struct softnet_data*)regs->dx;
	struct my_data *data;

	if(sd && sd->rps_ipi_next == NULL){
		printk("Sending irq to %d\n", sd->cpu);
		data = &per_cpu(core_data, sd->cpu);
		data->latency = rdtsc();
		data->ready = 0;
	}

	return 0;
}


static int measure_latency_out(struct fprobe *fp, unsigned long entry_ip, 
	unsigned long ret_ip, struct pt_regs *regs, void *entry_data){

	struct my_data *data;
	int cpu = smp_processor_id();
	data = &per_cpu(core_data, cpu);


	if(!(data->ready)){
		data->latency = rdtsc() - data->latency;
		data->extra_data = 0;
		data->ready = 1;
	}
	
	return 0;

}
struct fprobe fp1 = {

	.entry_handler 		= measure_latency_in,
};
struct fprobe fp2 = {

	.entry_handler 		= measure_latency_out,
};
	
const char *funcs1[] = {"net_rps_send_ipi"};
const char *funcs2[] = {"rps_trigger_softirq"};
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

	entry = proc_create("ipi_lat_module",0777,NULL,&my_fops);
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
		temp->ready = 1;
		temp->latency = 0;
		temp->extra_data = 0;
	}



	ret = register_fprobe_syms(&fp1, funcs1, 1);
	if(ret < 0){
		printk(KERN_ERR "module could not register fprobe (%d)\n", ret);
		return ret;
	}
	ret = register_fprobe_syms(&fp2, funcs2, 1);
	if(ret < 0){
		unregister_fprobe(&fp1);
		printk(KERN_ERR "module could not register fprobe (%d)\n", ret);
		return ret;
	}
	
	ret = create_proc();
	if(ret < 0){
		printk(KERN_ERR "pkt_steer_module could not setup proc (%d)\n", ret);
		unregister_fprobe(&fp1);
		unregister_fprobe(&fp2);
		return ret;
	}

	return ret;

}

static void __exit exit_latency_module(void){

	remove_proc_entry("ipi_lat_module", NULL);
	unregister_fprobe(&fp1);
	unregister_fprobe(&fp2);

}


module_init(init_latency_module)
module_exit(exit_latency_module)
MODULE_LICENSE("GPL");

