/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Authors: Jose Nuñez <jose.nunez@cttc.cat
 */

///
/// \file	backpressure-repositories.h
/// \brief	Here are defined all data structures needed by a BACKPRESSURE node.
///

#ifndef __BACKPRESSURE_REPOSITORIES_H__
#define __BACKPRESSURE_REPOSITORIES_H__

#include <set>
#include <vector>

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/log.h"//delete when not needed routing-data-queue.cc NS-3
#include "ns3/arp-cache.h"

namespace ns3 { namespace backpressure {



/// An Interface Association Tuple.
struct IfaceAssocTuple
{
  /// Interface address of a node.
  Ipv4Address ifaceAddr;
  /// Main address of the node.
  Ipv4Address mainAddr;
  /// Time at which this tuple expires and must be removed.
  Time time;
};

static inline bool
operator == (const IfaceAssocTuple &a, const IfaceAssocTuple &b)
{
  return (a.ifaceAddr == b.ifaceAddr
          && a.mainAddr == b.mainAddr);
}

static inline std::ostream&
operator << (std::ostream &os, const IfaceAssocTuple &tuple)
{
  os << "IfaceAssocTuple(ifaceAddr=" << tuple.ifaceAddr
     << ", mainAddr=" << tuple.mainAddr
     << ", time=" << tuple.time << ")";
  return os;
}

struct FlowTuple
{
  // source address
  Ipv4Address src;
  // destination address
  Ipv4Address dst;
  // timestamp indicating last time a nh and iface were updated
  Time lstRouteDec;
  // timestamp indicating last time the flow entry was used
  Time lstEntryUsed; 
  // dst port
  uint16_t dstPort;
  // src port
  uint16_t srcPort;
  // next-hop taken in last routing decision
  Ipv4Address nh; 
  // output interface
  uint32_t iface;
  // ttl when inserted the rule
  uint8_t ttl;
};

struct PacketTuple
{ //struct to store packet characteristics when reenqueued
  // source address
  Ipv4Address src;
  // destination address
  Ipv4Address dst;
  // dst port
  uint16_t dstPort;
  // src port
  uint16_t srcPort; 
};


/// A Neighbor Tuple.
struct NeighborTuple
{
  /// Local address receiver of the HELLO msg
  Ipv4Address localIfaceAddr;
  /// interface from when the HELLO is received
  uint32_t interface;
  /// Main address of a neighbor node.
  Ipv4Address theMainAddr;
  /// Sender Iface of a neighbor node.
  Ipv4Address neighborMainAddr;
  /// Hw Address
  Mac48Address neighborhwAddr;
  /// Neighbor Type and Link Type at the four less significative digits.
  enum Status {
    STATUS_NOT_SYM = 0, // "not symmetric"
    STATUS_SYM = 1, // "symmetric"
  } status;
  /// A value between 0 and 7 specifying the node's willingness to carry traffic on behalf of other nodes. Unusued
  uint8_t willingness;
  /// queue Length announced by the node at current timeslot
  uint32_t qLength;
  /// queue Length announced by the node at previous timeslot
  uint32_t qLengthPrev;
  /// queue Length scheduled to be prev
  uint32_t qLengthSchedPrev;
  /// time last Hello was received
  Time lastHello; 
  Time qPrev;
  //time the qPrev was updated
  double x;
  double y;
  
  // Neighbor Node Identifier
  uint32_t Id;
  ///Variable to extend the grid-multiradio element
  uint8_t n_interfaces;
  std::vector<uint32_t> qLength_eth; //vector to store neighbor_eth_length
  //uint8_t sending_interface;
  
};

/////////in order to use it in a map, we have to define the == and the < operator
static inline bool
operator == (const PacketTuple &a, const PacketTuple &b)
{
  return (a.src == b.src      //src address
          && a.dst == b.dst   //dst address
          && a.srcPort == b.srcPort //srcPort
          && a.dstPort == b.dstPort); //dstPort
}

static inline bool
operator < (const PacketTuple &a, const PacketTuple &b)
{
  //we order by srcPort or dst Port
  return a.srcPort < b.srcPort || (a.srcPort == b.srcPort && a.dstPort < b.dstPort);
}
//////////////
static inline bool
operator == (const FlowTuple &a, const FlowTuple &b)
{
  return (a.src == b.src      //src address
          && a.dst == b.dst   //dst address
          && a.srcPort == b.srcPort //srcPort
          && a.dstPort == b.dstPort); //dstPort
}

static inline bool
operator == (const NeighborTuple &a, const NeighborTuple &b)
{
  return (a.neighborMainAddr == b.neighborMainAddr
          && a.status == b.status
          && a.willingness == b.willingness);
}

static inline std::ostream&
operator << (std::ostream &os, const NeighborTuple &tuple)
{
  os << "NeighborTuple(neighborMainAddr=" << tuple.neighborMainAddr
     << ", status=" << (tuple.status == NeighborTuple::STATUS_SYM? "SYM" : "NOT_SYM")
     << ", willingness=" << (int) tuple.willingness << ")";
  return os;
}

typedef std::vector<FlowTuple>			FlowSet;       ///< Flow Set type.
typedef std::vector<NeighborTuple>		NeighborSet;	///< Neighbor Set type.
typedef std::vector<NeighborTuple*>		NeighborSameLengthSet; ///< Neighbor of Same Length type
typedef std::vector<IfaceAssocTuple>		IfaceAssocSet; ///< Interface Association Set type.
typedef std::vector<Ptr<ArpCache> > 		ArpSet;


}}; // namespace ns3, backpressure

#endif  /* __PROB_OLSR_REPOSITORIES_H__ */
