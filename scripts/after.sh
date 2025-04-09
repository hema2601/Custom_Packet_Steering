#!/bin/bash

name=${1}
intf=${2:-ens10f1}

current_path=/home/hema/Custom_Packet_Steering

ethtool -S $intf | grep 'packets\|dropped:' > after_pkt.txt
cat /proc/softirqs | grep NET_ > after_soft_irq.txt 
cat /proc/interrupts | grep -f tmp.txt > after_irq.txt 
cat /proc/net/softnet_stat > after_softnet.txt
cat /proc/stat > after_proc_stat.txt

if test -f /proc/pkt_steer_module; then
    head -n -1 /proc/pkt_steer_module > after_pkt_steer.txt
    tail -n 1 /proc/pkt_steer_module > after_busy_histo.txt
fi

cat before_pkt.txt after_pkt.txt > $current_path/data/$name/packet_cnt.json
cat before_soft_irq.txt after_soft_irq.txt > $current_path/data/$name/softirq.json
cat before_irq.txt after_irq.txt > $current_path/data/$name/irq.json
cat before_softnet.txt after_softnet.txt > $current_path/data/$name/softnet.json
cat before_proc_stat.txt after_proc_stat.txt > $current_path/data/$name/proc_stat.json
if test -f /proc/pkt_steer_module; then
    cat before_pkt_steer.txt after_pkt_steer.txt > $current_path/data/$name/pkt_steer.json
    cat before_busy_histo.txt after_busy_histo.txt > $current_path/data/$name/busy_histo.json
fi

cat $current_path/iperf.json > $current_path/data/$name/iperf_lat.json

rm before_pkt.txt
rm after_pkt.txt
rm before_soft_irq.txt
rm after_soft_irq.txt
rm before_irq.txt
rm after_irq.txt
rm before_softnet.txt
rm after_softnet.txt
rm before_proc_stat.txt
rm after_proc_stat.txt
if test -f before_pkt_steer.txt; then
    rm before_pkt_steer.txt
    rm before_busy_histo.txt
fi
if test -f after_pkt_steer.txt; then
    rm after_pkt_steer.txt
    rm after_busy_histo.txt 
fi

rm tmp.txt
