#!/bin/bash

set -Ee

: ${LOG_PATH:=log}
: ${LOG_FILE:="$LOG_PATH"/test_log.txt}

test_dh_akem_call() {
make clean
make -j12
cat << EOF | tee -a $LOG_FILE
================================
Testing pre-quantum AKEM...
================================
EOF
./test_dh_akem >> $LOG_FILE
}

test_pq_akem_call() {
make clean
make -j12 KEM_PATH=$1 RSIG_PATH=$2
cat << EOF | tee -a $LOG_FILE
================================================================
Testing post-quantum AKEM with $1 + $2...
================================================================
EOF
./test_pq_akem >> $LOG_FILE
}

test_h_akem_call() {
make clean
make -j12 KEM_PATH=$1 RSIG_PATH=$2
cat << EOF | tee -a $LOG_FILE
================================================================
Testing hybrid AKEM with $1 + $2...
================================================================
EOF
./test_h_akem >> $LOG_FILE
}

rm -f $LOG_FILE

test_dh_akem_call

test_pq_akem_call mlkem GandalfFalcon
test_pq_akem_call mlkem GandalfFalconC
test_pq_akem_call mlkem GandalfMitaka
test_pq_akem_call BAT GandalfFalcon
test_pq_akem_call BAT GandalfFalconC
test_pq_akem_call BAT GandalfMitaka

test_h_akem_call mlkem GandalfFalcon
test_h_akem_call mlkem GandalfFalconC
test_h_akem_call mlkem GandalfMitaka
test_h_akem_call BAT GandalfFalcon
test_h_akem_call BAT GandalfFalconC
test_h_akem_call BAT GandalfMitaka



