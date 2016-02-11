#!/bin/bash

nodes=100
sqrt=$(echo "sqrt(${nodes})" | bc)
for i in $(seq 1 ${nodes}); do
  rightborder=$(echo "${i}%${sqrt}" | bc)
  if [ ${rightborder} -eq 0 ]; then
    echo -en "1 "
  else
    echo -en "0 "
  fi
done
echo -en "\n"
