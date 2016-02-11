#!/bin/bash

# uplink downlink
if [ "$2" = "" ]; then
  style="download"
else
  style=$2
fi

app=()
for n in 1 2 4 6 8 10 12 14 16 18 20 25 50 75 100 125 150 175 200; do
  local_app=""
  for i in $(seq 1 ${n}); do
    s=$(echo "4 + ($i * 0.05)" | bc)
    first=""
    if [ "${style}" = "download" ]; then
      first="remote|UE${i}"
    else
      first="UE${i}|remote"
    fi

    if [ "${local_app}" = "" ]; then
      local_app="[${first}|${s}|70s|ftp|ns3::TcpSocketFactory]"
    else
      local_app="[${first}|${s}|70s|ftp|ns3::TcpSocketFactory]#${local_app}"
    fi
  done
  local_app="${local_app}"
  app[$n]=${local_app}
done

echo ${app[$1]}
