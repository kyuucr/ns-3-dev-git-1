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
#ifndef GOD_LOCATION_TABLE_H
#define GOD_LOCATION_TABLE_H

#include "abstract-location-table.h"

namespace ns3 {

/**
 * \brief Location Table managed by God, he knows every node's position
 *
 * The implementation is specialized by searching Ipv4 or Ipv6 address in
 * GodIpv4LocationTable or GodIpv6LocationTable.
 *
 * Here, all methods have an empty implementation; there's no need to add, update
 * or delete positions.
 *
 * Please remember there is no need to instantiate this as an object: no
 * attributes, no Config paths, and even the object can be safely (and fastly)
 * saved on the stack, instead of the heap (no dynamic memory allocation required)
 */
class GodLocationTable : public AbstractLocationTable
{
public:
  GodLocationTable ();
  ~GodLocationTable();

  bool HasPosition (const Address &addr) const;
  bool IsInSearch (const Address &addr) const;

  Vector GetInvalidPosition () const;
  Time GetEntryUpdateTime (const Address &addr) const;

  void AddEntry (const Address &addr, const Vector &position);
  void DeleteEntry (const Address &addr);
  void Purge ();
  void Clear ();
};

/**
 * \brief The God Ipv4 location table
 *
 * This class checks for Ipv4 addresses on nodes.
 */
class GodIpv4LocationTable : public GodLocationTable
{
public:
  GodIpv4LocationTable ();

  Vector GetPosition (const Address &addr) const;
};

/**
 * \brief The God Ipv6 location table
 *
 * This class checks for Ipv6 addresses on nodes.
 */
class GodIpv6LocationTable : public GodLocationTable
{
public:
  GodIpv6LocationTable ();

  Vector GetPosition (const Address &addr) const;
};

} // namespace ns3
#endif /* GOD_LOCATION_TABLE_H */

