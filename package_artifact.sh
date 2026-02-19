#!/bin/bash

rm -rf artifact
mkdir -p artifact
cp -r latex log src \
      artifact/
cp *.md bench_everything.sh clean_up.sh test_everything.sh artifact/
cd artifact
bash ./clean_up.sh
find . -name '.git*' -exec rm -rf {} \;
cd ../
rm -f artifact.zip
zip -r artifact.zip artifact
