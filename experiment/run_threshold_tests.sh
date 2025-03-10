#!/bin/bash
intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=0

name="BigMsg_GRO_Sep_Thresholds"

echo 200 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 500 > /sys/module/pkt_steer_module/parameters/threshold_low

exp_name=Custom2-1
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $i $intf 0 4 1 1
	done
	conn=$((conn*2))
done

echo 400 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 700 > /sys/module/pkt_steer_module/parameters/threshold_low

exp_name=Custom2-2
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $i $intf 0 4 1 1
	done
	conn=$((conn*2))
done

echo 600 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 900 > /sys/module/pkt_steer_module/parameters/threshold_low

exp_name=Custom2-3
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $i $intf 0 4 1 1
	done
	conn=$((conn*2))
done
rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3 $exponential

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/Custom* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

echo 200 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 500 > /sys/module/pkt_steer_module/parameters/threshold_low
