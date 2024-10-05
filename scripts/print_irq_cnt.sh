#!/bin/bash

source /usr/sbin/common_irq_affinity.sh

intf=${1:-ens4}
target=${2:-0}




for i in $( get_irq_list $intf );
do
    echo "$i:"
done 
