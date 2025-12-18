#!/bin/bash

bench_file=bench.txt
bench_latex_file=bench_latex.tex

rm -f $bench_file

bench() {
make clean
make -j12 KEM_PATH=$1 RSIG_PATH=$2

if [[ "$OSTYPE" == "darwin"* ]]; then
echo "Benchmarking post-quantum AKEM with $1 + $2..."
sudo ./speed_pq_akem >> $bench_file
echo "Benchmarking hybrid AKEM with $1 + $2..."
sudo ./speed_h_akem >> $bench_file
else
echo "Benchmarking post-quantum AKEM with $1 + $2..."
./speed_pq_akem >> $bench_file
echo "Benchmarking hybrid AKEM with $1 + $2..."
./speed_h_akem >> $bench_file
fi
}

bench mlkem GandalfFalcon
bench mlkem GandalfFalconC
bench mlkem GandalfMitaka
bench BAT GandalfFalcon
bench BAT GandalfFalconC
bench BAT GandalfMitaka

if ! command -v cc > /dev/null 2>&1
then
    echo "cc cannot be found!"
    exit 1
else
cc get_latex.c -o get_latex; ./get_latex $bench_file $bench_latex_file
fi

