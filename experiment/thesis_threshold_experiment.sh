#!/bin/bash
source my_lib.sh

intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=0

name="Thesis_Thresholds"

pkt_steer="0 0 0 0"
backup=0
queues=4

for((th=35;th<=60;th+=5));
do
exp_name="IAPS-Full-Overload-${th}"

set_iaps $ON
set_backup_core $RPS_BACKUP
set_overload $ON
set_overload_threshold $th
set_reordering $ON
set_util_loadbalance $OFF
set_latency_measures $ON
set_pkt_size_measures $ON
set_backup_choice_monitoring $ON

#echo $exp_name

run_exp $exp_name $rep $conns $exponential

done

rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3 $exponential

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/IAPS* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

