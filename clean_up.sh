#!/bin/bash

find . -name 'Makefile' -execdir make clean \;
find . -name '__pycache__' -exec rm -rf {} \;
find . -name '.DS_Store' -exec rm -rf {} \;
find . -name 'get_latex' -exec rm -f {} \;
