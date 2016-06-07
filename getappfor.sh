#!/bin/bash

# Choose the flow number
flows=$1

# upload download
if [ "$2" = "" ]; then
  style="download"
else
  if [[ "$2" = "download" || "$2" = "upload" ]]; then
    style=$2
  else
    echo "Mode $2 not recognized. Valid values are upload or download"
    exit 1
  fi
fi

# Background traffic. Could be empty.
background=$3

local_app=""
for i in $(seq 1 ${flows}); do
  m=$(echo "((${i}-1) / 25) + 1" | bc)
  s=$(echo "(${m}*4) + ((${i}-(25*(${m}-1))) * 0.05)" | bc)

  first=""
  if [ "${style}" = "download" ]; then
    first="remote|UE${i}"
  else
    first="UE${i}|remote"
  fi

  if [ "${local_app}" = "" ]; then
    local_app="[${first}|${s}|100s|ftp|ns3::TcpSocketFactory]"
  else
    local_app="[${first}|${s}|100s|ftp|ns3::TcpSocketFactory]#${local_app}"
  fi
done

if [[ ! "${background}" = "" ]]; then
  echo "${local_app}#${background}"
else
  echo "${local_app}"
fi
