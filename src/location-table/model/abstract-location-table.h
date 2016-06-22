/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
#ifndef ABSTRACT_LOCATION_TABLE_H
#define ABSTRACT_LOCATION_TABLE_H

#include "ns3/address.h"
#include "ns3/nstime.h"
#include "ns3/vector.h"

namespace ns3 {

/**
 * \brief An interface for a table storing address and node position
 *
 * This class is not meant to be an ns-3 Object, since it is owned by
 * a node and follows the lifetime of the node itself. It is born to be easy
 * and extendible; the classical usage mode is to add node position through
 * the call to AddEntry, and retrieving the position for an address through
 * GetPosition.
 *
 * The table supports information timing: for instance, it records the time
 * at which one entry is added or updated.
 */
class AbstractLocationTable
{
public:

  /**
   * \brief Deconstructor
   */
  virtual ~AbstractLocationTable () { }

  /**
   * \brief Get a position for an address
   * \param addr Address
   * \return the position of addr, or an invalid position if addr is not stored
   */
  virtual Vector GetPosition (const Address &addr) const = 0;

  /**
   * \brief Check if addr has a position
   * \param addr Address
   * \return true if addr has a position stored, false otherwise
   */
  virtual bool HasPosition (const Address &addr) const = 0;

  /**
   * \brief IsInSearch -- TODO, what is the purpose?
   * \param addr
   * \return
   */
  virtual bool IsInSearch (const Address &addr) const = 0;

  /**
   * \brief Define and return what is an invalid position
   * \return an invalid position
   */
  virtual Vector GetInvalidPosition () const = 0;

  /**
   * \brief Gets the last time the entry was updated
   * \param id Ipv4Address to get time of update from
   * \return Time of last update to the position
   */
  virtual Time GetEntryUpdateTime (const Address &id) const = 0;

  /**
   * \brief Add an entry in the table and create/update its timestamp
   * \param addr Address
   * \param position Position
   */
  virtual void AddEntry (const Address &addr, const Vector &position) = 0;

  /**
   * \brief Delete the entry, if it exists
   * \param addr Address to delete
   */
  virtual void DeleteEntry (const Address &addr) = 0;

  /**
   * \brief Remove entries with expired lifetime
   */
  virtual void Purge () = 0;

  /**
   * \brief Clear all entries
   */
  virtual void Clear () = 0;

};

} // namespace ns3
#endif // ABSTRACT_LOCATION_TABLE_H

