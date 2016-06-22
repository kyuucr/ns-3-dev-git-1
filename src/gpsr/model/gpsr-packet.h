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
#ifndef GPSRPACKET_H
#define GPSRPACKET_H

#include <iostream>
#include "ns3/header.h"

namespace ns3 {

class GpsrTypeHeader : public Header
{
public:
  enum MessageType
  {
    GPSR_HELLO = 1,
    GPSR_POS   = 2
  };
};

class GpsrHelloHeader : public GpsrTypeHeader
{
public:
  GpsrHelloHeader ();

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  void SetOriginPosx (uint64_t posx)
  {
    m_originPosx = posx;
  }

  uint64_t GetOriginPosx () const
  {
    return m_originPosx;
  }

  void SetOriginPosy (uint64_t posy)
  {
    m_originPosy = posy;
  }

  uint64_t GetOriginPosy () const
  {
    return m_originPosy;
  }

  bool operator== (GpsrHelloHeader const & o) const;
private:
  uint64_t         m_originPosx;          ///< Originator Position x
  uint64_t         m_originPosy;          ///< Originator Position x
};

std::ostream & operator<< (std::ostream & os, GpsrHelloHeader const &);

class GpsrPositionHeader : public GpsrTypeHeader
{
public:
  GpsrPositionHeader ();

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  void SetDstPosx (uint64_t posx)
  {
    m_dstPosx = posx;
  }
  uint64_t GetDstPosx () const
  {
    return m_dstPosx;
  }
  void SetDstPosy (uint64_t posy)
  {
    m_dstPosy = posy;
  }
  uint64_t GetDstPosy () const
  {
    return m_dstPosy;
  }
  void SetUpdated (uint32_t updated)
  {
    m_updated = updated;
  }
  uint32_t GetUpdated () const
  {
    return m_updated;
  }
  void SetRecPosx (uint64_t posx)
  {
    m_recPosx = posx;
  }
  uint64_t GetRecPosx () const
  {
    return m_recPosx;
  }
  void SetRecPosy (uint64_t posy)
  {
    m_recPosy = posy;
  }
  uint64_t GetRecPosy () const
  {
    return m_recPosy;
  }
  void SetInRec (uint8_t rec)
  {
    m_inRec = rec;
  }
  uint8_t GetInRec () const
  {
    return m_inRec;
  }
  void SetLastPosx (uint64_t posx)
  {
    m_lastPosx = posx;
  }
  uint64_t GetLastPosx () const
  {
    return m_lastPosx;
  }
  void SetLastPosy (uint64_t posy)
  {
    m_lastPosy = posy;
  }
  uint64_t GetLastPosy () const
  {
    return m_lastPosy;
  }

  bool operator== (GpsrPositionHeader const & o) const;
private:
  uint64_t         m_dstPosx;    ///< Destination Position x
  uint64_t         m_dstPosy;    ///< Destination Position x
  uint32_t         m_updated;    ///< Time of last update
  uint64_t         m_recPosx;    ///< x of position that entered Recovery-mode
  uint64_t         m_recPosy;    ///< y of position that entered Recovery-mode
  uint8_t          m_inRec;      ///< 1 if in Recovery-mode, 0 otherwise
  uint64_t         m_lastPosx;   ///< x of position of previous hop
  uint64_t         m_lastPosy;   ///< y of position of previous hop
};

std::ostream & operator<< (std::ostream & os, GpsrPositionHeader const &);

}
#endif /* GPSRPACKET_H */
