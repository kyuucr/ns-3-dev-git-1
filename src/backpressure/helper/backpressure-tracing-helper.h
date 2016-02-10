/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: José Núñez <jnunez@cttc.es>
 */

#ifndef BACKPRESSURE_TRACING_HELPER_H
#define BACKPRESSURE_TRACING_HELPER_H

#include<string>
#include<map>
#include <vector>

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/attribute.h"
#include "ns3/uinteger.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/nstime.h"
#include "ns3/wifi-phy.h"
#include "ns3/double.h"
#include "ns3/mac48-address.h"

namespace ns3 {


class NetDevice;

/**
 * @brief create BackpressureWifiTraceSink instances and connect them to wifi devices
 *
 * 
 */
class BackpressureTracingHelper
{
public:
  BackpressureTracingHelper ();
  void EnableBackpressure (std::string filename,  uint32_t nodeid, uint32_t deviceid, Ipv4Address srcIp, Ipv4Address dstIp);
  void EnableBackpressure (std::string filename,  uint32_t nodeid, uint32_t deviceid, Ipv4Address dstIp);//all the flows
  void EnableBackpressure (std::string filename,  uint32_t nodeid, uint32_t deviceid);
  void EnableBackpressure (std::string filename, Ptr<NetDevice> nd);
  void EnableBackpressure (std::string filename, NetDeviceContainer d, Ipv4Address srcIp, Ipv4Address dstIp);
  void EnableBackpressure (std::string filename, NetDeviceContainer d, Ipv4Address dstIp);//all the flows
  void EnableBackpressure (std::string filename, NetDeviceContainer d);
  void EnableBackpressure (std::string filename, NodeContainer n);
  
private:
  Time m_interval;    
};
class PacketStats
{
public:
 PacketStats ();
  void SetTime(Time time);
  void SetPacketId(uint64_t id);
  void SetSource(const Ipv4Address addr);
  const Ipv4Address GetSource();
  uint64_t GetTime(); 
  uint64_t GetPacketId();

private:
  uint64_t m_arrivalTime;
  uint64_t m_idPacket;
  Ipv4Address m_addr;
};
/**
 * @brief trace sink for wifi device that mimics madwifi's athstats tool.
 *
 * The BackpressureWifiTraceSink class is a trace sink to be connected to several of the traces
 * available within a wifi device. The purpose of BackpressureWifiTraceSink is to
 * mimic the behavior of the athstats tool distributed wih the madwifi
 * driver. In particular, the reproduced behavior is that obtained
 * when executing athstats without parameters: a report written in
 * text format is produced every fixed interval, based on the events
 * observed by the wifi device. 
 * 
 * Differences with the "real" athstats:
 *
 * - BackpressureWifiTraceSink is expected to write its output to a file
 *   (not to stdout).
 *
 * - only a subset of the metrics supported by athstats is supported
 *   by BackpressureWifiTraceSink
 * 
 * - BackpressureWifiTraceSink does never produce a cumulative report.
 */
class BackpressureWifiTraceSink  : public Object
{
public: 
  static TypeId GetTypeId (void);
  BackpressureWifiTraceSink ();
  virtual ~BackpressureWifiTraceSink ();
    
  /** 
   * function to be called when the net device receives a packet
   * 
   * @param context 
   * @param p the packet being received
   */
  //void IncQueue (std::string context, uint32_t length);
  void IncQueue (std::string context, uint32_t length, std::vector<uint32_t> IfacesSizes, uint32_t n_interfaces);
  void AvgQueue (std::string context, uint32_t length);
  void printV (std::string context, uint32_t v);
  void IncQueueOverflow (std::string context, uint32_t inc);
  void IncExpiredTTL(std::string context, uint32_t exp);
  //void IncTx(std::string context, const Ipv4Header &h, Ptr<const Packet> p);
  void IncTx(std::string context, const Ipv4Header &h, Ptr<const Packet> p, uint32_t deviceid);
  void SetSource(Ipv4Address src);
  const Ipv4Address GetSource();
  void SetDst(Ipv4Address dst);
  void SetNode(uint32_t nodeId);
  bool GetDoPktStats();
  void SetDeviceId (uint32_t deviceId);

  /** 
   * Open a file for output
   * 
   * @param name the name of the file to be opened.
   */
  void OpenStats (std::string const& name);
  void OpenPacketStats (std::string const& name);
  //void SetNodeId(uint32_t nodeId){m_NodeId=nodeId;}

private:

  /** 
   * @internal
   */
  void WriteStats ();

  /** 
   * @internal
   */
  void ResetCounters ();

  uint32_t m_qLength;
//to set the values of the length of the ifaces
  std::vector<uint32_t> m_qLength_ifaces;
  uint32_t m_ifaces;
  
  uint32_t m_v;
  uint32_t m_qOflow;
  uint32_t m_expTTL;
  //uint32_t m_TxData;
  std::vector<uint32_t> m_TxData;
  uint32_t m_deviceId;
  
  uint32_t m_queueAvg;
  Ipv4Address m_src;
  Ipv4Address m_dst;
  uint32_t m_idNode;
  bool m_do_pktStats;
  std::vector<PacketStats> m_packetStats;

  std::ofstream *m_Awriter;
  std::ofstream *m_Anotherwriter;
  uint32_t m_size;
  uint32_t m_seconds_file; //to print the seconds in the file

  Time m_interval;

}; // class BackpressureWifiTraceSink

//------------------------------------------------------

} // namespace ns3




#endif /* BACKPRESSURE_TRACING_HELPER_H */
