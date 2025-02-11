#!/bin/bash
intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering


name="BigMsg_GRO_Sep_Overload_Tests"

exp_name=RSS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 1 0 0 0 0 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done
# No overload

echo 0 > /sys/module/pkt_steer_module/parameters/activate_overload

exp_name=Custom1-0
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 1 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

exp_name=Custom2-0
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

# Overload 10

echo 1 > /sys/module/pkt_steer_module/parameters/activate_overload
echo 10 > /sys/module/pkt_steer_module/parameters/iq_thresh

exp_name=Custom1-1
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 1 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

exp_name=Custom2-1
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

# Overload 50

echo 1 > /sys/module/pkt_steer_module/parameters/activate_overload
echo 50 > /sys/module/pkt_steer_module/parameters/iq_thresh

exp_name=Custom1-2
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 1 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

exp_name=Custom2-2
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

# Overload 100

echo 1 > /sys/module/pkt_steer_module/parameters/activate_overload
echo 100 > /sys/module/pkt_steer_module/parameters/iq_thresh

exp_name=Custom1-3
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 1 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

exp_name=Custom2-3
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done


rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/Custom* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

