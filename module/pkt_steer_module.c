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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

DEFINE_SPINLOCK(backlog_lock);
LIST_HEAD(busy_backlog);
LIST_HEAD(idle_backlog);

struct backlog_item{

	struct list_head busy_list_entry;
	struct list_head idle_list_entry;
	struct softnet_data *sd;
	int cpu;
	u64 idle;
	u64 elapsed;
};
DEFINE_PER_CPU(struct backlog_item, backlog_item);


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
	int toOverloaded;
	int prevOverloaded;
	int fromOverloaded;
	int allBusyOverloaded;
	int potentialReorder;
};

static int num_curr_cpus = 1;

static int curr_busy = 0;

static int *busy_histo;
static int busy_histo_size;

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

static unsigned long threshold_up __read_mostly = 500;

module_param(threshold_up, ulong, 0644);
MODULE_PARM_DESC(threshold_up, "Define the upper CPU utilization threshold at which IAPS scales up (if 50%, then give 500)");

static unsigned long threshold_low __read_mostly = 200;

module_param(threshold_low, ulong, 0644);
MODULE_PARM_DESC(threshold_low, "Define the lower CPU utilization threshold at which IAPS scales down (if 20%, then give 200)");

static int iq_thresh __read_mostly = 100;
module_param(iq_thresh, int, 0644);
MODULE_PARM_DESC(iq_thresh, "Define the limit of packets in the input queue after which a core will be considered overloaded");

static int activate_overload __read_mostly = 1;
module_param(activate_overload, int, 0644);
MODULE_PARM_DESC(activate_overload, "Toggle overload detection");

static int risk_reorder __read_mostly = 0;
module_param(risk_reorder, int, 0644);
MODULE_PARM_DESC(risk_reorder, "Toggle whether packets should be steered away from an overloaded core, even if the previous core still has packets from the same connection");



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

static int perform_rfs(struct net_device *dev, struct sk_buff *skb){
	//[HEMA] Add local variables
	struct softnet_data *sd;
	//===========================
	const struct rps_sock_flow_table *sock_flow_table;
	struct netdev_rx_queue *rxqueue = dev->_rx;
	struct rps_dev_flow_table *flow_table;
	int cpu = -1;
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
	if (!flow_table){
		goto done;
	}
	skb_reset_network_header(skb);
	hash = skb_get_hash(skb);
	if (!hash){
		goto done;
	}

	sock_flow_table = rcu_dereference(net_hotdata.rps_sock_flow_table);
	//[HEMA] Get sd to access pkt steering ops
	sd = &per_cpu(softnet_data, smp_processor_id());
	//=======================================
	if (flow_table && sock_flow_table) {
		u32 ident;

		/* First check into global flow table if there is a match.
		 * This READ_ONCE() pairs with WRITE_ONCE() from rps_record_sock_flow().
		 */
		ident = READ_ONCE(sock_flow_table->ents[hash & sock_flow_table->mask]);
		if ((ident ^ hash) & ~net_hotdata.rps_cpu_mask)
			goto done;

		cpu = ident & net_hotdata.rps_cpu_mask;
	}
done:
	return cpu;

}


static int previous_invalid(int tcpu){
    return tcpu >= nr_cpu_ids || !cpu_online(tcpu);
}

static int previous_still_busy(int tcpu){
    struct sk_buff_head *target_input_queue = &(per_cpu(softnet_data, tcpu).input_pkt_queue);
    unsigned long *target_NAPI_state = &per_cpu(softnet_data, tcpu).backlog.state;


    unsigned int in_NAPI = test_bit(NAPI_STATE_SCHED, target_NAPI_state);
    unsigned int has_work = !skb_queue_empty(target_input_queue);

    return in_NAPI || has_work;

}

static int is_overloaded(int tcpu){

    //[TODO] Decide whether input queue length alone is enough, or whether CPU util should be considered as well

    struct sk_buff_head *target_input_queue = &(per_cpu(softnet_data, tcpu).input_pkt_queue);

    return (activate_overload) ? skb_queue_len_lockless(target_input_queue) > iq_thresh
                               : 0;
}


static int this_get_cpu(struct net_device *dev, struct sk_buff *skb,
		struct rps_dev_flow **rflowp){

	//[HEMA] Add local variables
	struct softnet_data *sd;
	struct my_ops_stats *stats;
	//===========================
	struct netdev_rx_queue *rxqueue = dev->_rx;
	struct rps_dev_flow_table *flow_table;
	struct rps_map *map;
	int cpu = -1;
	int this_cpu = smp_processor_id();
	u32 tcpu;
	u32 hash;
	unsigned long flags;

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

	//flow_table = rcu_dereference(rxqueue->rps_flow_table);
	//if(flow_table)
	//	printk("Found it!");
	flow_table = rcu_dereference(iaps_flow_table);
	map = rcu_dereference(rxqueue->rps_map);
	if (!flow_table && !map)
		goto done;

	skb_reset_network_header(skb);
	hash = skb_get_hash(skb);
	if (!hash)
		goto done;

	//[HEMA] Get sd to access pkt steering ops
	sd = &per_cpu(softnet_data, this_cpu);
	//=======================================
	if (flow_table) {
		struct rps_dev_flow *rflow;

		rflow = &flow_table->flows[hash & flow_table->mask];
		tcpu = rflow->cpu;

		uint8_t invalid = 0, idle = 0, overloaded = 0;

		if(			(invalid = previous_invalid(tcpu)) 
				|| 	(idle = !previous_still_busy(tcpu))
				|| 	(overloaded = is_overloaded(tcpu))
		  ) {

			if(invalid){
				stats->prevInvalid++;
			}else if (idle){
				stats->prevIdle++;
			}else{	//overloaded
				stats->prevOverloaded++;
				//If packets of this flow are still enqueued, jump out of new target selection
				if ((int)(READ_ONCE(per_cpu(softnet_data, tcpu).input_queue_head) -
					 rflow->last_qtail) < 0){
					if(!risk_reorder)	goto confirm_target;
					stats->potentialReorder++;
				}
				
				stats->fromOverloaded++;

			}

			//[TODO] Decide backup core
			switch(choose_backup_core){
				case CURR:
					tcpu = this_cpu;
					break;
				case RPS:
					tcpu = (hash % max_cpus) + base_cpu;
					break;
				case APP:
					tcpu = perform_rfs(dev, skb);
					if(tcpu != -1)
						break;
					WARN_ONCE(1, "RFS does not seem to be working, fall back to load balancing");
					fallthrough;
				case LB:
					tcpu = (hash % num_curr_cpus) + base_cpu;
				case PREV:
				default:
					break;
			}

			spin_lock_irqsave(&backlog_lock, flags);
			if(!list_empty(&busy_backlog)){
				struct backlog_item *item;
				struct list_head *next;	

				list_for_each(next, &busy_backlog){
					item = list_entry(next, struct backlog_item, busy_list_entry);
					if (!is_overloaded(item->cpu))	break;
					else							item = NULL;
				}

				if(item){
					stats->assignedToBusy++;
					tcpu = item->cpu;
				}else{
					stats->allBusyOverloaded++;
				}


			}else{
				stats->noBusyAvailable++;

				//For Debug reasons, lets check whether this new core is actuyally "Not Busy"
				if(previous_still_busy(tcpu)){
					pr_debug("Actually Steered to Busy Core!!\n");
				}
	


			}
			spin_unlock_irqrestore(&backlog_lock, flags);

			rflow = sd->pkt_steer_ops->set_rps_cpu(dev, skb, rflow, tcpu);
		}
confirm_target:
		busy_histo[curr_busy]++;
		if (tcpu < nr_cpu_ids && cpu_online(tcpu)) {
			*rflowp = rflow;
			cpu = tcpu;
			goto done;
		}else{
			stats->choseInvalid++;

		}
	}

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

	unsigned long flags;
	struct list_head *list = &(per_cpu(backlog_item, tcpu).busy_list_entry);	
	
    
	spin_lock_irqsave(&backlog_lock, flags);
	//printk("Adding to busy list");
	if(list->prev == list){
		curr_busy++;
    	list_add(list, &busy_backlog);
	}
	spin_unlock_irqrestore(&backlog_lock, flags);
   
}

static void this_after(int tcpu){
	
	unsigned long flags;
	struct backlog_item *item = &per_cpu(backlog_item, tcpu);	
	
    spin_lock_irqsave(&backlog_lock, flags);
	//printk("Removing from busy list");
	if(item->busy_list_entry.prev != &item->busy_list_entry){
		curr_busy--;
		list_del_init(&item->busy_list_entry);
    }
    spin_unlock_irqrestore(&backlog_lock, flags);

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
		seq_printf(m, "%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", i, stat.total, stat.prevInvalid, stat.prevIdle, stat.assignedToBusy, stat.noBusyAvailable, stat.targetIsSelf, stat.choseInvalid, stat.fallback, stat.prevOverloaded, stat.fromOverloaded, stat.allBusyOverloaded, stat.potentialReorder);
	}

	for(i = 0; i < busy_histo_size; i++)	seq_printf(m, "%08x ", busy_histo[i]);

	seq_printf(m, "\n");
	

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
		pr_info("Create proc file successfully\n");
	}
	return 0;

}


static struct timer_list lb_timer;


static unsigned long get_and_update_cpu_util(struct backlog_item *item, int cpu){

        unsigned long util;
        unsigned int idle, elapsed, prev_idle, prev_elapsed;

        prev_idle       = item->idle;
        prev_elapsed    = item->elapsed;

        item->idle = get_cpu_idle_time(cpu, &item->elapsed, 0);

        elapsed = item->elapsed - prev_elapsed;
        idle    = item->idle - prev_idle;

        util = elapsed/1000 - idle/1000;

        pr_debug("CPU #%d: %lu Idle time: %u (%u)\n", cpu, util, idle/1000, elapsed/1000);

        return util;

}

static void iaps_load_balancer(struct timer_list *timer){

    unsigned long avg_util = 0;
    struct backlog_item *item;


    //Skip Calculations, if no load balancing needed
    if(choose_backup_core != LB)
        goto rearm;


    //Check average utilization
    for(int i = 0; i < num_curr_cpus; i++){
        item = &per_cpu(backlog_item, base_cpu + i);
        avg_util += get_and_update_cpu_util(item, base_cpu+i);
    }
    avg_util /= num_curr_cpus;


    //update CPU count
    if((avg_util > threshold_up) && num_curr_cpus < max_cpus){
        item = &per_cpu(backlog_item, base_cpu + num_curr_cpus);
        //Obtain initial values
        item->idle=get_cpu_idle_time(base_cpu+num_curr_cpus, &item->elapsed, 0);
        num_curr_cpus++;
    }

    if(avg_util < threshold_low && num_curr_cpus > 1)
        num_curr_cpus--;


    // Rearm Timer
rearm:
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
	struct backlog_item *item;
	pr_info("Inserting Module");
	
	for_each_possible_cpu(i){
		item = &per_cpu(backlog_item, i);

		item->cpu = i;
		INIT_LIST_HEAD(&item->busy_list_entry);
		INIT_LIST_HEAD(&item->idle_list_entry);
		item->sd = &per_cpu(softnet_data, i);
	}

	busy_histo_size = cpumask_weight(cpu_possible_mask) + 1;
	busy_histo = kmalloc_array(busy_histo_size, sizeof(int), GFP_KERNEL);

	if(busy_histo == NULL){
		pr_err("No more mem\n");
		return ret;
	}

	for (int i = 0; i < busy_histo_size; i++)
		busy_histo[i] = 0;	
	
	ret = create_flow_table(0);
	if(ret < 0){
		kfree(busy_histo);
		pr_err("Failed to set up flow table for IAPS (%d)\n", ret);
		return ret;
	}

	if(!list_empty(&busy_backlog)){
		pr_err("Busy Backlog still has entries. Abort.");
		kfree(busy_histo);
		create_flow_table(1);
		return -1;
	}

	ret = create_proc();
	if(ret < 0){
		pr_err("pkt_steer_module could not setup proc (%d)\n", ret);
		kfree(busy_histo);
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
	}

	remove_proc_entry("pkt_steer_module", NULL);

	//remove timer
	del_timer(&lb_timer);


	WRITE_ONCE(net_hotdata.pkt_steer_ops, &pkt_standard_ops);
		
	create_flow_table(1);

	kfree(busy_histo);
	
}

module_init(init_pkt_steer_mod)
module_exit(exit_pkt_steer_mod)
MODULE_LICENSE("GPL");

/****************************************************************************
 *	Versioning																*
 * ========================================================================	*		
 * 	Modules with the same first version number use the same custom kernel	*
 * 	Modules ending in a 0 are a new 'major version'. 						*
 *	Performance benchmarks for them should be provided.						*
 *	Modules with the same second version number but different third			*
 *	differ in minor features. Iterate on the third number until stable 		*
 *	performance can be observed.											*
 ****************************************************************************/
MODULE_VERSION("1.0.1" " " "20250310");
MODULE_AUTHOR("Maike Helbig <hema@g.skku.edu>");

