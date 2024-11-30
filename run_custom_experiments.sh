#!/bin/bash

intf=$1
rep=$2
conns=$3

rss=0
rfs=0
rps=0
iaps=0
bc=1

mss=1460
gro=1
sep=1

run_exp () {

	exp_name=$1

	conn=1
	for((i=1;i<=$conns;i++));
	do
		for((j=1;j<=$rep;j++));
		do
			dir="$exp_name"_"$conn"_"$j"
			echo $dir
			echo $intf
			./run_mini_project.sh $dir $rss $rps $rfs $iaps $bc $conn $intf 0 4 $gro $sep $mss 
		done
		conn=$((conn*2))
	done

}

run_all_once() {
	rss=1
	run_exp "RSS"
	rss=0
	
	rps=1
	run_exp "RPS"
	rps=0
	
	rfs=1
	run_exp "RFS"
	rfs=0
	
	iaps=1
	bc=1
	run_exp "Custom1"
	bc=4
	run_exp "Custom2"
	iaps=0

	local dir=$current_path/data/$1

	mkdir $dir
	echo "Created $dir"

	rm -r ./summaries

	python3 $current_path/merger.py $rep $conns

	mv $current_path/data/R* $dir
	mv $current_path/data/Custom* $dir
	mv ./summaries $dir

}

current_path=/home/hema/Custom_Packet_Steering

# Big message, GRO, combined

mss=1460
gro=1
sep=0
run_all_once "BigMsg_GRO_Comb"

# Small message, GRO, separate

mss=100
gro=1
sep=1
run_all_once "SmlMsg_GRO_Sep"

# Big message, GRO, separate

mss=1460
gro=1
sep=1
run_all_once "BigMsg_GRO_Sep"

# Small message, GRO, combined

mss=100
gro=1
sep=0
run_all_once "SmlMsg_GRO_Comb"

# Big message, no GRO, separate

mss=1460
gro=0
sep=1
run_all_once "BigMsg_NoGRO_Sep"

# Small message, no GRO, separate

mss=100
gro=0
sep=1
run_all_once "SmlMsg_NoGRO_Sep"

#rm -r ./summaries

#python3 merger.py $2 $3
