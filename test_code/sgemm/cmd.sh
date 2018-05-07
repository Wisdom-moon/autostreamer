#!/bin/bash
../../bin/omp2hs sgemm_kernel.cpp -- -fopenmp -I/home/moon/local/lib/clang/5.0.1/include/ -I./
