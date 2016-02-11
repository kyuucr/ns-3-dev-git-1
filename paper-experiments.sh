#!/bin/bash


#TCP="TcpHighSpeed TcpNewReno TcpCubic TcpBic TcpWestwood TcpHybla"
TCP="TcpCubic"
python2 ./waf

# SCENARIO 0: QUEUE-RTT
# [app] [ue_number] [tcp] [routing] [scenario] [ssth] [prefix] [cmd]
#parallel --halt now,fail=1 -j7 ./execute.sh \""[remote|UE1|4.0s|70s|ftp|ns3::TcpSocketFactory]"\" \
#  {1} {2} {3} 1 ss queue ::: ${TCP} ::: 1 ::: bp-2-340 bp-3-680 olsr

# [app] [ue_number] [tcp] [routing] [scenario] [ssth] [prefix] [cmd]
SHELL=$(type -p bash) parallel -v --halt now,fail=1 -j8 ./execute.sh \
  "\$(./getappfor.sh {1} downlink) {1} {2} {3} 1 ss scalability" ::: \
  1 2 4 6 8 10 12 14 16 ::: ${TCP} ::: bp-2-340 bp-3-340 olsr

