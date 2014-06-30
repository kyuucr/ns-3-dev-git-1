/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello, <natale.patriciello@gmail.com>
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
 */

#ifndef TCPHIGHSPEED_H
#define TCPHIGHSPEED_H

#include "ns3/tcp-newreno.h"

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief An implementation of Tcp HighSpeed
 *
 * This class contains the HighSpeed TCP implementation, as of \RFC{3649}.
 */
class TcpHighSpeed : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  TcpHighSpeed (void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpHighSpeed (const TcpHighSpeed& sock);

  virtual ~TcpHighSpeed (void);

protected:
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual void NewAck (SequenceNumber32 const& seq);
  virtual void DupAck (const TcpHeader& t, uint32_t count);

private:
  // Functions that return default values, from RFC 3649
  uint32_t  TableLookupA (uint32_t w);
  float     TableLookupB (uint32_t w);
};

} // namespace ns3

#endif // TCPHIGHSPEED_H
