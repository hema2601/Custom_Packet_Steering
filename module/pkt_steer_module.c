#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/bpf.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>

#include "/home/hema/mini_project_rfs/linux-6.10.8/include/net/pkt_steer_ops.h"
#include <net/rps.h>
#include <net/netdev_rx_queue.h>
#include <linux/cpu_rmap.h>
#include <linux/percpu.h>

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
};

DEFINE_PER_CPU(struct my_ops_stats, pkt_steer_stats);


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



		//[Hema] 1. Check qlen of former target
		// To decide: Do I want to check the input queue only or also process queue?

		if(tcpu >= nr_cpu_ids || !cpu_online(tcpu) ||
				(skb_queue_empty(&per_cpu(softnet_data, tcpu).input_pkt_queue) 
				 && skb_queue_empty(&per_cpu(softnet_data, tcpu).process_queue))) {

			//Target is not busy anymore, find a new one
			//		printk("Need to find new CPU");
			tcpu = next_cpu;
			//[HEMA] Replace with interface
			rflow = sd->pkt_steer_ops->set_rps_cpu(dev, skb, rflow, next_cpu);


			spin_lock(&busy_backlog_lock);
			if(!list_empty(&rps_busy_backlog)){

				//printk("Entry in busy backlog!");

				struct busy_backlog_item *item;

				item = list_first_entry_or_null(&rps_busy_backlog, struct busy_backlog_item, list);

				if(item){
					//printk("Found busy CPU!!");
					stats->assignedToBusy++;
					tcpu = item->cpu;
					rflow = sd->pkt_steer_ops->set_rps_cpu(dev, skb, rflow, next_cpu);

				}else{
					stats->noBusyAvailableRace++;
				}

			}else{
				stats->noBusyAvailable++;
			}

			spin_unlock(&busy_backlog_lock);



		}
		//===========================================================



	/*
	 * If the desired CPU (where last recvmsg was done (next_cpu)) is
	 * different from current CPU (one in the rx-queue flow
	 * table entry(tcpu)), switch if one of the following holds:
	 *   - Current CPU is unset (>= nr_cpu_ids).
	 *   - Current CPU is offline.
	 *   - The current CPU's queue tail has advanced beyond the
	 *     last packet that was enqueued using this table entry.
	 *     This guarantees that all previous packets for the flow
	 *     have been dequeued, thus preserving in order delivery.
	 */
	/*if (unlikely(tcpu != next_cpu) &&
	  (tcpu >= nr_cpu_ids || !cpu_online(tcpu) ||
	  ((int)(READ_ONCE(per_cpu(softnet_data, tcpu).input_queue_head) -
	  rflow->last_qtail)) >= 0)) {
	  tcpu = next_cpu;
	//[HEMA] Replace with interface
	rflow = sd->pkt_steer_ops->set_rps_cpu(dev, skb, rflow, next_cpu);
	//[ORIGINAL]
	//rflow = set_rps_cpu(dev, skb, rflow, next_cpu);
	//=============================================
	}*/



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
    if(cpu == this_cpu)
	    stats->targetIsSelf++;
    return cpu;


}

static void this_before(void){

	struct busy_backlog_item *item = &per_cpu(busy_backlog_item, smp_processor_id());	

    
	spin_lock(&busy_backlog_lock);
    list_add(&item->list, &rps_busy_backlog);
    spin_unlock(&busy_backlog_lock);
   
}

static void this_after(void){
	
	struct busy_backlog_item *item = &per_cpu(busy_backlog_item, smp_processor_id());	
	
    spin_lock(&busy_backlog_lock);
	list_del_init(&item->list);
    spin_unlock(&busy_backlog_lock);

}

static struct pkt_steer_ops my_ops = {

	.get_rps_cpu = this_get_cpu,
	.set_rps_cpu = this_set_cpu,
	.before_process_backlog = this_before,
	.after_process_backlog = this_after

};

static int my_proc_show(struct seq_file *m,void *v){

	int i;

	for_each_possible_cpu(i){
		struct my_ops_stats stat = per_cpu(pkt_steer_stats, i);
		seq_printf(m, "%08x %08x %08x %08x %08x %08x\n", i, stat.total, stat.assignedToBusy, stat.noBusyAvailable, stat.noBusyAvailableRace, stat.targetIsSelf);
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

	if(!list_empty(&rps_busy_backlog)){
		printk(KERN_ERR "Busy Backlog still has entries. Abort.");
		return -1;
	}

	ret = create_proc();
	if(ret < 0){
		printk(KERN_ERR "pkt_steer_module could not setup proc (%d)\n", ret);
		return ret;
	}

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

	WRITE_ONCE(net_hotdata.pkt_steer_ops, &pkt_standard_ops);
	
}

module_init(init_pkt_steer_mod)
module_exit(exit_pkt_steer_mod)
MODULE_LICENSE("GPL");


