#!/bin/bash

queue=340
TCP="TcpHybla TcpCubic TcpVegas"
ROUTING="bp-${queue}-2 bp-${queue}-3 olsr-${queue}"
FLOW="1 2 4 8 16"

python2 ./waf

# $1 - cbr rate of the traffic
function get_background_traffic
{
  cbr_rate=$1
  echo -en "[ENB5|ENB1|1.0s|1000s|cbr${cbr_rate}|ns3::UdpSocketFactory]#"
  echo -en "[ENB15|ENB11|1.0s|1000s|cbr${cbr_rate}|ns3::UdpSocketFactory]#"
  echo -en "[ENB25|ENB21|1.0s|1000s|cbr${cbr_rate}|ns3::UdpSocketFactory]\n"
}
# [app] [ue_number] [tcp] [routing] [scenario] [ssth] [prefix] [bytes] [cmd]
SHELL=$(type -p bash) parallel -v --dry-run --halt now,fail=1 -j8 ./execute-wiley.sh \
  "\$(./getappfor.sh {1} download \"$(get_background_traffic {4}Mbps)\") {1} {2} {3} 1 ss wiley-{4}Mbps {5}" ::: \
  ${FLOW} ::: \
  ${TCP} ::: \
  ${ROUTING} ::: \
  1 100 200 300 400 ::: \
  1M 5M 10M
