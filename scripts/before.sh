#!/bin/bash

intf=${1:-ens10f1}


ethtool -S ${intf} | grep 'packets\|dropped:' > before_pkt.txt
cat /proc/softirqs | grep NET_ > before_soft_irq.txt 
./scripts/print_irq_cnt.sh $intf > tmp.txt
cat /proc/interrupts | grep -f tmp.txt > before_irq.txt
cat /proc/net/softnet_stat > before_softnet.txt
cat /proc/stat > before_proc_stat.txt
if test -f /proc/pkt_steer_module; then
	cat /proc/pkt_steer_module > before_pkt_steer.txt
fi


