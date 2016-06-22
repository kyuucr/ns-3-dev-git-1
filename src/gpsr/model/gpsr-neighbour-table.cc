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
#include "gpsr-neighbour-table.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"

#include <algorithm>
#include <complex>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GpsrTable");

AbstractGpsrNeighbourTable::AbstractGpsrNeighbourTable ()
  : AbstractLocationTable (),
    m_lifeTime (Seconds (2))
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
AbstractGpsrNeighbourTable::SetLifeTime (const Time &lifeTime)
{
  m_lifeTime = lifeTime;
}

Vector
AbstractGpsrNeighbourTable::GetInvalidPosition () const
{
  static Vector v (-1, -1, 0);
  return v;
}

double
AbstractGpsrNeighbourTable::GetAngle (const Vector &centrePos, const Vector &refPos,
                                     const Vector &node)
{
  NS_LOG_FUNCTION (centrePos << refPos << node);
  static double const PI = 4*atan(1);

  std::complex<double> A = std::complex<double>(centrePos.x,centrePos.y);
  std::complex<double> B = std::complex<double>(node.x,node.y);
  std::complex<double> C = std::complex<double>(refPos.x,refPos.y);   //Change B with C if you want angles clockwise

  std::complex<double> AB; //reference edge
  std::complex<double> AC;
  std::complex<double> tmp;
  std::complex<double> tmpCplx;

  std::complex<double> Angle;

  AB = B - A;
  AB = (real(AB)/norm(AB)) + (std::complex<double>(0.0,1.0)*(imag(AB)/norm(AB)));

  AC = C - A;
  AC = (real(AC)/norm(AC)) + (std::complex<double>(0.0,1.0)*(imag(AC)/norm(AC)));

  tmp = log(AC/AB);
  tmpCplx = std::complex<double>(0.0,-1.0);
  Angle = tmp*tmpCplx;
  Angle *= (180/PI);

  if (real (Angle) < 0)
    {
      Angle = 360+real(Angle);
    }

  return real(Angle);
}


GpsrIpv4NeighbourTable::GpsrIpv4NeighbourTable ()
  : AbstractGpsrNeighbourTable ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Time 
GpsrIpv4NeighbourTable::GetEntryUpdateTime (const Address &addr) const
{
  NS_LOG_FUNCTION (this << addr);

  if (! Ipv4Address::IsMatchingType(addr))
    {
      NS_FATAL_ERROR (addr << " is not Ipv4Address");
    }

  Ipv4Address ipv4_addr = Ipv4Address::ConvertFrom (addr);

  if (ipv4_addr == Ipv4Address::GetZero ())
    {
      return Time::Min ();
    }

  AddressPositionMap::const_iterator i = m_table.find (addr);

  if (i == m_table.end ())
    {
      NS_LOG_WARN (addr << " is not present in the table");
      return Time::Min ();
    }

  return GetTimeFromIterator (i);
}

void 
GpsrIpv4NeighbourTable::AddEntry (const Address &addr, const Vector &position)
{
  NS_LOG_FUNCTION (this << addr << position);

  AddressPositionMap::iterator i = m_table.find (addr);

  if (i != m_table.end ())
    {
      m_table.erase (addr);
    }

  m_table.insert (std::make_pair (addr, std::make_pair (position, Simulator::Now ())));
}

void
GpsrIpv4NeighbourTable::DeleteEntry (const Address &addr)
{
  m_table.erase (addr);
}

Vector 
GpsrIpv4NeighbourTable::GetPosition (const Address &addr) const
{
  NS_LOG_FUNCTION (this << addr);

  AddressPositionMap::const_iterator i = m_table.find (addr);

  if (i != m_table.end ())
    {
      return GetPositionFromIterator (i);
    }

  return GpsrIpv4NeighbourTable::GetInvalidPosition ();
}

bool
GpsrIpv4NeighbourTable::IsNeighbour (const Address &addr)
{
  NS_LOG_FUNCTION (this << addr);

  // A neighbour has informed us of its position, otherwise is not a neighbour
  // anymore
  Purge ();
  return HasPosition (addr);
}

void 
GpsrIpv4NeighbourTable::Purge ()
{
  NS_LOG_FUNCTION (this);
  
  for (AddressPositionMap::iterator i = m_table.begin (); i != m_table.end (); ++i)
    {
      if (m_lifeTime + GetEntryUpdateTime (i->first) <= Simulator::Now ())
        {
          i = m_table.erase (i);
        }
    }
}

bool
GpsrIpv4NeighbourTable::IsInSearch (const Address &addr) const
{
  (void) addr;
  return false;
}

bool
GpsrIpv4NeighbourTable::HasPosition (const Address &addr) const
{
  AddressPositionMap::const_iterator i = m_table.find (addr);

  if (i != m_table.end ())
    {
      return true;
    }

  return false;
}

void 
GpsrIpv4NeighbourTable::Clear ()
{
  NS_LOG_FUNCTION (this);
  m_table.clear ();
}

Address
GpsrIpv4NeighbourTable::BestNeighbour (const Vector &position, const Vector &nodePos)
{
  NS_LOG_FUNCTION (this << position << nodePos);

  Purge ();

  if (m_table.empty ())
    {
      NS_LOG_DEBUG ("BestNeighbor table is empty; Position: " << position);
      return Address ();
    }

  const double initialDistance = CalculateDistance (nodePos, position);

  Address bestFoundID = GetAddressFromIterator (m_table.begin ());
  double bestFoundDistance = CalculateDistance (GetPositionFromIterator (m_table.begin ()), position);

  AddressPositionMap::const_iterator i;
  for (i = m_table.begin (); i != m_table.end (); ++i)
    {
      double dist = CalculateDistance (GetPositionFromIterator (i), position);
      if (bestFoundDistance > dist)
        {
          bestFoundID = GetAddressFromIterator (i);
          bestFoundDistance = dist;
        }
    }

  if (initialDistance > bestFoundDistance)
    {
      return bestFoundID;
    }
  else
    {
      return Address ();
    }
}

Address
GpsrIpv4NeighbourTable::BestAngle (const Vector &previousHop, const Vector &nodePos)
{
  NS_LOG_FUNCTION (this << previousHop << nodePos);
  Purge ();

  if (m_table.empty ())
    {
      NS_LOG_DEBUG ("BestNeighbor table is empty; Position: " << nodePos);
      return Address ();
    }

  double tmpAngle;
  bool found = false;
  Address bestFoundID = Address ();
  double bestFoundAngle = 360;
  AddressPositionMap::const_iterator i;

  for (i = m_table.begin (); i != m_table.end (); ++i)
    {
      tmpAngle = GetAngle (nodePos, previousHop, GetPositionFromIterator (i));
      if (bestFoundAngle > tmpAngle && tmpAngle != 0)
        {
          bestFoundID = GetAddressFromIterator (i);
          bestFoundAngle = tmpAngle;
          found = true;
        }
    }

  if (! found)
    {
      return GetAddressFromIterator (m_table.begin ());
    }

  return bestFoundID;
}

} // namespace s3
