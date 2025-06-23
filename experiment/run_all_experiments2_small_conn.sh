#!/bin/bash
intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exponential=0

name="BigMsg_GRO_Sep_NonExp"

#exp_name=RSS
#conn=1
#for((i=1;i<=$conns;i++));
#do
#	for((j=1;j<=$rep;j++));
#	do
#		dir="$exp_name"_"$i"_"$j"
#		echo $dir
#		$current_path/run_mini_project.sh $dir 1 0 0 0 0 $i $intf 0 4 1 1
#	done
#	conn=$((conn*2))
#done
#
exp_name=RPS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		echo 1 > /sys/module/pkt_steer_module/parameters/latency_measures
		$current_path/run_mini_project.sh $dir 0 1 0 0 0 $i $intf 0 4 1 1
		echo 0 > /sys/module/pkt_steer_module/parameters/latency_measures
	done
	conn=$((conn*2))
done
#
#exp_name=RFS
#conn=1
#for((i=1;i<=$conns;i++));
#do
#	for((j=1;j<=$rep;j++));
#	do
#		dir="$exp_name"_"$i"_"$j"
#		echo $dir
#		$current_path/run_mini_project.sh $dir 0 0 1 0 0 $i $intf 0 4 1 1
#	done
#	conn=$((conn*2))
#done

echo 1 > /sys/module/pkt_steer_module/parameters/risk_reorder
#exp_name=IAPS+RFS
#conn=1
#for((i=1;i<=$conns;i++));
#do
#	for((j=1;j<=$rep;j++));
#	do
#		dir="$exp_name"_"$i"_"$j"
#		echo $dir
#		$current_path/run_mini_project.sh $dir 0 0 0 1 1 $i $intf 0 4 1 1
#	done
#	conn=$((conn*2))
#done


echo 600 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 900 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 1 > /sys/module/pkt_steer_module/parameters/deactivate_util_lb
echo 1 > /sys/module/pkt_steer_module/parameters/iq_thresh

exp_name=IAPS+LB
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		echo 1 > /sys/module/pkt_steer_module/parameters/latency_measures
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $i $intf 0 4 1 1
		echo 0 > /sys/module/pkt_steer_module/parameters/latency_measures
	done
	conn=$((conn*2))
done

echo 0 > /sys/module/pkt_steer_module/parameters/deactivate_util_lb
echo 200 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 500 > /sys/module/pkt_steer_module/parameters/threshold_low

exp_name=IAPS+RPS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		echo 1 > /sys/module/pkt_steer_module/parameters/latency_measures
		$current_path/run_mini_project.sh $dir 0 0 0 1 3 $i $intf 0 4 1 1
		echo 0 > /sys/module/pkt_steer_module/parameters/latency_measures
	done
	conn=$((conn*2))
done
echo 0 > /sys/module/pkt_steer_module/parameters/risk_reorder

echo 20 > /sys/module/pkt_steer_module/parameters/iq_thresh
rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3 $exponential

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/IAPS* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

