#!/bin/bash

# ./execute-lena-mesh-nat.sh mode tcp_cong [gdb]
#
# mode: single_ftp or single_cbr
# tcp_cong: TcpNewReno, TcpHybla, TcpCubic, TcpBic ...

cmd="lena-mesh-hybrid-nat"
endtime="300.0"

if [[ $1 == "single_ftp" ]]; then
  app="[remote|UE1|4.0s|${endtime}s|ftp|ns3::TcpSocketFactory] --ftpBytes=50000000"
elif [[ $1 == "single_cbr" ]]; then
  app="[remote|UE1|4.0s|${endtime}s|cbr|ns3::TcpSocketFactory]"
fi

output="out-${1}-${2}"
mkdir -p ${output}

args="--simTime=${endtime} --replication=0 --num-rows=3 --num-cols=3 \
   --num-ues=1 --distance=60.0 --interPacketInterval=2 --localstrategy=3 --algV=2 \
   --theV=200 --bpStat=true --tcp=ns3::${2} --out=${output} --app=${app}"

if [[ $3 == "valgrind" ]]; then
  python2 ./waf --run "${cmd}" --command-template="valgrind --leak-check=full --track-origins=yes -v %s $args" 2>err.txt 1>out.txt
elif [[ $3 == "gdb" ]]; then
  python2 ./waf --run "${cmd}" --command-template="gdb --args %s $args"
else
  python2 ./waf --run "${cmd} ${args}"
fi

