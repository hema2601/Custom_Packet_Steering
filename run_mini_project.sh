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
num_queue=${9:-8}
core_start=${10:-0}
core_num=${11:-8}
time=${12:-10} 

IPERF_BIN=iperf3

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
else
	echo "Disable RSS"
	ethtool -L $intf combined 1
fi

# Setup rfs/rps
if [[ "$rps" == "1" ]]
then
	#enable rps
	echo "Enable RPS"
	$current_path/scripts/enable_rps.sh $intf $core_start $core_num
else
	echo "Disable RPS"
	$current_path/scripts/disable_rps.sh $intf

fi

if [[ "$rfs" == "1" ]]
then
	#enable rfs
	echo "Enable RFS"
	$current_path/scripts/enable_rfs.sh $intf
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
	$current_path/scripts/enable_rfs.sh $intf
	$current_path/scripts/enable_rps.sh $intf $core_start $core_num
	echo $backup_core > /sys/module/pkt_steer_module/parameters/choose_backup_core
else
	echo "Disable Custom"
	#rmmod pkt_steer_module
	if [[ "$rfs" == "0" ]]
	then
		$current_path/scripts/disable_rfs.sh $intf
	fi
fi


	
# Set Mappings
$current_path/scripts/set_affinity.sh $intf $core_start

# Get "Before" Values
$current_path/scripts/before.sh $intf

# Run iperf3
taskset -c "$core_start-$((core_start + core_num - 1))" $IPERF_BIN -s -1 -J > $current_path/iperf.json & ssh $remote_client_addr "iperf3 -c ${server_ip} -P ${conns} > /dev/null"&
IPERF_PID=$!

# Wait for iperf3 to exit [Optional]
tail --pid=$IPERF_PID -f /dev/null
echo "Iperf ended"

# Get "After" Values
$current_path/scripts/after.sh $exp_name $intf

#perf json to data folder
mv $current_path/iperf.json $current_path/data/$exp_name/

# Apply File Transformation
python3 file_formatter.py $exp_name IRQ SOFTIRQ PACKET_CNT IPERF SOFTNET

if [[ "$custom" == "1" ]]
then
python3 file_formatter.py $exp_name PKT_STEER
#rmmod pkt_steer_module
fi 


