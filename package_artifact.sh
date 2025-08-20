#!/bin/bash

# external_path=~/Desktop/git/private/AKEM_C

rm -rf artifact
mkdir -p artifact
cp -r BAT cycles dh GandalfFalcon GandalfFalconC GandalfMitaka hash mlkem ntru_gen randombytes symmetric artifact/
cp Makefile README.md *.h *.c *.sh artifact/
# cp -r $external_path/submission_files/* artifact/
cd artifact
find . -name 'Makefile' -execdir make clean \;
find . -name '.git*' -exec rm -rf {} \;
find . -name '__pycache__' -exec rm -rf {} \;
find . -name '.DS_Store' -exec rm -rf {} \;
rm -f get_latex
cd ../
rm -f artifact.zip
zip -r artifact.zip artifact
