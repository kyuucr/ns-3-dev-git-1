/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 José Núñez 
 * Copyright (c) 2012 CTTC
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
 * Authors: Jose Núñez <jose.nunez@cttc.cat>
 *
 */

/// \brief	This header file declares and defines internal state of an BACKPRESSURE node.

#ifndef __BACKPRESSURE_STATE_H__
#define __BACKPRESSURE_STATE_H__

#include "backpressure-repositories.h"
#include "ns3/vector.h"
#include "ns3/ipv4.h"
#include "ns3/net-device.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/ipv4-interface.h"
#include "ns3/arp-cache.h"
#include "ns3/wifi-mac-header.h"

namespace ns3 {

using namespace backpressure;

/// This class encapsulates all data structures needed for maintaining internal state of an PROB_OLSR node.
class BackpressureState
{
  //  friend class Backpressure;
private: 
uint32_t getQlengthCentralized(uint32_t id, Ipv4Address &addr,uint32_t &virt_cong, Ipv4Address dstAddr);
float GetRate(uint32_t anode, Ipv4Address neighAddr, Ptr<Ipv4> m_ipv4, uint32_t interface);

protected:
  NeighborSet m_neighborSet;		///< Neighbor Set.
  FlowSet     m_flowSet; 		///< Flow Set.
  //Time m_neighValid; 			///< Time Neighbors are valid.
  ArpSet m_hwSet;			///<Mapping of IP address to Hw Address.
  IfaceAssocSet m_ifaceAssocSet;	///< Interface Association Set.
  //uint32_t m_shadow;

public:

  uint32_t m_maxV;
  BackpressureState ()
  {}
  
  Time m_neighValid; 			///< Time Neighbors are valid
  
  void PrintNeighInfo(uint32_t anode);
  // Neighbor
  const NeighborSet & GetNeighbors () const
  {
    return m_neighborSet;
  }
  NeighborSet & GetNeighbors ()
  {
    return m_neighborSet;
  }
  NeighborTuple* FindNeighborTuple (const Ipv4Address &mainAddr);
  bool FindNeighborMainAddr (Ipv4Address mainAddr);
  //uint32_t GetShadow();
  uint32_t GetShadow(Ipv4Address dstAddr);
  int32_t FindNeighborMaxQueueDiff(uint32_t id);
  int32_t FindNeighborMaxQueue(uint32_t id);
  int32_t FindNeighborMaxQueueInterface(uint32_t id);
  void UpdateIfaceQlengthCentralized (uint32_t id, Ipv4Address addr);
  int32_t FindNeighborMaxQueue(uint32_t id,uint32_t direction);
  int32_t FindNeighborMinQueue(uint32_t id);
  
  //Penalty Functions
  float CalculatePenaltyNeigh(uint32_t anode, Vector dPos, uint32_t strategy, uint32_t ttl, double DistFromSrc, double DistFromNeigh, double distFromCurrToSrc, double distFromNeighToSrc, uint32_t virt_cong, const Mac48Address &hwNeigh, const Mac48Address &from, Ipv4Address dstAddr);
  float CalculatePenaltyNeighGrid(uint32_t anode, Vector dPos, uint32_t ttl, double distFromCurrToDst, double distFromNeigh, const Mac48Address &hwNeigh, const Mac48Address &from, Ipv4Address dstAddr);
  float CalculatePenaltyNeighGridSANSA(uint32_t anode, double HopsFromCurr, double HopsFromNeigh, const Mac48Address &hwNeigh, const Mac48Address &from);
  float CalculatePenaltyNeighGridSANSA_v2(Ptr<Ipv4> m_ipv4, uint32_t anode, Ipv4Address dstAddr, double HopsFromCurr, double HopsFromNeigh, const Mac48Address &hwNeigh, const Mac48Address &from, bool SatFlow);

  //Find Next Hop variants
  Ipv4Address FindNextHopBasic(const Ipv4Address &srcAddr, const Ipv4Address &dstAddr);
  Ipv4Address FindNextHopBcp(const Ipv4Address &srcAddr, const Ipv4Address &dstAddr, uint32_t myLength, uint32_t node);
  Ipv4Address FindNextHopInFlowTable(Ipv4Address srcAddr, Ipv4Address dstAddr, Ipv4Address currAddr,uint16_t s_port, uint16_t d_port, uint32_t &iface); //, bool &erased, Ipv4Address &prev_nh);
  //this is version 2 of the the FindNextHopInFlowTable function
  Ipv4Address FindNextHopInFlowTable(Ptr<Ipv4> m_ipv4,Ipv4Address srcAddr, Ipv4Address dstAddr, Ipv4Address currAddr,uint16_t s_port, uint16_t d_port, uint32_t &iface, bool &erased, Ipv4Address &prev_nh, uint8_t ttl);
  void UpdateFlowTableEntry(Ipv4Address srcAddr, Ipv4Address dstAddr, Ipv4Address currAddr, uint16_t s_port, uint16_t d_port, Ipv4Address nh, uint32_t iface, uint8_t ttl);
  void EraseFlowTableEntry (Ipv4Address srcAddr, Ipv4Address dstAddr, uint16_t s_port, uint16_t d_port);
  void EraseFlowTable();
  Ipv4Address FindNextHopBackpressureCentralized (uint32_t strategy, Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t ttl, uint32_t myLength, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface, const Mac48Address &from, uint32_t, Ptr<Ipv4> );
  Ipv4Address FindNextHopBackpressureDistributed (uint32_t strategy, Ipv4Address const &curAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t ttl, uint32_t myLength, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface, const Mac48Address &from, uint32_t);
  
  //Next Hop: Greedy forwarding alternatives
  Ipv4Address FindNextHopGreedyForwardingSinglePath(Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, Vector currPos, uint32_t &iface);
  Ipv4Address FindNextHopGreedyForwardingMultiPath(Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, Vector currPos, uint32_t &iface);
  
  //GRID-> single queue per interface and nodes with multiple interfaces
  Ipv4Address FindNextHopBackpressureCentralizedGridSANSA(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t anode, uint32_t m_weightV, uint32_t &iface, Ptr<Ipv4> m_ipv4, uint32_t nodesColumn, const Mac48Address &from);
  Ipv4Address FindNextHopBackpressureCentralizedGridSANSAv2(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t anode, uint32_t m_weightV, uint32_t &iface, Ptr<Ipv4> m_ipv4, uint32_t nodesColumn, const Mac48Address &from, bool erased, Ipv4Address prev_nh);
  Ipv4Address FindNextHopBackpressureCentralizedGridSANSALena(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t m_weightV, uint32_t &iface, Ptr<Ipv4> m_ipv4, const Mac48Address &from, bool erased, Ipv4Address prev_nh, bool SatFlow);
  Ipv4Address FindNextHopBackpressureCentralizedGridSANSALenaMR(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t m_weightV, uint32_t &iface, uint32_t output_interface, Ptr<Ipv4> m_ipv4, const Mac48Address &from, bool erased, Ipv4Address prev_nh, bool SatFlow);
  //RING ENHANCED->queue_per_interface
  Ipv4Address FindNextHopBackpressurePenaltyRingEnhanced ( Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t qlen_clck, uint32_t qlen_countclck, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface, uint32_t ttl, Ptr<Ipv4> m_ipv4, bool centraliced, uint32_t output_interface);
  //GRID ENHANCED->queue_per_interface
  Ipv4Address FindNextHopBackpressurePenaltyGridEnhanced ( Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface, uint32_t ttl, Ptr<Ipv4> m_ipv4, bool centraliced, uint32_t output_interface, uint32_t nodesRow, const Mac48Address &from);
  Ipv4Address FindNextHopBackpressurePenaltyGridEnhancedHybrid ( Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t qlen_max, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface, uint32_t ttl, Ptr<Ipv4> m_ipv4, bool centraliced, uint32_t output_interface, uint32_t nodesRow, const Mac48Address &from);
  
//Function to find the Ip address of the neighbor which is listening to this interface
  Ipv4Address FindtheNeighbourAtThisInterface (uint32_t interface);

  NeighborTuple* FindNeighborTuple (const Ipv4Address &mainAddr,
                                    uint8_t willingness);
  void EraseNeighborTuple (const NeighborTuple &neighborTuple);
  void EraseNeighborTuple (const Ipv4Address &mainAddr);
  void InsertNeighborTuple (const NeighborTuple &tuple);
  void CheckTerrestrialNeighbor (NeighborTuple *neighborTuple, Time now);
  
  void InsertArpTuple(Ptr<ArpCache> a);
  Mac48Address GetHwAddress(const Ipv4Address addr);
  bool areAllCloserToSrc(Vector srcPos,Vector currPos);
  uint32_t GetQlengthCentralized(uint32_t id, Ipv4Address &addr);
  uint32_t GetVirtualQlengthCentralized(uint32_t id, Ipv4Address &addr, uint32_t direction_bis);

  // Interface association
  const IfaceAssocSet & GetIfaceAssocSet () const
  {
    return m_ifaceAssocSet;
  }
  IfaceAssocSet & GetIfaceAssocSetMutable ()
  {
    return m_ifaceAssocSet;
  }
  IfaceAssocTuple* FindIfaceAssocTuple (const Ipv4Address &ifaceAddr);
  const IfaceAssocTuple* FindIfaceAssocTuple (const Ipv4Address &ifaceAddr) const;
  void EraseIfaceAssocTuple (const IfaceAssocTuple &tuple);
  void InsertIfaceAssocTuple (const IfaceAssocTuple &tuple);

  // Returns a vector of all interfaces of a given neighbor, with the
  // exception of the "main" one.
  std::vector<Ipv4Address>
  FindNeighborInterfaces (const Ipv4Address &neighborMainAddr) const;
  
  std::map<Ipv4Address, uint32_t> m_shadow;
  //update Neighbor Time expiration on State
  void SetNeighExpire(const Time time);
  
  void SetSatNodeValue(bool value);
  bool GetSatNodeValue ();
  void SetTerrGwNodeValue(bool value);
  bool GetTerrGwNodeValue ();
  bool IsValidNeighborSANSA(Ptr<Ipv4> m_ipv4, Time lastHello, uint32_t interface, bool SatFlow, Ipv4Address dstAddr, Ipv4Address NeighAddr);
  bool m_SatNode;
  bool m_TerrGwNode;
  
  
};

} // namespace ns3

#endif
