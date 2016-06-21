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
#ifndef GodLocationService_H
#define GodLocationService_H

#include "location-service.h"

namespace ns3
{
/**
 * \ingroup godLS
 * 
 * \brief God Location Service
 */
class GodLocationService : public LocationService
{
public:

  /// c-tor
  GodLocationService (Time tableLifeTime);
  GodLocationService ();
  virtual ~GodLocationService();
  virtual void DoDispose ();
  Vector GetPosition (Ipv4Address adr);
  bool HasPosition (Ipv4Address adr);
  bool IsInSearch (Ipv4Address adr);

  void SetIpv4 (Ptr<Ipv4> ipv4);
  Vector GetInvalidPosition ();
  Time GetEntryUpdateTime (Ipv4Address id);
  void AddEntry (Ipv4Address id, Vector position);
  void DeleteEntry (Ipv4Address id);

  void Purge ();
  virtual void Clear ();

private:
  /// Start protocol operation
  void Start ();
};
}
#endif /* GodLocationService_H */

