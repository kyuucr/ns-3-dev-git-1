/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Arizona
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Junseok Kim <junseok@email.arizona.edu> <engr.arizona.edu/~junseok>
 */

#ifndef SHORTEST_PATH_ROUTING_H
#define SHORTEST_PATH_ROUTING_H

#include <list>
#include "ns3/ipv4-routing-protocol.h"
#include "sp-routing-table.h"
#include "ns3/traced-callback.h"
#include <vector>
#include <iostream>

//Location Table
#include "ns3/location-list.h"
//Node List
#include "ns3/node-list.h"

/***********Shortest Path Strategy ************/
#define SINGLE_PATH		0
#define MULTI_PATH 		1
/*********************************************/

using namespace std;
using namespace ns3;

namespace ns3 {
  
class ShortestPathRouting : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);

  ShortestPathRouting ();  
  virtual ~ShortestPathRouting ();
  
  // Below are from Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

  virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                           UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                           LocalDeliverCallback lcb, ErrorCallback ecb);
  
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;
  
  void SetRtable1 (Ptr<ShortestPathRoutingTable> prt);
  void SetRtable2 (Ptr<ShortestPathRoutingTable> prt);
  uint32_t FindIfaceAssociatedtoRelay (Ipv4Address relay);
  uint32_t FindIfaceAssociatedtoRelayLena (Ipv4Address relay);
  void MapMACAddressToIPinterface (Mac48Address MAC, uint32_t &interface);
  
  /// Create loopback route for given header
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header) const;
  
protected:
private:
  
  Ptr<ShortestPathRoutingTable> m_rtable1;
  Ptr<ShortestPathRoutingTable> m_rtable2;
  std::vector<Ipv4Address> m_address;
  std::vector<Ipv4Address> m_broadcast;
  std::vector<uint32_t> m_ifaceId;
  //Ipv4Address m_address;
  //Ipv4Address m_broadcast;
  //uint32_t m_ifaceId;
  uint32_t m_nodesRow;
  Ptr<Ipv4> m_ipv4;
  uint8_t m_variant;
  bool m_rrobin;
  
  TracedCallback <Ipv4Address, Ipv4Address, Ptr<const Packet>, Ipv4Header, int64_t, uint32_t > m_TxBeginData;
  //TracedCallback <Ipv4Address, Ipv4Address, Ptr<const Packet> > m_TxBeginData;
  TracedCallback <Ptr<const Packet>, Ipv4Header, int64_t, uint32_t > m_TxDataAir;
  
};

} //namespace ns3

#endif /* SHORTEST_PATH_ROUTING_H */
