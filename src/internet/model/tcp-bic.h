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

#ifndef TCPBIC_H
#define TCPBIC_H

#include "ns3/tcp-socket-base.h"
#include "ns3/timer.h"

namespace ns3 {

/** \brief BIC congestion control algorithm
 *
 * BIC is optimized for high speed networks with high latency:
 * so-called "long fat networks".
 *
 * BIC has a unique congestion window (cwnd) algorithm. This algorithm
 * tries to find the maximum where to keep the window at for a long period
 * of time, by using a binary search algorithm.
 */
class TcpBic : public TcpSocketBase
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Constructor
   */
  TcpBic ();

  // From TcpSocketBase
  virtual int Connect (const Address &address);
  virtual int Listen (void);

protected:
  virtual uint32_t Window (void);   // Return the max possible number of unacked bytes
  virtual Ptr<TcpSocketBase> Fork (void);   // Call CopyObject<TcpNewReno> to clone me
  virtual void NewAck (SequenceNumber32 const& seq);
  virtual void DupAck (const TcpHeader& t, uint32_t count);    // Halving cwnd and reset nextTxSequence
  virtual void Retransmit (void);   // Exit fast recovery upon retransmit timeout

  // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetInitialSSThresh (uint32_t threshold);
  virtual uint32_t GetInitialSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;
  virtual void ScaleSsThresh (uint8_t scaleFactor);

protected:
  /**
   * \brief State of the congestion control machine: open or loss
   */
  enum BicState
  {
    OPEN     = 0x1, //!< Open state
    LOSS     = 0x2, //!< Loss state
  };

  uint32_t m_initialCwnd;      //!< Initial cWnd

  // User parameters
  bool     m_fastConvergence;  //!< Enable or disable fast convergence algorithm
  double   m_beta;             //!< Beta for cubic multiplicative increase
  uint32_t m_maxIncr;          //!< Maximum window increment
  uint32_t m_lowWnd;           //!< Lower bound on congestion window
  int m_smoothPart;            //!< Number of RTT needed to reach Wmax from Wmax-B

  // Bic parameters
  uint32_t     m_cWndCnt;         //!<  cWnd integer-to-float counter
  uint32_t     m_lastMaxCwnd;     //!<  Last maximum cWnd
  uint32_t     m_lastCwnd;        //!<  Last cWnd
  Time         m_epochStart;      //!<  Beginning of an epoch
  Time         m_lastTime;

  TracedValue<uint32_t> m_cWnd;     //!< Congestion window
  TracedValue<uint32_t> m_ssThresh; //!< Slow start threshold

  uint8_t m_bicState;             //!< Bic state

private:
  /**
   * \brief Reset BIC parameters
   */
  void Reset (void);

  /**
   * \brief Initialize the algorithm
   */
  void Init (void);

  /**
   * \brief The congestion avoidance phase of Cubic
   */
  void CongAvoid (void);

  /**
   * \brief Bic window update after a new ack received
   */
  uint32_t Update(void);

  /**
   * \brief Cubic window update after a loss
   */
  void RecalcSsthresh (void);
};

} // namespace ns3
#endif // TCPBIC_H
