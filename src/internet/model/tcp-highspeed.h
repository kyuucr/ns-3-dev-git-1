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
 * When an ACK is received (in congestion avoidance), the window is increased
 * by a(w)/w and when a loss is detected through triple duplicate
 * acknowledgments, the window is decreased by (1-b(w))w, where w is the current
 * window size. When the congestion window is small, HSTCP behaves exactly
 * like standard TCP so a(w) is 1 and b(w) is 0.5. When TCP's congestion window
 * is beyond a certain threshold, a(w) and b(w) become functions of the
 * current window size. In this region, as the congestion window increases,
 * the value of a(w) increases and the value of b(w) decreases.
 * This means that HSTCP's window will grow faster than standard TCP
 * and also recover from losses more quickly. This behavior allows
 * HSTCP to be friendly to standard TCP flows in normal networks and
 * also to quickly utilize available bandwidth in networks with large
 * bandwidth delay products.
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
  virtual void Retransmit (void);

private:
  /**
   * \brief Lookup table for a (from RFC 3649)
   *
   * \param w Window value (in packets)
   *
   * \return the coefficent a
   */
  uint32_t  TableLookupA (uint32_t w);

  /**
   * \brief Lookup table for b (from RFC 3649)
   *
   * \param w Window value (in packets)
   *
   * \return the coefficent b
   */
  double    TableLookupB (uint32_t w);

  /**
   * \brief Update cWnd and ssTh after a loss event
   *
   */
  void Loss ();
};

} // namespace ns3

#endif // TCPHIGHSPEED_H
