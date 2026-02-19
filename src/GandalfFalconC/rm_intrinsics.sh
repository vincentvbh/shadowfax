#!/bin/bash

clean_ifdef (){
unifdef -UTARGET_NEON -UTARGET_SSE2 -UFNDSA_SSE2 -UFNDSA_AVX2 -UFNDSA_NEON -UFNDSA_RV64D $1 > tmp.c
mv tmp.c $1
}

for i in *.[ch];
do
    clean_ifdef $i
done
