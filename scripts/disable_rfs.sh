#!/bin/bash

INTF=$1

echo 0 > /proc/sys/net/core/rps_sock_flow_entries

for f in /sys/class/net/$INTF/queues/rx-*/rps_flow_cnt
do
	echo 0 > $f
done

set_irq_affinity.sh $INTF
