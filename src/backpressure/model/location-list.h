/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Authors: 
 *  José Núñez-Martínez <jose.nunez@cttc.cat>,
 */
#ifndef LOCATION_LIST_H
#define LOCATION_LIST_H

#include <map>
#include "ns3/vector.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/sp-routing-table.h"

namespace ns3 {

/**
 * \ingroup node
 *
 * \brief the list of the coordinates of each node by identifier (i.e., ip address)
 *
 * Every Node created is automatically added to this list.
 */
class LocationList
{
public:
  
  ~LocationList ();
  
  typedef std::map<Ipv4Address, Vector>::const_iterator Iterator;
  /**
   * \param node node to add
   * \returns index of node in list.
   *
   * This operation is called in the main simulation program.
   * 
   */
  static void Add (Ipv4Address addr, Vector position);
  /**
   * \param addr ipv4 address of the node to add
   * \param id the Node id regarding the general NodeList
   *
   * This operation is called in the main simulation program.
   * 
   */
  static void AddNodeId ( Ipv4Address addr, uint32_t id);
  /**
   * \param addr ipv4 address of the node to add
   * \param gw_id the id of the node in the backhaul connected to the EPC
   *
   * This operation is called in the main simulation program.
   * 
   */
  static void AddGw (Ipv4Address addr, uint32_t gw_id);
  
  /**
   * \param addr ipv4 address of the node to add
   * \param gw_id the id of the node in the backhaul with a satellite connection
   *
   * This operation is called in the main simulation program.
   * 
   */
  static void AddSatGw (Ipv4Address addr, uint32_t gw_id);
  /**
   * \returns a C++ iterator located at the beginning of this
   *          list.
   */
  static LocationList::Iterator Begin (void);
  /**
   * \returns a C++ iterator located at the end of this
   *          list.
   */
  static LocationList::Iterator End (void);
  /**
   * \param addr ipv4 address of the node you are requesting info.
   * \returns the Location associated node with Ipv4Address addr.
   */
  static Vector GetLocation (Ipv4Address addr);
  /**
   * \param addr ipv4 address of the node you are requesting info.
   * \returns the id of the NodeList associated to the specified Ipv4Address addr 
   */
  static uint32_t GetNodeId (Ipv4Address addr);
  /**
   * \param srt ShortestPathRoutingTable to determine the number of hops.
   */  
  static void SetRoutingTable (Ptr<ShortestPathRoutingTable> srt);
  /**
   * \param srt ShortestPathRoutingTable to determine the number of hops including the
   *            the satellite connection.
   */  
  static void SetRoutingTableSat (Ptr<ShortestPathRoutingTable> srt);
  /**
   * \param src_addr origin ip address.
   * \param dst_addr destination ip address.
   * \returns the minimum number of hops between origin and destination 
   */
  static uint32_t GetHops (Ipv4Address src_addr, Ipv4Address dst_addr);
  /**
   * \param src_addr origin ip address.
   * \param dst_addr destination ip address.
   * \returns the minimum number of hops between origin and destination, but this
   *          value depends whether this flow can go through the satellite connection 
   */
  static uint32_t GetHops (Ipv4Address src_addr, Ipv4Address dst_addr, bool SatFlow);
  
  /**
   * \param n index of requested ip address.
   * \returns the node identifier of this node connected to the gw/epc 
   */
  static uint32_t GetGwId (Ipv4Address addr);
  
  /**
   * \param n index of requested ip address.
   * \returns the node identifier of this node connected to the sat gw 
   */
  static uint32_t GetSatGwId (Ipv4Address addr);
  
  static void PrintNodeIds ();
  static void PrintGws ();
  
  static std::map<Ipv4Address, Vector> m_locations;
  static std::map<Ipv4Address, uint32_t> m_ids;  // To get queue backlog inside backpressure in centralized mode
  static std::map<Ipv4Address, uint32_t> m_Gws;  // To know the node identifier of a terrestrial node connected to the epc
  static std::map<Ipv4Address, uint32_t> m_SatGws; // To know the node identifier of a terrestrial node with satellite connection
  static Ptr<ShortestPathRoutingTable> m_sprt;  //  Kind of God Routing: Tells you the minimum number of hops between a srcs and dsts through the terrestrial bh
  static Ptr<ShortestPathRoutingTable> m_sprtsat; //Contains the previous information but also the path through the satellite connection
  
  //wowmon_we need of a central element to change the behavior of the network
  static bool m_reset;
  static void SetResetLocationList(bool value);
  static bool GetResetLocationList();
  
  
private:
};

}//namespace ns3


#endif /* LOCATION_LIST_H */
