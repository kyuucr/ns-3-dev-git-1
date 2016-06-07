#!/bin/bash
app=$1
ue=$2
tcp=$3
routing=$4
scenario=$5
ssth=$6
prefix=$7
bytes=$8
cmd=$9

usage()
{
  echo -en "$0 is made to execute simulations.\nUsage:\n"
  echo -en "\n${0} [app] [ue_number] [tcp] [routing] [scenario] [ssth] [prefix] [bytes] [cmd]\n"
  echo -en "\t[app]:\tApplication parameters. E.g: [remote|UE1|4.0s|40s|ftp|ns3::TcpSocketFactory]\n"
  echo -en "\t[ue_number]:\tNumber of UE. E.g: 1\n"
  echo -en "\t[tcp]:\tns-3 tcp. E.g: TcpHighSpeed\n"
  echo -en "\t[routing]:\tRouting protocol. For BP select strategy and q size (bp-2-200) 2=PER_PACKET. E.g: bp, olsr\n"
  echo -en "\t[scenario]:\tScenario. E.g: 1, 2, 3\n"
  echo -en "\t[ssth]:\tEmploy or not the slow start. E.g.: ss, ca\n"
  echo -en "\t[prefix]:\tOutput prefix\n"
  echo -en "\t[bytes]:\tBytes to transmit. E.g.: 0.1M\n"
  echo -en "\t[cmd]:\tCommand to prefix execution. E.g: \"gdb --args\"\n"
}

unit=$(echo ${bytes} | awk '{print substr($0,length,1)}')
value=$(echo ${bytes} | head -c -2)

if [[ "${unit}" = "K" ]]; then
  mult="1000"
elif [[ "${unit}" = "M" ]]; then
  mult="1000000"
else
  echo "${unit} is not valid. Valid are K and M"
  exit 1
fi

output="${prefix}-${scenario}-${tcp}-${ue}-${routing}-${bytes}-${ssth}"
bytes=$(echo "${value} * ${mult}" | bc)

if [[ ${ssth} = "ss" ]]; then
  ssth="--initSsTh=4000000000U"
elif [[ ${ssth} = "ca" ]]; then
  ssth="--initSsTh=500U"
else
  echo "${ssth} not valid as ssth option. Params: $@"
  usage
  exit 1
fi

mkdir -p $output

rout=$(echo ${routing} | cut -d "-" -f1)
bpQ=$(echo ${routing} | cut -d "-" -f2)
bpV=$(echo ${routing} | cut -d "-" -f3)

routString="--routing=${rout}"
if [[ ${rout} == "bp" ]]; then
  # ALGV=2 stands for BP-MR (SANSA)
  routString="${routString} --localstrategy=${bpV} --algV=2 --theV=${bpQ} --bpStat=false"
fi

if [[ "${bpQ}" == "" ]]; then
  echo "Missing queue for mesh network! "
  exit 1
fi

args="--simTime=1000.0s ${ssth} --replication=0 --meshQ=${bpQ} \
      --num-ues=${ue} --distance=60.0 ${routString} \
      --tcp=ns3::${tcp} --out=${output} --app=${app} \
      --ftpBytes=${bytes} --scenario=${scenario}"

${cmd} ./build/scratch/lena-mesh-hybrid-nat/lena-mesh-hybrid-nat ${args}

