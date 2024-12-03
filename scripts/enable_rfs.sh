#!/bin/bash

INTF=$1

echo 32768 > /proc/sys/net/core/rps_sock_flow_entries

num_queues=0

for f in /sys/class/net/$INTF/queues/rx-*/rps_flow_cnt
do
	(( num_queues++ ))
	#echo 32768 > $f
done

per_queue=$(( 32768 / $num_queues ))

for f in /sys/class/net/$INTF/queues/rx-*/rps_flow_cnt
do
	#echo $per_queue
	echo $per_queue > $f
done

#set_irq_affinity.sh $INTF
