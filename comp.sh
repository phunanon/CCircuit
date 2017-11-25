#!/bin/bash

[ -z $1 ] && g++ CCircuit.cpp -o CCircuit.out --std=c++11 -g && exit 1
g++ CCircuit.cpp -o CCircuit.out --std=c++11 -static-libstdc++ -O3
