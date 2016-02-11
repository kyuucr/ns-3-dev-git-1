#!/bin/bash

nodes=25
output="mesh.txt"

# Empty the output file (hope you have git installed)
echo -en "" > ${output}

for n in $(seq 1 ${nodes}); do
  echo -en "Doing node ${n}..\n"

  sqrt=$(echo "sqrt(${nodes})" | bc)

  # indicates the node below ${n}
  down=$(echo "${n}+${sqrt}" | bc)
  # indicates the node ideally at right of ${n}
  right=$(echo ${n}+1 | bc)

  # indicate the id of the starting node of the down
  # border (e.g. in 5x5 is 21)
  downborder=$(echo "${nodes}-${sqrt}+1" | bc)

  # indicate (rightborder==0) if ${n} is part of the
  # right border
  rightborder=$(echo "${n}%${sqrt}" | bc)

  for i in $(seq 1 ${nodes}); do
    echo -en "Connectivity between ${n} and ${i}\n"

    # The nodes does not have connection with itself
    if [[ $i -eq $n ]]; then
      echo -en "0 " >> ${output}
      continue
    fi

    # The node $n connects to the $n+1, but not if it is in the
    # right border (e.g. 5x5, node 5 does not connect to 6)
    if [ ${i} -eq ${right} ] && ! [ ${rightborder} -eq 0 ] ; then
      echo -en "Connecting ${n} with ${i} (right)\n"
      echo -en "1 " >> ${output}
      continue
    fi

    if [ ${i} -eq ${down} ] && ! [ ${n} -ge ${downborder} ]; then
      echo -en "Connecting ${n} with ${i} (down)\n"
      echo -en "1 " >> ${output}
      continue
    fi

    # ${n} and ${i} are not connected
    echo -en "0 " >> ${output}

  done
  echo -en "\n" >> ${output}
done
