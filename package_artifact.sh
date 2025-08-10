#!/bin/bash

external_path=~/Desktop/git/private/AKEM_C

rm -rf artifact
mkdir -p artifact/code
cp -r BAT cycles dh Gandalf hash ntru_gen randombytes symmetric artifact/code/
cp Makefile README.md *.h *.c artifact/code/
# cp -r $external_path/submission_files/* artifact/
cd artifact/code
find . -name 'Makefile' -execdir make clean \;
find . -name '.git*' -exec rm -rf {} \;
find . -name '__pycache__' -exec rm -rf {} \;
find . -name '.DS_Store' -exec rm -rf {} \;
cd ../../
rm -f artifact.zip
zip -r artifact.zip artifact
