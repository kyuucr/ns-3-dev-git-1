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


/// A Neighbor Tuple.
class NeighborTuple
{
  /// Local address receiver of the HELLO msg
  Ipv4Address localIfaceAddr;
  /// interface from when the HELLO is received
  uint32_t interface;
  /// Main address of a neighbor node.
  Ipv4Address theMainAddr;
  /// Sender Iface of a neighbor node.
  Ipv4Address neighborMainAddr;
  //hw Address
  Mac48Address neighborhwAddr;
  /// Neighbor Type and Link Type at the four less significative digits.
  enum Status {
    STATUS_NOT_SYM = 0, // "not symmetric"
    STATUS_SYM = 1, // "symmetric"
  } status;
  /// A value between 0 and 7 specifying the node's willingness to carry traffic on behalf of other nodes.
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
};

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


typedef std::vector<NeighborTuple>		NeighborSet;	///< Neighbor Set type.
typedef std::vector<NeighborTuple*>		NeighborSameLengthSet; ///< Neighbor of Same Length type
typedef std::vector<IfaceAssocTuple>		IfaceAssocSet; ///< Interface Association Set type.


}}; // namespace ns3, backpressure

#endif  /* __PROB_OLSR_REPOSITORIES_H__ */
