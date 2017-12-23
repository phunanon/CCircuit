#!/bin/bash

#Compile as either debug or supply any argument to release
[ -z $1 ] && g++ src/*.hpp src/*.cpp -o CCircuit.out --std=c++11 -g && exit 1
g++ src/*.hpp src/*.cpp -o dir/CCircuit.out --std=c++11 -static-libstdc++ -O3
