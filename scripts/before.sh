#!/bin/bash

intf=${1:-ens10f1}
current_path=/home/hema/Custom_Packet_Steering

ethtool -S ${intf} | grep 'packets\|dropped:' > before_pkt.txt
cat /proc/softirqs | grep NET_ > before_soft_irq.txt 
$current_path/scripts/print_irq_cnt.sh $intf > tmp.txt
cat /proc/interrupts | grep -f tmp.txt > before_irq.txt
cat /proc/net/softnet_stat > before_softnet.txt
cat /proc/stat > before_proc_stat.txt
if test -f /proc/pkt_steer_module; then
	head -n -2 /proc/pkt_steer_module > before_pkt_steer.txt
	tail -n 2 /proc/pkt_steer_module | head -n 1 > before_busy_histo.txt
	tail -n 1 /proc/pkt_steer_module > before_pkt_lat_histo.txt
fi


