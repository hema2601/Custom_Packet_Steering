#!/bin/bash

current_path=/home/hema/Custom_Packet_Steering/
remote_client_addr=hema@115.145.178.17
server_ip=10.0.0.3

exp_name=${1:-exp}
rss=${2:-1}
rps=${3:-0}
rfs=${4:-0}
custom=${5:-0}
backup_core=${6:-1}
conns=${7:-6}
intf=${8:-ens4np0}
iaps_busy_list=${9:-0}
num_queue=${10:-8}
gro=${11:-1}
core_start=${12:-0}
core_num=${13:-8}
time=${14:-10} 

IPERF_BIN=iperf3

PERF_BIN=$current_path/linux-6.10.8/tools/perf/perf

type="Unknown"

if command -v iperf3_napi &> /dev/null
then
	IPERF_BIN=iperf3_napi
    echo "Using custom iperf3"
fi

#create directory
mkdir $current_path/data/$exp_name

set -e
# Disable irqbalancer
service irqbalance stop

# Setup the Queues
echo $rss
if [[ "$rss" == "1" ]]
then
	echo "Enable RSS"
	ethtool -L $intf combined $num_queue
	type="RSS"
else
	echo "Disable RSS"
	ethtool -L $intf combined 1
	num_queue=1
fi

# Setup rfs/rps
if [[ "$rps" == "1" ]]
then
	#enable rps
	echo "Enable RPS"
	$current_path/scripts/enable_rps.sh $intf $(( core_start + num_queue )) $((core_num - num_queue))
	type="RPS"
else
	echo "Disable RPS"
	$current_path/scripts/disable_rps.sh $intf

fi

if [[ "$rfs" == "1" ]]
then
	#enable rfs
	echo "Enable RFS"
	$current_path/scripts/enable_rfs.sh $intf
	type="RFS"
else
	echo "Disable RFS"
	$current_path/scripts/disable_rfs.sh $intf

fi

if [[ "$custom" == "1" ]]
then
	#enable rfs
	echo "Enable Custom (based on RFS)"
	#make -C $current_path/module
	#insmod $current_path/module/pkt_steer_module.ko
	#$current_path/scripts/enable_rfs.sh $intf
	$current_path/scripts/enable_rps.sh $intf $((core_start + num_queue)) $((core_num - num_queue))	
    echo $backup_core > /sys/module/pkt_steer_module/parameters/choose_backup_core
	echo $iaps_busy_list > /sys/module/pkt_steer_module/parameters/list_position
	echo $((core_start + num_queue)) > /sys/module/pkt_steer_module/parameters/base_cpu
	echo $((core_num - num_queue)) > /sys/module/pkt_steer_module/parameters/max_cpus
	echo 1 > /sys/module/pkt_steer_module/parameters/custom_toggle
	type="IAPS"
else
	echo "Disable Custom"
	echo 0 > /sys/module/pkt_steer_module/parameters/custom_toggle

	#rmmod pkt_steer_module
	if [[ "$rfs" == "0" ]]
	then
		$current_path/scripts/disable_rfs.sh $intf
	fi
fi

if [[ "$gro" == "1" ]]
then
	ethtool -K $intf gro on
else
	ethtool -K $intf gro off
fi


	
# Set Mappings
$current_path/scripts/set_affinity.sh $intf $core_start

# Get "Before" Values
$current_path/scripts/before.sh $intf

# Run perf
$PERF_BIN record -C $core_start-$((core_start + core_num - 1)) -o $current_path/perf.json &
PERF_PID=$!

# Run iperf3
taskset -c "$core_start-$((core_start + core_num - 1))" $IPERF_BIN -s -1 -J > $current_path/iperf.json & ssh $remote_client_addr "iperf3 -c ${server_ip} -P ${conns} -M 100 > /dev/null"&
IPERF_PID=$!

# Perform Latency Test 
sleep 3
if test -f /proc/latency_module; then
	python3 latency_accessor.py $current_path/data/$exp_name/latency $type
fi
if test -f /proc/ipi_lat_module; then
	python3 $current_path/module/ipi_latency_module/accessor.py $current_path/data/$exp_name/latency $type
fi

# Wait for iperf3 to exit [Optional]
tail --pid=$IPERF_PID -f /dev/null
echo "Iperf ended"
kill $PERF_PID
tail --pid=$PERF_PID -f /dev/null

# Get "After" Values
$current_path/scripts/after.sh $exp_name $intf

#move iperf and perf json to data folder
mv $current_path/iperf.json $current_path/data/$exp_name/
$PERF_BIN report --stdio --stdio-color never --percent-limit 0.01 -i $current_path/perf.json > $current_path/data/$exp_name/perf.json
rm $current_path/perf.json

# Apply File Transformation
python3 file_formatter.py $exp_name IRQ SOFTIRQ PACKET_CNT IPERF SOFTNET PROC_STAT PKT_STEER PERF

#if [[ "$custom" == "1" ]]
#then
#python3 file_formatter.py $exp_name PKT_STEER
#rmmod pkt_steer_module
#fi 


