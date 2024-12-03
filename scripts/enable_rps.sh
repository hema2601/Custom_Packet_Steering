#!/bin/bash

INTF=$1
CORE_START=$2
CORE_NUM=$3

source /usr/sbin/common_irq_affinity.sh
mask=""

for ((i=0;i<$CORE_START;i++));
do
	mask=0"$mask"
done

for ((i=$CORE_START;i<$((CORE_START+CORE_NUM));i++));
do
	mask=1"$mask"
done



for f in /sys/class/net/$INTF/queues/rx-*/rps_cpus
do
	echo $f
	value=$(printf '%x\n' "$((2#$mask))")
	echo $value
	echo $( add_comma_every_eight $value ) > $f
done

#set_irq_affinity.sh $INTF
