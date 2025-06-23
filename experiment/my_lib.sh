ON=1
OFF=0

RPS_BACKUP=3
IDLE_BACKUP=4

ARG_STRING=""

set_rps() {
    pkt_steer="0100"
	ARG_STRING="$ARG_STRING -P $pkt_steer"
}
set_rfs() {
    pkt_steer="0010"
	ARG_STRING="$ARG_STRING -P $pkt_steer"
}
set_iaps() {
    pkt_steer="0001"
	ARG_STRING="$ARG_STRING -P $pkt_steer"
}
set_rss() {
    pkt_steer="1000"
	ARG_STRING="$ARG_STRING -P $pkt_steer"
}
set_mq_rps() {
    pkt_steer="1100"
	ARG_STRING="$ARG_STRING -P $pkt_steer"
}
set_mq_iaps() {
    pkt_steer="1001"
	ARG_STRING="$ARG_STRING -P $pkt_steer"
}
set_backup_core() {
    backup=$1
	ARG_STRING="$ARG_STRING -b $backup"
}

set_queues() {
    queues=$1
	ARG_STRING="$ARG_STRING -q $queues"
}
set_intf() {
    intf=$1
	ARG_STRING="$ARG_STRING -i $intf"
}
set_gro (){
    gro=$1
	ARG_STRING="$ARG_STRING -g $gro"
}
set_sep (){
    sep=$1
	ARG_STRING="$ARG_STRING -s $sep"
}
set_mss (){
    mss=$1
	ARG_STRING="$ARG_STRING -m $mss"
}
set_core_start (){
    core_start=$1
	ARG_STRING="$ARG_STRING -S $core_start"
}
set_core_num (){
    core_num=$1
	ARG_STRING="$ARG_STRING -C $core_num"
}
set_time (){
    t=$1
	ARG_STRING="$ARG_STRING -t $t"
}
set_overload_threshold() {
    overload_thresh=$1
    echo $overload_thresh > /sys/module/pkt_steer_module/parameters/iq_thresh
}
set_overload() {
    overload=$1
    echo $overload > /sys/module/pkt_steer_module/parameters/activate_overload
}
set_reordering() {
    reorder=$1
    echo $reorder > /sys/module/pkt_steer_module/parameters/risk_reorder
}
set_util_loadbalance() {
    util_lb=$((!$1))
    echo $util_lb
    echo $util_lb > /sys/module/pkt_steer_module/parameters/deactivate_util_lb
}
set_latency_measures() {
    lat=$1
    echo $lat > /sys/module/pkt_steer_module/parameters/latency_measures
}
set_pkt_size_measures(){
    pkt_size=$1
    echo $pkt_size > /sys/module/pkt_steer_module/parameters/pkt_histo_measures
}
set_backup_choice_monitoring(){
    backup_choice=$1
    echo $backup_choice > /sys/module/pkt_steer_module/parameters/check_backup_choice
}

run_exp () { #1: Name 2:Rep 3:Conn 4:Exp
	rep=$2
	conns=$3
	as_exponential=$4
    conn=1
    for((i=1;i<=$conns;i++));
    do
		if [[ "$as_exponential" == 1 ]]
		then
			marker=$conn
		else
			marker=$i
		fi
        for((j=1;j<=$rep;j++));
        do
            dir="$1"_"$marker"_"$j"
            echo $dir
			echo $ARG_STRING
			$current_path/run_wrapper.sh $ARG_STRING -n $dir -c $marker
            #$current_path/run_mini_project.sh $dir $pkt_steer $backup $marker $intf 0 $queues 1 1
        done
        conn=$((conn*2))
    done

}


