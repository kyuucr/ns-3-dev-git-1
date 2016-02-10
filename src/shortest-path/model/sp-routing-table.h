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

#ifndef SHORTEST_PATH_ROUTING_TABLE_H
#define SHORTEST_PATH_ROUTING_TABLE_H

#include "ns3/node.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/net-device-container.h"
#include "ns3/wifi-net-device.h"
#include <list>
#include <vector>

namespace ns3 {
  
class ShortestPathRoutingTable : public Object
{
public:

  ShortestPathRoutingTable ();
  virtual ~ShortestPathRoutingTable ();

  static TypeId GetTypeId ();
  void AddRoute (Ipv4Address srcAddr, Ipv4Address relayAddr, Ipv4Address dstAddr);
  void AddNode (Ptr<Node> node, Ipv4Address addr);
  void AddNodeHoles(Ptr<Node> node, Ipv4Address addr, bool ethUp);
  Ipv4Address LookupRoute (Ipv4Address srcAddr, Ipv4Address dstAddr);
  void UpdateRoute (double txRange);
  void UpdateRouteBegin (double txRange);
  void UpdateRouteBeginGrid (double txRange);
  void UpdateRouteSansaGrid (double txRange);
  void UpdateRouteSansaLena (double txRange);
  void UpdateRouteEnd (double txRange);
  void UpdateRouteETT (double txRange, NetDeviceContainer &wifiDevices);
  double GetDistance (Ipv4Address srcAddr, Ipv4Address dstAddr);
  uint32_t CheckRoute (Ipv4Address srcAddr, Ipv4Address dstAddr);

  void Print (Ptr<OutputStreamWrapper> stream) const;

private:
  typedef struct
    {
      Ipv4Address srcAddr;
      Ipv4Address relayAddr;
      Ipv4Address dstAddr;
    }
  SPtableEntry;
  
  typedef struct
    {
      Ptr<Node> node;
      Ipv4Address addr;
      bool eth;
    } 
  SpNodeEntry;
  
  double DistFromTable (uint16_t i, uint16_t j);
  double DistFromTableSansa (uint16_t i, uint16_t j);
  double DistFromTableSansaLena ( uint16_t i, uint16_t j);
  
  std::list<SPtableEntry> m_sptable;
  std::vector<SpNodeEntry> m_nodeTable;
  
  uint16_t* m_spNext;
  double*   m_spDist;
  
  double    m_txRange;
};

}

#endif // SHORTEST_PATH_ROUTING_TABLE_H
