#!/bin/bash

current_path=/home/hema/mini_project_rfs/
remote_client_addr=hema@115.145.178.17
server_ip=10.0.0.3

exp_name=${1:-exp}
rss=${2:-1}
rps=${3:-0}
rfs=${4:-0}
conns=${5:-6}
intf=${6:-ens4np0}
num_queue=${7:-8}
core_start=${8:-0}
core_num=${9:-8}
time=${10:-10} 

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

