#!/bin/bash
source my_lib.sh

intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=0

name="Main_Baseline"

pkt_steer="0 0 0 0"
backup=0
queues=4

exp_name="RSS"

set_rss $ON
set_latency_measures $ON
set_pkt_size_measures $ON

run_exp $exp_name $rep $conns $exponential

exp_name="RPS"

set_rps $ON
set_latency_measures $ON
set_pkt_size_measures $ON

run_exp $exp_name $rep $conns $exponential

#exp_name="RFS"

#set_rfs $ON
#set_latency_measures $ON
#set_pkt_size_measures $ON

#run_exp $exp_name $rep $conns $exponential

rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3 $exponential

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/IAPS* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

