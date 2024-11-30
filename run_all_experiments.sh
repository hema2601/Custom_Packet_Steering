#!/bin/bash
intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering

exp_name=RSS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		./run_mini_project.sh $dir 1 0 0 0 0 $i $intf 0 2 0
	done
	conn=$((conn*2))
done

exp_name=RPS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		./run_mini_project.sh $dir 0 1 0 0 0 $i $intf
	done
	conn=$((conn*2))
done

exp_name=RFS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		./run_mini_project.sh $dir 0 0 1 0 0 $i $intf
	done
	conn=$((conn*2))
done

exp_name=RSS+RPS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		#./run_mini_project.sh $dir 1 1 0 0 0 $i $intf 0 2 0
	done
	conn=$((conn*2))
done

exp_name=RSS+RFS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
#		./run_mini_project.sh $dir 1 0 1 0 0 $i $intf
	done
	conn=$((conn*2))
done

#make -C $current_path/module
#insmod $current_path/module/pkt_steer_module.ko

exp_name=Custom1
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		./run_mini_project.sh $dir 0 0 0 1 4 $i $intf 
	done
	conn=$((conn*2))
done

exp_name=Custom2
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		#./run_mini_project.sh $dir 0 0 0 1 1 $i $intf 1
	done
	conn=$((conn*2))
done

exp_name=Custom3
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
#		./run_mini_project.sh $dir 0 0 0 1 3 $i $intf
	done
	conn=$((conn*2))
done

exp_name=RSS+Custom1
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		#./run_mini_project.sh $dir 1 0 0 1 4 $i $intf 0 2 0
	done
	conn=$((conn*2))
done

exp_name=RSS+Custom2
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
#		./run_mini_project.sh $dir 1 0 0 1 2 $i $intf
	done
	conn=$((conn*2))
done

exp_name=RSS+Custom3
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
#		./run_mini_project.sh $dir 1 0 0 1 3 $i $intf
	done
	conn=$((conn*2))
done

#rmmod pkt_steer_module


rm -r ./summaries

python3 merger.py $2 $3
