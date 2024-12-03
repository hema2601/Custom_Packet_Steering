#!/bin/bash

base_cpu=${1:-0}
max_cpu=${2:-8}
backup_core=${3:-1}

echo 1 > /sys/module/pkt_steer_module/parameters/custom_toggle 
echo $base_cpu > /sys/module/pkt_steer_module/parameters/base_cpu 
echo $max_cpu > /sys/module/pkt_steer_module/parameters/max_cpus 
echo $backup_core > /sys/module/pkt_steer_module/parameters/choose_backup_core
