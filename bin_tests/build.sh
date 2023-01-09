#!/bin/sh
g++ -std=c++17 -ggdb3 -I../src send.cpp -o send
g++ -std=c++17 -ggdb3 -I../src recv.cpp -o recv