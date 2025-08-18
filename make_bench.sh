#!/bin/bash

bench_file=bench.txt
bench_latex_file=bench_latex.tex

rm -f $bench_file

bench() {
make clean
make -j12 KEM_PATH=$1 RSIG_PATH=$2

if [[ "$OSTYPE" == "darwin"* ]]; then
sudo ./speed_pq_akem >> $bench_file
sudo ./speed_h_akem >> $bench_file
else
./speed_pq_akem >> $bench_file
./speed_h_akem >> $bench_file
fi
}

bench BAT GandalfMitaka
bench BAT GandalfFalcon
bench BAT GandalfFalconC
bench mlkem GandalfMitaka
bench mlkem GandalfFalcon
bench mlkem GandalfFalconC

gcc get_latex.c -o get_latex; ./get_latex $bench_file $bench_latex_file


