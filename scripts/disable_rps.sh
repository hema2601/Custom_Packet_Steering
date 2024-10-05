#!/bin/bash

INTF=$1

for f in /sys/class/net/$INTF/queues/rx-*/rps_cpus
do
	echo 0 > $f
done
