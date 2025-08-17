#!/bin/bash

password_path=~/password

bench() {
make clean
make -j12 KEM_PATH=$1 RSIG_PATH=$2

if [[ "$OSTYPE" == "darwin"* ]]; then
sudo -S <$password_path ./speed_pq_akem > bench_pq_akem_$1_$2.txt
sudo -S <$password_path ./speed_h_akem > bench_h_akem_$1_$2.txt
else
./speed_pq_akem > bench_pq_akem_$1_$2.txt
./speed_h_akem > bench_h_akem_$1_$2.txt
fi
}

rm -f bench*

bench BAT Gandalf_Mitaka
bench BAT Gandalf_Falcon
bench BAT Gandalf_Falcon_C
bench mlkem Gandalf_Mitaka
bench mlkem Gandalf_Falcon
bench mlkem Gandalf_Falcon_C