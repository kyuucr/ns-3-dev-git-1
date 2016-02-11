#!/bin/bash

rownodes=10

for i in $(seq 1 ${rownodes}); do
  for j in $(seq 1 ${rownodes}); do
    echo "$j $i"
  done
done
