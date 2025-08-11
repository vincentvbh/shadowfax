#!/usr/bin
unifdef -UFNDSA_SSE2 -UFNDSA_AVX2 -UFNDSA_NEON -UFNDSA_RV64D $1 > tmp.c
mv tmp.c $1
