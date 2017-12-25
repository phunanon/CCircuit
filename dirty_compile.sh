#!/bin/bash

#Compile as either debug or supply any argument to release
[ -z $1 ] && g++ src/*.hpp src/*.cpp -o CCircuit --std=c++17 -g && exit 1
g++ src/*.hpp src/*.cpp -o dir/CCircuit --std=c++17 -static-libstdc++ -O3
