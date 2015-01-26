/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello <natale.patriciello@gmail.com>
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
#ifndef TCPHYBLA_H
#define TCPHYBLA_H

#include "ns3/tcp-newreno.h"

namespace ns3 {

/**
 * \brief Implementation of the Tcp Hybla algorithm
 *
 * TCP Hybla aims to eliminate penalization of TCP connections that
 * incorporate a high-latency terrestrial or satellite radio link, due
 * to their longer round trip times. It stems from an analytical evaluation
 * of the congestion window dynamics, which suggests the necessary
 * modifications to remove the performance dependence on RTT.
 */
class TcpHybla : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpHybla ();

protected:
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual void NewAck (SequenceNumber32 const& seq);
  virtual void DupAck (const TcpHeader& t, uint32_t count);  // Halving cwnd and reset nextTxSequence
  virtual void Retransmit (void); // Exit fast recovery upon retransmit timeout

  virtual void InitializeCwnd (void);

protected:
  double     m_rho;         //!< Rho parameter
  Time       m_minRtt;      //!< Minimum smoothed round trip time value seen
  Time       m_rRtt;        //!< Reference RTT

private:
  /**
   * \brief Initialize the algorithm
   */
  void Init (void);

  /**
   * \brief Recalculate algorithm paramenters
   */
  void RecalcParam (void);
};

} // namespace ns3

#endif // TCPHYBLA_H
