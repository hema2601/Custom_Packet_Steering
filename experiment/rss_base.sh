#!/bin/bash
source my_lib.sh

intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=0

name="Baseline_RSS_16Conn"

pkt_steer="0 0 0 0"
backup=0
queues=9

exp_name="RSS"

set_queues 9
set_sep $ON
set_core_num 18
set_intf $intf

set_rss $ON

run_exp $exp_name $rep $conns $exponential


rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3 $exponential

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/IAPS* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

