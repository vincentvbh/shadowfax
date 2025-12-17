#!/bin/bash

rm -rf artifact
mkdir -p artifact
cp -r BAT cycles dh GandalfFalcon GandalfFalconC GandalfMitaka hash mlkem ntru_gen randombytes symmetric artifact/
cp Makefile README.md *.h *.c *.sh artifact/
cd artifact
bash ./clean_up.sh
find . -name '.git*' -exec rm -rf {} \;
cd ../
rm -f artifact.zip
zip -r artifact.zip artifact
