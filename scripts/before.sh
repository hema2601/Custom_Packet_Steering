#!/bin/bash

intf=${1:-ens10f1}


ethtool -S ${intf} | grep packets > before_pkt.txt
cat /proc/softirqs | grep NET_ > before_soft_irq.txt 
./scripts/print_irq_cnt.sh $intf > tmp.txt
cat /proc/interrupts | grep -f tmp.txt > before_irq.txt
cat /proc/net/softnet_stat | cut -c 1-27,82-88,108-116 > before_softnet.txt
