#!/bin/bash
source my_lib.sh

intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=1

name="Thesis_MQ_Test_Large"

pkt_steer="0 0 0 0"
backup=0
queues=2

exp_name="RPS"

set_mq_rps $ON
set_latency_measures $ON
set_pkt_size_measures $ON

run_exp $exp_name $rep $conns $exponential

exp_name="IAPS-Full-Overload-20"

set_mq_iaps $ON
set_backup_core $RPS_BACKUP
set_overload $ON
set_overload_threshold 20 
set_reordering $ON
set_util_loadbalance $OFF
set_latency_measures $ON
set_pkt_size_measures $ON
set_backup_choice_monitoring $ON
 
run_exp $exp_name $rep $conns $exponential

#exp_name="IAPS-Idle-Core-20"
#
#set_mq_iaps $ON
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

