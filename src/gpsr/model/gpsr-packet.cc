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

#include "gpsr-packet.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GpsrPacket");

//-----------------------------------------------------------------------------
// HELLO
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (GpsrHelloHeader);

GpsrHelloHeader::GpsrHelloHeader ()
  : GpsrTypeHeader (),
    m_originPosx (0),
    m_originPosy (0)
{
}

TypeId
GpsrHelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::HelloHeader")
    .SetParent<GpsrTypeHeader> ()
    .AddConstructor<GpsrHelloHeader> ()
  ;
  return tid;
}

TypeId
GpsrHelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
GpsrHelloHeader::GetSerializedSize () const
{
  return 17;
}

void
GpsrHelloHeader::Serialize (Buffer::Iterator i) const
{
  NS_LOG_DEBUG ("Serialize X " << m_originPosx << " Y " << m_originPosy);

  i.WriteU8 (GpsrTypeHeader::GPSR_HELLO);
  i.WriteHtonU64 (m_originPosx);
  i.WriteHtonU64 (m_originPosy);
}

uint32_t
GpsrHelloHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  uint8_t type = i.ReadU8 ();

  if (type != GpsrTypeHeader::GPSR_HELLO)
    {
      NS_LOG_DEBUG ("This is not an HELLO packet, returning");
      return 0;
    }

  m_originPosx = i.ReadNtohU64 ();
  m_originPosy = i.ReadNtohU64 ();

  NS_LOG_DEBUG ("Deserialize X " << m_originPosx << " Y " << m_originPosy);

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());

  return dist;
}

void
GpsrHelloHeader::Print (std::ostream &os) const
{
  os << " PositionX: " << m_originPosx
     << " PositionY: " << m_originPosy;
}

bool
GpsrHelloHeader::operator== (GpsrHelloHeader const & o) const
{
  return (m_originPosx == o.m_originPosx && m_originPosy == o.m_originPosy);
}

std::ostream &
operator<< (std::ostream & os, GpsrHelloHeader const & h)
{
  h.Print (os);
  return os;
}


//-----------------------------------------------------------------------------
// Position
//-----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (GpsrPositionHeader);
GpsrPositionHeader::GpsrPositionHeader ()
  : GpsrTypeHeader (),
    m_dstPosx (0),
    m_dstPosy (0),
    m_updated (0),
    m_recPosx (0),
    m_recPosy (0),
    m_inRec (0),
    m_lastPosx (0),
    m_lastPosy (0)
{
}

TypeId
GpsrPositionHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::GpsrPositionHeader")
    .SetParent<GpsrTypeHeader> ()
    .AddConstructor<GpsrPositionHeader> ()
  ;
  return tid;
}

TypeId
GpsrPositionHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
GpsrPositionHeader::GetSerializedSize () const
{
  return 54;
}

void
GpsrPositionHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (GpsrTypeHeader::GPSR_POS);
  i.WriteU64 (m_dstPosx);
  i.WriteU64 (m_dstPosy);
  i.WriteU32 (m_updated);
  i.WriteU64 (m_recPosx);
  i.WriteU64 (m_recPosy);
  i.WriteU8 (m_inRec);
  i.WriteU64 (m_lastPosx);
  i.WriteU64 (m_lastPosy);
}

uint32_t
GpsrPositionHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();

  if (type != GpsrTypeHeader::GPSR_POS)
    {
      NS_LOG_DEBUG ("This is not an POS packet, returning");
      return 0;
    }

  m_dstPosx = i.ReadU64 ();
  m_dstPosy = i.ReadU64 ();
  m_updated = i.ReadU32 ();
  m_recPosx = i.ReadU64 ();
  m_recPosy = i.ReadU64 ();
  m_inRec = i.ReadU8 ();
  m_lastPosx = i.ReadU64 ();
  m_lastPosy = i.ReadU64 ();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
GpsrPositionHeader::Print (std::ostream &os) const
{
  os << " PositionX: "  << m_dstPosx
     << " PositionY: " << m_dstPosy
     << " Updated: " << m_updated
     << " RecPositionX: " << m_recPosx
     << " RecPositionY: " << m_recPosy
     << " inRec: " << m_inRec
     << " LastPositionX: " << m_lastPosx
     << " LastPositionY: " << m_lastPosy;
}

std::ostream &
operator<< (std::ostream & os, GpsrPositionHeader const & h)
{
  h.Print (os);
  return os;
}

bool
GpsrPositionHeader::operator== (GpsrPositionHeader const & o) const
{
  return (m_dstPosx == o.m_dstPosx && m_dstPosy == o.m_dstPosy
          && m_updated == o.m_updated && m_recPosx == o.m_recPosx
          && m_recPosy == o.m_recPosy && m_inRec == o.m_inRec
          && m_lastPosx == o.m_lastPosx && m_lastPosy == o.m_lastPosy);
}


} // namespace ns3
