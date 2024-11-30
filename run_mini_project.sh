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
separate=${12:-0}
core_start=${13:-0}
core_num=${14:-8}
time=${15:-10} 

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

# Calculate Irq/PackProc/App Cores

if [[ "$separate" == "1" ]]
then

	IRQ_CORE=$core_start
	IRQ_CORE_NUM=$num_queue

	if [[ ( "$rps" == "1" ) || ( "$custom" == "1") ]]
	then	
		PP_CORE=$((core_start + IRQ_CORE_NUM))
		PP_CORE_NUM=$(( (core_num - IRQ_CORE_NUM) / 2))
		APP_CORE=$((core_start + IRQ_CORE_NUM + PP_CORE_NUM))
		APP_CORE_NUM=$((core_num - IRQ_CORE_NUM - PP_CORE_NUM))
	else
		PP_CORE=0
		PP_CORE_NUM=0
		APP_CORE=$((core_start))
		APP_CORE_NUM=$((core_num))
	fi

	echo $IRQ_CORE
	echo $IRQ_CORE_NUM
	echo $PP_CORE
	echo $PP_CORE_NUM
	echo $APP_CORE
	echo $APP_CORE_NUM


else
	IRQ_CORE=$core_start
	IRQ_CORE_NUM=$num_queue
	PP_CORE=$core_start
	PP_CORE_NUM=$core_num
	APP_CORE=$core_start
	APP_CORE_NUM=$core_num
fi


# Setup rfs/rps
if [[ "$rps" == "1" ]]
then
	#enable rps
	echo "Enable RPS"

	$current_path/scripts/enable_rps.sh $intf $PP_CORE $PP_CORE_NUM

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
	
	echo $PP_CORE > /sys/module/pkt_steer_module/parameters/base_cpu
	echo $PP_CORE_NUM > /sys/module/pkt_steer_module/parameters/max_cpus
	
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
if [[ "$separate" == "1" ]]
then
	$PERF_BIN record -C $IRQ_CORE-$((IRQ_CORE + IRQ_CORE_NUM - 1)) -o $current_path/perf_irq.json &
	IRQPERF_PID=$!
	if [[ "$PP_CORE_NUM" != "0" ]]
	then
		$PERF_BIN record -C $PP_CORE-$((PP_CORE + PP_CORE_NUM - 1)) -o $current_path/perf_pp.json &
		PPPERF_PID=$!
	fi
	$PERF_BIN record -C $APP_CORE-$((APP_CORE + APP_CORE_NUM - 1)) -o $current_path/perf_app.json &
	APPPERF_PID=$!
fi

$PERF_BIN record -C $core_start-$((core_start + core_num - 1)) -o $current_path/perf.json &
PERF_PID=$!

# Run iperf3
taskset -c "$APP_CORE-$((APP_CORE + APP_CORE_NUM - 1))" $IPERF_BIN -s -1 -J > $current_path/iperf.json & ssh $remote_client_addr "iperf3 -c ${server_ip} -P ${conns} > /dev/null"&
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
if [[ "$separate" == 1 ]]
then
	kill $IRQPERF_PID
	tail --pid=$IRQPERF_PID -f /dev/null
	if [[ "$PP_CORE_NUM" != "0" ]]
	then
		kill $PPPERF_PID
		tail --pid=$PPPERF_PID -f /dev/null
	fi
	kill $APPPERF_PID
	tail --pid=$APPPERF_PID -f /dev/null

fi


kill $PERF_PID
tail --pid=$PERF_PID -f /dev/null

# Get "After" Values
$current_path/scripts/after.sh $exp_name $intf

#move iperf json to data folder
mv $current_path/iperf.json $current_path/data/$exp_name/

#deal with perf output

echo "" > $current_path/data/$exp_name/perf.json 
if [[ "$separate" == 1 ]]
then
	if test -f perf_irq.json
	then 
		echo "TYPE	IRQ" >> $current_path/data/$exp_name/perf.json 
		$PERF_BIN report --stdio --stdio-color never --percent-limit 0.01 -i $current_path/perf_irq.json >> $current_path/data/$exp_name/perf.json
	fi

	if test -f perf_pp.json
	then 
		echo "TYPE	PP" >> $current_path/data/$exp_name/perf.json 
		$PERF_BIN report --stdio --stdio-color never --percent-limit 0.01 -i $current_path/perf_pp.json >> $current_path/data/$exp_name/perf.json
	fi

	if test -f perf_app.json
	then 
		echo "TYPE	APP" >> $current_path/data/$exp_name/perf.json 
		$PERF_BIN report --stdio --stdio-color never --percent-limit 0.01 -i $current_path/perf_app.json >> $current_path/data/$exp_name/perf.json
	fi

fi

echo "TYPE	FULL" >> $current_path/data/$exp_name/perf.json 
$PERF_BIN report --stdio --stdio-color never --percent-limit 0.01 -i $current_path/perf.json >> $current_path/data/$exp_name/perf.json



#$PERF_BIN report --stdio --stdio-color never --percent-limit 0.01 -i $current_path/perf.json > $current_path/data/$exp_name/perf.json
rm $current_path/perf*.json


# Apply File Transformation
#python3 file_formatter.py $exp_name IRQ SOFTIRQ PACKET_CNT IPERF SOFTNET PROC_STAT PKT_STEER
python3 file_formatter.py $exp_name IRQ SOFTIRQ PACKET_CNT IPERF SOFTNET PROC_STAT PKT_STEER PERF


#if [[ "$custom" == "1" ]]
#then
#python3 file_formatter.py $exp_name PKT_STEER
#rmmod pkt_steer_module
#fi 


