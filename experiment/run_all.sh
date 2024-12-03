#!/bin/bash
intf=$1
rep=$2
conns=$3


 ./run_all_experiments.sh $1 $2 $3 && ./run_all_experiments2.sh $1 $2 $3 &&./run_all_experiments3.sh $1 $2 $3 &&./run_all_experiments4.sh $1 $2 $3 &&./run_all_experiments5.sh $1 $2 $3 &&./run_all_experiments6.sh $1 $2 $3 &&./run_all_experiments7.sh $1 $2 $3 
