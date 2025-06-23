#!/bin/bash
source my_lib.sh

intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=1

name="Thesis_Test_Suite_Large"

pkt_steer="0 0 0 0"
backup=0
queues=4

exp_name="IAPS-Base"

set_iaps $ON
set_backup_core $RPS_BACKUP
set_overload $OFF
set_reordering $OFF
set_util_loadbalance $OFF
set_latency_measures $ON
set_pkt_size_measures $ON
set_backup_choice_monitoring $ON

run_exp $exp_name $rep $conns $exponential

exp_name="IAPS-Basic-Overload-40"

set_iaps $ON
set_backup_core $RPS_BACKUP
set_overload $ON
set_overload_threshold 40 
set_reordering $OFF
set_util_loadbalance $OFF
set_latency_measures $ON
set_pkt_size_measures $ON
set_backup_choice_monitoring $ON

run_exp $exp_name $rep $conns $exponential

exp_name="IAPS-Full-Overload-40"

set_iaps $ON
set_backup_core $RPS_BACKUP
set_overload $ON
set_overload_threshold 40 
set_reordering $ON
set_util_loadbalance $OFF
set_latency_measures $ON
set_pkt_size_measures $ON
set_backup_choice_monitoring $ON

run_exp $exp_name $rep $conns $exponential
#
#exp_name="IAPS-Idle-Core-20"
#
#set_iaps $ON
#set_backup_core $IDLE_BACKUP
#set_overload $ON
#set_overload_threshold 20 
#set_reordering $ON
#set_util_loadbalance $OFF
#set_latency_measures $ON
#set_pkt_size_measures $ON
#set_backup_choice_monitoring $ON
#
#run_exp $exp_name $rep $conns $exponential

rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3 $exponential

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/IAPS* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

