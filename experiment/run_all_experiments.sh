#!/bin/bash
intf=$1
rep=$2
conns=$3

current_path=/home/hema/Custom_Packet_Steering


name="BigMsg_GRO_Comb"

exp_name=RSS
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 1 0 0 0 0 $conn $intf
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
		$current_path/run_mini_project.sh $dir 0 1 0 0 0 $conn $intf
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
		$current_path/run_mini_project.sh $dir 0 0 1 0 0 $conn $intf 
	done
	conn=$((conn*2))
done

exp_name=Custom1
conn=1
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 0 1 1 $conn $intf
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
		$current_path/run_mini_project.sh $dir 0 0 0 1 4 $conn $intf 
	done
	conn=$((conn*2))
done

rm -r $current_path/summaries

python3 $current_path/merger.py $2 $3

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/Custom* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

