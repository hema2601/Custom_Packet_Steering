#!/bin/bash

source /usr/sbin/common_irq_affinity.sh

intf=${1:-ens4}
target=${2:-0}


echo "Disabling irqbalance service..."
service irqbalance stop

echo "Redirecting interrupts associated with $intf to CPU#$target"

core=$target

for i in $( get_irq_list $intf );
do
    echo $(core_to_affinity $core)  > /proc/irq/$i/smp_affinity;
    cat /proc/irq/$i/smp_affinity;
	core=$((core + 1))
done 
