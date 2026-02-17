#!/bin/bash

: ${LOG_PATH:=log}
: ${LOG_FILE:="$LOG_PATH"/bench_log.txt}
: ${LATEX_FILE:="$LOG_PATH"/bench_latex.tex}

bench() {
make clean
make -j12 KEM_PATH=$1 RSIG_PATH=$2

if [[ "$OSTYPE" == "darwin"* ]]; then
cat << EOF | tee -a $LOG_FILE
================================================================
Benchmarking post-quantum AKEM with $1 + $2...
================================================================
EOF
sudo ./speed_pq_akem >> $LOG_FILE
cat << EOF | tee -a $LOG_FILE
================================================================
Benchmarking hybrid AKEM with $1 + $2...
================================================================
EOF
sudo ./speed_h_akem >> $LOG_FILE
else
cat << EOF | tee -a $LOG_FILE
================================================================
Benchmarking post-quantum AKEM with $1 + $2...
================================================================
EOF
./speed_pq_akem >> $LOG_FILE
cat << EOF | tee -a $LOG_FILE
================================================================
Benchmarking hybrid AKEM with $1 + $2...
================================================================
EOF
./speed_h_akem >> $LOG_FILE
fi
}

rm -f $LOG_FILE

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
cc get_latex.c -o get_latex; ./get_latex $LOG_FILE $LATEX_FILE
fi

