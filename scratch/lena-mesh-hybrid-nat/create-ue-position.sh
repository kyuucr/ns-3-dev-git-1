#!/bin/bash

n=$1  # n UE in the mesh
l=$2  # lxl mesh

gimme_op ()
{
  rand=$(echo $RANDOM % 2 | bc)
  op=""

  if [[ $rand == 0 ]]; then
    op="+"
  elif [[ $rand == 1 ]]; then
    op="-"
  else
    echo -en "FAILED HARD\n";
  fi

  echo $op
}

i=1
j=1
for k in $(seq 1 ${n}); do
  xmod=$(echo "n=($RANDOM % 500); scale=3; n/1000" | bc)
  ymod=$(echo "n=($RANDOM % 500); scale=3; n/1000" | bc)
  xop=$(gimme_op)
  yop=$(gimme_op)

  x=$(echo "scale=3; $i $xop $xmod" | bc)
  y=$(echo "scale=3; $j $yop $ymod" | bc)

  echo -en "$x $y\n"

  i=$(echo $i + 1 | bc)

  ch=""
  if [[ ${i} -gt ${l} ]]; then
    i=1
    j=$(echo $j + 1 | bc)
    ch="true"
  fi

  if [[ ${j} -gt ${l} ]]; then
    j=1
    if [[ ${ch} = "" ]]; then
      i=$(echo $i + 1 | bc)
    fi
  fi

done
