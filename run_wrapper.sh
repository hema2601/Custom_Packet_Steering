#!/bin/bash


# [DEFAULTS]

EXP_NAME=exp 		# -n
RSS=1
RPS=0
RFS=0
IAPS=0
# -P (bitmask) 1   0   0   0
#			   ^   ^   ^   ^
#			   |   |   |   |
#			  RSS RPS RFS IAPS

Backup_Core=1		# -b
Conns=6 			# -c
INTF=ens4np0		# -i
IAPS_BUSY_LIST=0	# DEPRECATED
NUM_QUEUE=8			# -q
GRO=1				# -G
SEPARATE=0			# -s
MSS=1460			# -m
CORE_START=0		# -S
CORE_NUM=8			# -C
TIME=10				# -t

current_path=/home/hema/Custom_Packet_Steering

while getopts ":P:b:c:i:q:G:s:m:S:C:t:n:" opt; do
  case $opt in
    P) BITMASK="$OPTARG"
	   RSS=$(( (2#$BITMASK & 2#1000) >> 3))
	   RPS=$(( (2#$BITMASK & 2#0100) >> 2))
	   RFS=$(( (2#$BITMASK & 2#0010) >> 1))
	   IAPS=$((2#$BITMASK & 2#0001))
    ;;
    b) Backup_Core="$OPTARG"
    ;;
    c) Conns="$OPTARG"
    ;;
    i) INTF="$OPTARG"
    ;;
    q) NUM_QUEUE="$OPTARG"
    ;;
    G) GRO="$OPTARG"
    ;;
    s) SEPARATE="$OPTARG"
    ;;
    m) MSS="$OPTARG"
    ;;
    S) CORE_START="$OPTARG"
    ;;
    C) CORE_NUM="$OPTARG"
    ;;
    t) TIME="$OPTARG"
    ;;
    n) EXP_NAME="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
    exit 1
    ;;
  esac

  case $OPTARG in
    -*) echo "Option $opt needs a valid argument"
    exit 1
    ;;
  esac
done

#echo $EXP_NAME $RSS $RPS $RFS $IAPS $Backup_Core $Conns $INTF $IAPS_BUSY_LIST $NUM_QUEUE $GRO $SEPARATE $MSS $CORE_START $CORE_NUM $TIME
$current_path/run_mini_project.sh $EXP_NAME $RSS $RPS $RFS $IAPS $Backup_Core $Conns $INTF $IAPS_BUSY_LIST $NUM_QUEUE $GRO $SEPARATE $MSS $CORE_START $CORE_NUM $TIME

