#!/bin/bash
intf=$1
rep=$2

exp_name=RSS
for((i=1;i<=32;i*=2));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		./run_mini_project.sh $dir 1 0 0 $i $intf
	done
done

exp_name=RPS
for((i=1;i<=32;i*=2));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		./run_mini_project.sh $dir 0 1 0 $i $intf
	done
done

exp_name=RFS
for((i=1;i<=32;i*=2));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		./run_mini_project.sh $dir 0 0 1 $i $intf
	done
done

exp_name=RSS+RPS
for((i=1;i<=32;i*=2));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		./run_mini_project.sh $dir 1 1 0 $i $intf
	done
done

exp_name=RSS+RFS
for((i=1;i<=32;i*=2));
do
	for((j=1;j<=$rep;j++));
	do
		dir="$exp_name"_"$i"_"$j"
		echo $dir
		./run_mini_project.sh $dir 1 0 1 $i $intf
	done
done


rm -r ./summaries

python3 merger.py $2
