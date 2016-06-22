/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#ifndef GPSR_Neighbour_TABLE_H
#define GPSR_Neighbour_TABLE_H

#include <map>
#include "ns3/abstract-location-table.h"

namespace ns3 {

/*
 * \ingroup gpsr
 * \brief Definition of position table used by GPSR
 *
 * Implement in GpsrIpv4NeighbourTable
 */
class AbstractGpsrNeighbourTable : public AbstractLocationTable
{
public:
  AbstractGpsrNeighbourTable ();

  void SetLifeTime (const Time &lifeTime);

  // From AbstractLocationTable
  Vector GetInvalidPosition () const;

  /**
   * \brief Checks if a node is a neighbour
   * \param addr Address of the node to check
   * \return True if the node is neighbour, false otherwise
   */
  virtual bool IsNeighbour (const Address &addr) = 0;

  /**
   * \brief Gets next hop according to GPSR protocol
   * \param destPos the position of the destination node
   * \param nodePos the position of the node that has the packet
   * \return Ipv4Address of the next hop, Ipv4Address::GetZero () if no neighbour was found in greedy mode
   */
  virtual Address BestNeighbour (const Vector &destPos, const Vector &nodePos) = 0;

  /**
   * \brief Gets next hop according to GPSR recovery-mode protocol (right hand rule)
   * \param previousHop the position of the node that sent the packet to this node
   * \param nodePos the position of the destination node
   * \return Ipv4Address of the next hop, Ipv4Address::GetZero () if no nighbour was found in greedy mode
   */
  virtual Address BestAngle (const Vector &previousHop, const Vector &nodePos) = 0;

  /**
   * \brief Gives angle between the vector CentrePos-Refpos to the vector CentrePos-node counterclockwise
   * \param centrPos
   * \param refPos
   * \param node
   * \return
   */
  static double GetAngle (const Vector &centrePos, const Vector &refPos, const Vector &node);

protected:
  Time m_lifeTime;
};

/**
 * \ingroup gpsr
 * \brief The GpsrIpv4NeighbourTable class
 *
 * Define and implement a neighbour table based on Ipv4 address. Please use this
 * class as a base for defining an Ipv6 one. If, while developing the Ipv6 one,
 * the developers find out that the class can be based on the Address interface
 * without specializing anything, these could be merged in one class
 * (e.g. GpsrNeighbourTable)
 */
class GpsrIpv4NeighbourTable : public AbstractGpsrNeighbourTable
{
public:
  GpsrIpv4NeighbourTable ();

  // From AbstractGpsrNeighbourTable
  bool IsNeighbour (const Address &addr);
  Address BestNeighbour (const Vector &destPos, const Vector &nodePos);
  Address BestAngle (const Vector &previousHop, const Vector &nodePos);

  // From AbstractLocationTable
  Vector GetPosition (const Address &addr) const;
  bool HasPosition (const Address &addr) const;
  bool IsInSearch (const Address &addr) const;

  Time GetEntryUpdateTime (const Address &addr) const;

  void AddEntry (const Address &addr, const Vector &position);
  void DeleteEntry (const Address &addr);
  void Purge ();
  void Clear ();

protected:
  typedef std::map<Address, std::pair<Vector, Time> >  AddressPositionMap;

  static Address GetAddressFromIterator (const AddressPositionMap::const_iterator &it)
  {
    return it->first;
  }
  static Vector GetPositionFromIterator (const AddressPositionMap::const_iterator &it)
  {
    return it->second.first;
  }
  static Time GetTimeFromIterator (const AddressPositionMap::const_iterator &it)
  {
    return it->second.second;
  }

  AddressPositionMap m_table;
};

} // namespace ns3

#endif /* GPSR_Neighbour_TABLE_H */
