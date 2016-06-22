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

#include "god-location-table.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GodLocationTable");

GodLocationTable::GodLocationTable ()
  : AbstractLocationTable ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

GodLocationTable::~GodLocationTable ()
{
}

bool
GodLocationTable::HasPosition (const Address &addr) const
{
  (void) addr;
  NS_LOG_DEBUG ("GodLocationTable has always a position");
  return true;
}

bool
GodLocationTable::IsInSearch (const Address &addr) const
{
  (void) addr;
  return false;
}

Vector
GodLocationTable::GetInvalidPosition () const
{
  static Vector v (-1, -1, 0);
  return v;
}

Time
GodLocationTable::GetEntryUpdateTime (const Address &addr) const
{
  (void) addr;
  NS_LOG_DEBUG ("GodLocationTable has always a position at time Now");
  return Simulator::Now ();
}

void
GodLocationTable::AddEntry (const Address &addr, const Vector &position)
{
  (void) addr;
  (void) position;
  NS_LOG_DEBUG ("GodLocationTable does not store positions: the Lord knows");
}

void
GodLocationTable::DeleteEntry (const Address &addr)
{
  (void) addr;
  NS_LOG_DEBUG ("GodLocationTable does not forget positions");
}


void
GodLocationTable::Purge ()
{
  NS_LOG_DEBUG ("GodLocationTable does not purge position");
}

void
GodLocationTable::Clear ()
{
  NS_LOG_DEBUG ("GodLocationTable does not clear positions");
}

GodIpv4LocationTable::GodIpv4LocationTable ()
  : GodLocationTable ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Vector
GodIpv4LocationTable::GetPosition (const Address &addr) const
{
  NS_LOG_FUNCTION (this << addr);
  uint32_t nodes = NodeList ().GetNNodes ();
  Ipv4Address ipv4_addr = Ipv4Address::ConvertFrom (addr);
  Ptr<Node> node;
  Ptr<Ipv4> ipv4;
  uint32_t interfaces;

  for(uint32_t i = 0; i < nodes; ++i)
    {
      node = NodeList ().GetNode (i);
      ipv4 = node->GetObject<Ipv4> ();
      interfaces = ipv4->GetNInterfaces ();
      if (interfaces == 0)
        {
          NS_LOG_WARN ("Node does not have interfaces, no addresses");
        }

      for (uint32_t j = 0; j < interfaces; ++j)
        {
          if (ipv4->GetAddress (j, 0).GetLocal () == ipv4_addr)
            {
              Ptr<MobilityModel> mm = node->GetObject<MobilityModel> ();
              if (mm == 0)
                {
                  NS_LOG_WARN ("Node does not have a mobility model");
                  return GetInvalidPosition ();
                }
              else
                {
                  return mm->GetPosition ();
                }
            }
        }
    }

  return GetInvalidPosition ();
}

Vector
GodIpv6LocationTable::GetPosition (const Address &addr) const
{
  NS_LOG_FUNCTION (this << addr);

  uint32_t nodes = NodeList ().GetNNodes ();
  Ipv6Address ipv6_addr = Ipv6Address::ConvertFrom (addr);
  Ptr<Node> node;
  Ptr<Ipv6> ipv6;
  uint32_t interfaces;

  for(uint32_t i = 0; i < nodes; ++i)
    {
      node = NodeList ().GetNode (i);
      ipv6 = node->GetObject<Ipv6> ();
      interfaces = ipv6->GetNInterfaces ();
      if (interfaces == 0)
        {
          NS_LOG_WARN ("Node does not have interfaces, no addresses");
        }

      for (uint32_t j = 0; j < interfaces; ++j)
        {
          if (ipv6->GetAddress (j, 0).GetAddress () == ipv6_addr)
            {
              Ptr<MobilityModel> mm = node->GetObject<MobilityModel> ();
              if (mm == 0)
                {
                  NS_LOG_WARN ("Node does not have a mobility model");
                  return GetInvalidPosition ();
                }
              else
                {
                  return mm->GetPosition ();
                }
            }
        }
    }

  return GetInvalidPosition ();
}

} // namepsace ns3
