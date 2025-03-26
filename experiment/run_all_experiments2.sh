#!/bin/bash
intf=$1
rep=$2
conns=$3
starting_conn=${4:-1}

current_path=/home/hema/Custom_Packet_Steering

name="BigMsg_GRO_Sep"

exp_name=RSS
conn=$starting_conn
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

exp_name=RPS
conn=$starting_conn
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 1 0 0 0 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

exp_name=RFS
conn=$starting_conn
for((i=1;i<=$conns;i++));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$conn"_"$j"
		echo $dir
		$current_path/run_mini_project.sh $dir 0 0 1 0 0 $conn $intf 0 4 1 1
	done
	conn=$((conn*2))
done

echo 1 > /sys/module/pkt_steer_module/parameters/risk_reorder
exp_name=IAPS+RFS
conn=$starting_conn
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


echo 600 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 900 > /sys/module/pkt_steer_module/parameters/threshold_low

exp_name=IAPS+LB
conn=$starting_conn
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

echo 200 > /sys/module/pkt_steer_module/parameters/threshold_low
echo 500 > /sys/module/pkt_steer_module/parameters/threshold_low

exp_name=IAPS+RPS
conn=1
for((i=1;i<=$conns;i++));
do
    for((j=1;j<=$rep;j++));
    do
        dir="$exp_name"_"$conn"_"$j"
        echo $dir
        $current_path/run_mini_project.sh $dir 0 0 0 1 3 $conn $intf 0 4 1 1
    done
    conn=$((conn*2))
done
echo 0 > /sys/module/pkt_steer_module/parameters/risk_reorder



rm -r $current_path/summaries


#[DANGER] Change this back
#python3 $current_path/merger.py $2 $((conns + 4)) 1
python3 $current_path/merger.py $2 $conns 1

mkdir $current_path/data/$name

mv $current_path/data/R* $current_path/data/$name
mv $current_path/data/IAPS* $current_path/data/$name
mv $current_path/summaries $current_path/data/$name

