#!/bin/bash -x

rm -fr *.o prog

g++ -W -Wall --std=c++11 -c Simulation.cpp
g++ -W -Wall --std=c++11 -c Map.cpp
g++ -W -Wall --std=c++11 -c Unit.cpp
g++ -W -Wall --std=c++11 -c Path.cpp
g++ -W -Wall --std=c++11 -c Agent.cpp
g++ -W -Wall --std=c++11 -c Resource.cpp
g++ -W -Wall --std=c++11 -c Resources.cpp
g++ -W -Wall --std=c++11 -c Rule.cpp
g++ -W -Wall --std=c++11 -c main.cpp
g++ -W -Wall --std=c++11 Simulation.o Map.co Unit.o Path.o Agent.o Resource.o Resources.o Rule.o main.o -o prog
