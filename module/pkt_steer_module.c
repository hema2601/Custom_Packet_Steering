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
#include <linux/moduleparam.h>
//#include <linux/sched.h>
#include "/home/hema/Custom_Packet_Steering/linux-6.10.8/kernel/sched/sched.h"


DEFINE_SPINLOCK(busy_backlog_lock);
LIST_HEAD(rps_busy_backlog);

struct busy_backlog_item{

	struct list_head list;
	struct softnet_data *sd;
	int cpu;
};
DEFINE_PER_CPU(struct busy_backlog_item, busy_backlog_item);


struct my_ops_stats{
	int assignedToBusy;
	int noBusyAvailable;
	int noBusyAvailableRace;
	int targetIsSelf;
	int total;
	int fallback;
	int prevInvalid;
	int prevIdle;
	int choseInvalid;
};

static int num_curr_cpus = 1;

DEFINE_PER_CPU(struct my_ops_stats, pkt_steer_stats);

#define APP 1
#define CURR 2
#define RPS 3
#define LB 4
#define PREV 5

#define MY_LIST_HEAD 0
#define MY_LIST_TAIL 1

static int choose_backup_core __read_mostly = PREV;

module_param(choose_backup_core, int, 0644);
MODULE_PARM_DESC(choose_backup_core, "Decide Core when there are no busy cores: Application Core(1), Current Core(2), RPS(3), Load Balancing(4), Previous(5)");

static int custom_toggle __read_mostly = 1;

module_param(custom_toggle, int, 0644);
MODULE_PARM_DESC(custom_toggle, "Switch between custom and normal packet steering, Normal = 0, Custom = 1");

static int list_position __read_mostly = MY_LIST_HEAD;

module_param(list_position, int, 0644);
MODULE_PARM_DESC(list_position, "Decide whether to find new busy cores from the head or from the tail of the busy list, Head = 0, Tail = 1");

static int base_cpu __read_mostly = 1;

module_param(base_cpu, int, 0644);
MODULE_PARM_DESC(base_cpu, "Define the beginning of the CPUs that IAPS can scale to");

static int max_cpus __read_mostly = 7;

module_param(max_cpus, int, 0644);
MODULE_PARM_DESC(max_cpus, "Define the maximum number of CPUs IAPS can scale to");

static unsigned long flow_table_size __read_mostly = 32768;

module_param(flow_table_size, ulong, 0644);
MODULE_PARM_DESC(flow_table_size, "Define the number of maximum flows for IAPS");



static struct rps_dev_flow_table __rcu *iaps_flow_table;


static struct rps_dev_flow *this_set_cpu(struct net_device *dev, struct sk_buff *skb,
		       struct rps_dev_flow *rflow, u16 next_cpu){


	if (next_cpu < nr_cpu_ids) {
		u32 head;
#ifdef CONFIG_RFS_ACCEL
		struct netdev_rx_queue *rxqueue;
		struct rps_dev_flow_table *flow_table;
		struct rps_dev_flow *old_rflow;
		u16 rxq_index;
		u32 flow_id;
		int rc;

		/* Should we steer this flow to a different hardware queue? */
		if (!skb_rx_queue_recorded(skb) || !dev->rx_cpu_rmap ||
				!(dev->features & NETIF_F_NTUPLE))
			goto out;
		rxq_index = cpu_rmap_lookup_index(dev->rx_cpu_rmap, next_cpu);
		if (rxq_index == skb_get_rx_queue(skb))
			goto out;

		rxqueue = dev->_rx + rxq_index;
		flow_table = rcu_dereference(rxqueue->rps_flow_table);
		if (!flow_table)
			goto out;
		flow_id = skb_get_hash(skb) & flow_table->mask;
		rc = dev->netdev_ops->ndo_rx_flow_steer(dev, skb,
				rxq_index, flow_id);
		if (rc < 0)
			goto out;
		old_rflow = rflow;
		rflow = &flow_table->flows[flow_id];
		WRITE_ONCE(rflow->filter, rc);
		if (old_rflow->filter == rc)
			WRITE_ONCE(old_rflow->filter, RPS_NO_FILTER);
out:
#endif
		head = READ_ONCE(per_cpu(softnet_data, next_cpu).input_queue_head);
		rps_input_queue_tail_save(&rflow->last_qtail, head);
	}

	WRITE_ONCE(rflow->cpu, next_cpu);
	return rflow;

}

static int this_get_cpu(struct net_device *dev, struct sk_buff *skb,
		struct rps_dev_flow **rflowp){

	//[HEMA] Add local variables
	struct softnet_data *sd;
	struct my_ops_stats *stats;
	//===========================
	const struct rps_sock_flow_table *sock_flow_table;
	struct netdev_rx_queue *rxqueue = dev->_rx;
	struct rps_dev_flow_table *flow_table;
	struct rps_map *map;
	int cpu = -1;
	int this_cpu = smp_processor_id();
	u32 tcpu;
	u32 hash;

	stats = &per_cpu(pkt_steer_stats, this_cpu);

	//printk("this_get_cpu: Called");

	stats->total++;

	if (skb_rx_queue_recorded(skb)) {
		u16 index = skb_get_rx_queue(skb);

		if (unlikely(index >= dev->real_num_rx_queues)) {
			WARN_ONCE(dev->real_num_rx_queues > 1,
					"%s received packet on queue %u, but number "
					"of RX queues is %u\n",
					dev->name, index, dev->real_num_rx_queues);
			goto done;
		}
		rxqueue += index;
	}

	/* Avoid computing hash if RFS/RPS is not active for this rxqueue */

	flow_table = rcu_dereference(rxqueue->rps_flow_table);
	map = rcu_dereference(rxqueue->rps_map);
	if (!flow_table && !map)
		goto done;

	skb_reset_network_header(skb);
	hash = skb_get_hash(skb);
	if (!hash)
		goto done;

	sock_flow_table = rcu_dereference(net_hotdata.rps_sock_flow_table);
	//[HEMA] Get sd to access pkt steering ops
	sd = &per_cpu(softnet_data, this_cpu);
	//=======================================
	if (flow_table && sock_flow_table) {
		struct rps_dev_flow *rflow;
		u32 next_cpu;
		u32 ident;

		/* First check into global flow table if there is a match.
		 * This READ_ONCE() pairs with WRITE_ONCE() from rps_record_sock_flow().
		 */
		ident = READ_ONCE(sock_flow_table->ents[hash & sock_flow_table->mask]);
		if ((ident ^ hash) & ~net_hotdata.rps_cpu_mask)
			goto try_rps;

		// CPU where last receive was done
		next_cpu = ident & net_hotdata.rps_cpu_mask;

		/* OK, now we know there is a match,
		 * we can look at the local (per receive queue) flow table
		 */
		rflow = &flow_table->flows[hash & flow_table->mask];
		tcpu = rflow->cpu;


		if(tcpu >= nr_cpu_ids || !cpu_online(tcpu) 
				|| !test_bit(NAPI_STATE_SCHED,&per_cpu(softnet_data, tcpu).backlog.state )
				|| (skb_queue_empty(&per_cpu(softnet_data, tcpu).input_pkt_queue) 
				//remove check for process_queue. Reason: The interrupt decision is made base don only input_pkt_queue
				 /*&& skb_queue_empty(&per_cpu(softnet_data, tcpu).process_queue)*/)) {

			if(tcpu >= nr_cpu_ids || !cpu_online(tcpu)){
				stats->prevInvalid++;
			}else{
				stats->prevIdle++;
			}

			//[TODO] Decide backup core
			switch(choose_backup_core){
				case APP:
					tcpu = next_cpu;
					break;
				case CURR:
					tcpu = this_cpu;
					break;
				case RPS:
					if (map) 
						tcpu = map->cpus[reciprocal_scale(hash, map->len)];
					break;
				case LB:
					tcpu = (hash % num_curr_cpus) + base_cpu;
				case PREV:
				default:
					break;
			}

			spin_lock(&busy_backlog_lock);
			if(!list_empty(&rps_busy_backlog)){
				struct busy_backlog_item *item;
				if(list_position == MY_LIST_HEAD)
					item = list_first_entry(&rps_busy_backlog, struct busy_backlog_item, list);
				else 
					item = list_last_entry(&rps_busy_backlog, struct busy_backlog_item, list);

				if(item){
					stats->assignedToBusy++;
					tcpu = item->cpu;
				}else{
					stats->noBusyAvailableRace++;
				}
			}else{
				stats->noBusyAvailable++;
			}
			spin_unlock(&busy_backlog_lock);

			rflow = sd->pkt_steer_ops->set_rps_cpu(dev, skb, rflow, tcpu);
		}

		if (tcpu < nr_cpu_ids && cpu_online(tcpu)) {
			*rflowp = rflow;
			cpu = tcpu;
			goto done;
		}else{
			stats->choseInvalid++;

		}
	}

try_rps:
	//Fall back on RPS
	stats->fallback++;
	if (map) {
		tcpu = map->cpus[reciprocal_scale(hash, map->len)];
		if (cpu_online(tcpu)) {
			cpu = tcpu;
			goto done;
		}
	}

done:
	if(cpu == this_cpu)
		stats->targetIsSelf++;
	return cpu;


}

static void this_before(int tcpu){

	struct list_head *list = &(per_cpu(busy_backlog_item, tcpu).list);	
	
    
	spin_lock(&busy_backlog_lock);
	//printk("Adding to busy list");
	if(list->prev == list)
    	list_add(list, &rps_busy_backlog);
    spin_unlock(&busy_backlog_lock);
   
}

static void this_after(int tcpu){
	
	struct busy_backlog_item *item = &per_cpu(busy_backlog_item, tcpu);	
	
    spin_lock(&busy_backlog_lock);
	//printk("Removing from busy list");
	list_del_init(&item->list);
    spin_unlock(&busy_backlog_lock);

}

static int get_rps_cpu_copy(struct net_device *dev, struct sk_buff *skb,
		struct rps_dev_flow **rflowp)
{
	//[HEMA] Add local variables
	struct softnet_data *sd;
	//===========================
	const struct rps_sock_flow_table *sock_flow_table;
	struct netdev_rx_queue *rxqueue = dev->_rx;
	struct rps_dev_flow_table *flow_table;
	struct rps_map *map;
	int cpu = -1;
	u32 tcpu;
	u32 hash;

	if (skb_rx_queue_recorded(skb)) {
		u16 index = skb_get_rx_queue(skb);

		if (unlikely(index >= dev->real_num_rx_queues)) {
			WARN_ONCE(dev->real_num_rx_queues > 1,
					"%s received packet on queue %u, but number "
					"of RX queues is %u\n",
					dev->name, index, dev->real_num_rx_queues);
			goto done;
		}
		rxqueue += index;
	}

	/* Avoid computing hash if RFS/RPS is not active for this rxqueue */

	flow_table = rcu_dereference(rxqueue->rps_flow_table);
	map = rcu_dereference(rxqueue->rps_map);
	if (!flow_table && !map)
		goto done;

	skb_reset_network_header(skb);
	hash = skb_get_hash(skb);
	if (!hash)
		goto done;

	sock_flow_table = rcu_dereference(net_hotdata.rps_sock_flow_table);
	//[HEMA] Get sd to access pkt steering ops
	sd = &per_cpu(softnet_data, smp_processor_id());
	//=======================================
	if (flow_table && sock_flow_table) {
		struct rps_dev_flow *rflow;
		u32 next_cpu;
		u32 ident;

		/* First check into global flow table if there is a match.
		 * This READ_ONCE() pairs with WRITE_ONCE() from rps_record_sock_flow().
		 */
		ident = READ_ONCE(sock_flow_table->ents[hash & sock_flow_table->mask]);
		if ((ident ^ hash) & ~net_hotdata.rps_cpu_mask)
			goto try_rps;

		next_cpu = ident & net_hotdata.rps_cpu_mask;

		/* OK, now we know there is a match,
		 * we can look at the local (per receive queue) flow table
		 */
		rflow = &flow_table->flows[hash & flow_table->mask];
		tcpu = rflow->cpu;

		/*
		 * If the desired CPU (where last recvmsg was done) is
		 * different from current CPU (one in the rx-queue flow
		 * table entry), switch if one of the following holds:
		 *   - Current CPU is unset (>= nr_cpu_ids).
		 *   - Current CPU is offline.
		 *   - The current CPU's queue tail has advanced beyond the
		 *     last packet that was enqueued using this table entry.
		 *     This guarantees that all previous packets for the flow
		 *     have been dequeued, thus preserving in order delivery.
		 */
		if (unlikely(tcpu != next_cpu) &&
				(tcpu >= nr_cpu_ids || !cpu_online(tcpu) ||
				 ((int)(READ_ONCE(per_cpu(softnet_data, tcpu).input_queue_head) -
					 rflow->last_qtail)) >= 0)) {
			tcpu = next_cpu;
			//[HEMA] Replace with interface
			rflow = sd->pkt_steer_ops->set_rps_cpu(dev, skb, rflow, next_cpu);
			//[ORIGINAL]
			//rflow = set_rps_cpu(dev, skb, rflow, next_cpu);
			//=============================================
		}

		if (tcpu < nr_cpu_ids && cpu_online(tcpu)) {
			*rflowp = rflow;
			cpu = tcpu;
			goto done;
		}
	}

try_rps:

	if (map) {
		tcpu = map->cpus[reciprocal_scale(hash, map->len)];
		if (cpu_online(tcpu)) {
			cpu = tcpu;
			goto done;
		}
	}

done:
	return cpu;
}





static void interface_mark_as_busy(int tcpu){

	if(custom_toggle == 1) this_before(tcpu);

}
static void interface_mark_as_idle(int tcpu){

	if(custom_toggle == 1) this_after(tcpu);
		
}
static int interface_get_rps_cpu(struct net_device *dev, struct sk_buff *skb,
		struct rps_dev_flow **rflowp){

	if(custom_toggle == 1)	return this_get_cpu(dev, skb, rflowp);
		
	else 				return get_rps_cpu_copy(dev, skb, rflowp);


}


static struct pkt_steer_ops my_ops = {

	.get_rps_cpu = interface_get_rps_cpu,
	.set_rps_cpu = this_set_cpu,
	.mark_as_busy = interface_mark_as_busy,
	.mark_as_idle = interface_mark_as_idle

};


static int my_proc_show(struct seq_file *m,void *v){

	int i;

	for_each_possible_cpu(i){
		struct my_ops_stats stat = per_cpu(pkt_steer_stats, i);
		seq_printf(m, "%08x %08x %08x %08x %08x %08x %08x %08x %08x\n", i, stat.total, stat.prevInvalid, stat.prevIdle, stat.assignedToBusy, stat.noBusyAvailable, stat.targetIsSelf, stat.choseInvalid, stat.fallback);
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

	entry = proc_create("pkt_steer_module",0777,NULL,&my_fops);
	if(!entry){
		return -1;
	}else{
		printk(KERN_INFO "create proc file successfully\n");
	}
	return 0;

}


static struct timer_list lb_timer;

static inline unsigned long cpu_util(int cpu){

	return cpu_util_cfs(cpu);
}

unsigned long threshold_up = 500;
unsigned long threshold_low = 200;


static void iaps_load_balancer(struct timer_list *timer){

	unsigned long avg_util = 0;

	printk("Timer Triggered!\n");


	//Check average utilization
	for(int i = 0; i < num_curr_cpus; i++){
		unsigned long util = cpu_util(base_cpu + i);

		printk("CPU %d Util: %lu\n", base_cpu + i, util);

		avg_util += util;
	}

	avg_util /= num_curr_cpus;

	printk("Avergae Usage: %lu\n", avg_util);

	//update CPU count

	if(avg_util > threshold_up && num_curr_cpus < max_cpus)
		num_curr_cpus++;
	
	if(avg_util < threshold_low && num_curr_cpus > 1)
		num_curr_cpus--;




	// Rearm Timer
	mod_timer(&lb_timer, jiffies + msecs_to_jiffies(1000));

}

//Copied from Kernel
static void rps_dev_flow_table_release(struct rcu_head *rcu)
{
	struct rps_dev_flow_table *table = container_of(rcu,
	    struct rps_dev_flow_table, rcu);
	vfree(table);
}

static int create_flow_table(int destroy){

	unsigned long mask, count = (destroy) ? 0 : flow_table_size;
	struct rps_dev_flow_table *table, *old_table;
	static DEFINE_SPINLOCK(rps_dev_flow_lock);

	if (count) {
		mask = count - 1;
		/* mask = roundup_pow_of_two(count) - 1;
		 * without overflows...
		 */
		while ((mask | (mask >> 1)) != mask)
			mask |= (mask >> 1);
		/* On 64 bit arches, must check mask fits in table->mask (u32),
		 * and on 32bit arches, must check
		 * RPS_DEV_FLOW_TABLE_SIZE(mask + 1) doesn't overflow.
		 */
#if BITS_PER_LONG > 32
		if (mask > (unsigned long)(u32)mask)
			return -EINVAL;
#else
		if (mask > (ULONG_MAX - RPS_DEV_FLOW_TABLE_SIZE(1))
				/ sizeof(struct rps_dev_flow)) {
			/* Enforce a limit to prevent overflow */
			return -EINVAL;
		}
#endif
		table = vmalloc(RPS_DEV_FLOW_TABLE_SIZE(mask + 1));
		if (!table)
			return -ENOMEM;

		table->mask = mask;
		for (count = 0; count <= mask; count++)
			table->flows[count].cpu = RPS_NO_CPU;
	} else {
		table = NULL;
	}

	spin_lock(&rps_dev_flow_lock);
	old_table = rcu_dereference_protected(iaps_flow_table,
			lockdep_is_held(&rps_dev_flow_lock));
	rcu_assign_pointer(iaps_flow_table, table);
	spin_unlock(&rps_dev_flow_lock);

	if (old_table)
		call_rcu(&old_table->rcu, rps_dev_flow_table_release);

	return 0;

}

static int __init init_pkt_steer_mod(void){
	int ret;
	int i;
	struct busy_backlog_item *item;
	printk("Inserting Module");
	
	for_each_possible_cpu(i){
		item = &per_cpu(busy_backlog_item, i);

		item->cpu = i;
		INIT_LIST_HEAD(&item->list);
		item->sd = &per_cpu(softnet_data, i);

	}

	ret = create_flow_table(0);
	if(ret < 0){
		printk(KERN_ERR "Failed to set up flow table for IAPS (%d)\n", ret);
		return ret;
	}

	if(!list_empty(&rps_busy_backlog)){
		printk(KERN_ERR "Busy Backlog still has entries. Abort.");
		create_flow_table(1);
		return -1;
	}

	ret = create_proc();
	if(ret < 0){
		printk(KERN_ERR "pkt_steer_module could not setup proc (%d)\n", ret);
		create_flow_table(1);
		return ret;
	}

	//activate timer
	timer_setup(&lb_timer, iaps_load_balancer, 0);
	mod_timer(&lb_timer, jiffies + msecs_to_jiffies(1000));

	WRITE_ONCE(net_hotdata.pkt_steer_ops, &my_ops);

	return 0;

}

static void __exit exit_pkt_steer_mod(void){
	int i;
	for_each_possible_cpu(i){
		struct my_ops_stats stat = per_cpu(pkt_steer_stats, i);
		printk("CPU %d: %d %d %d %d %d", i, stat.total, stat.assignedToBusy, stat.noBusyAvailable, stat.noBusyAvailableRace, stat.targetIsSelf);
	}

	remove_proc_entry("pkt_steer_module", NULL);

	//remove timer
	del_timer(&lb_timer);


	WRITE_ONCE(net_hotdata.pkt_steer_ops, &pkt_standard_ops);
		
	create_flow_table(1);
	
}

module_init(init_pkt_steer_mod)
module_exit(exit_pkt_steer_mod)
MODULE_LICENSE("GPL");


