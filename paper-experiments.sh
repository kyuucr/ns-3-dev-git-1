#!/bin/bash


#TCP="TcpHighSpeed TcpNewReno TcpCubic TcpBic TcpWestwood TcpHybla"
TCP="TcpCubic"
python2 ./waf

# SCENARIO 0: QUEUE-RTT
# [app] [ue_number] [tcp] [routing] [scenario] [ssth] [prefix] [cmd]
#parallel --halt now,fail=1 -j7 ./execute.sh \""[remote|UE1|4.0s|70s|ftp|ns3::TcpSocketFactory]"\" \
#  {1} {2} {3} 1 ss queue ::: ${TCP} ::: 1 ::: bp-2-340 bp-3-680 olsr


# $1 - cbr rate of the traffic
function get_background_traffic
{
  cbr_rate=$1
  echo -en "[ENB5|ENB1|4.0s|500s|cbr${cbr_rate}|ns3::UdpSocketFactory]#"
  echo -en "[ENB15|ENB1|4.0s|500s|cbr${cbr_rate}|ns3::UdpSocketFactory]#"
  echo -en "[ENB25|ENB1|4.0s|500s|cbr${cbr_rate}|ns3::UdpSocketFactory]\n"
}

bp_queue=340

# [app] [ue_number] [tcp] [routing] [scenario] [ssth] [prefix] [cmd]
SHELL=$(type -p bash) parallel -v --dry-run --halt now,fail=1 -j8 ./execute.sh \
  "\$(./getappfor.sh {1} download \"$(get_background_traffic {4}Mbps)\") {1} {2} {3} 1 ss scal-download-{4}Mbps" ::: \
  25 ::: ${TCP} ::: bp-2-${bp_queue} bp-3-${bp_queue} olsr ::: \
  260 280 300

