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
 * In TCP Bic the congestion control problem is viewed as a search
 * problem. Taking as a starting point the current window value
 * and as a target point the last maximum window value
 * (i.e. the cWnd value just before the loss event) a binary search
 * technique can be used to update the cWnd value at the midpoint between
 * the two, directly or using an additive increase strategy if the distance from
 * the current window is too large.
 *
 * This way, assuming a no-loss period, the congestion window logarithmically
 * approaches the maximum value of cWnd until the difference between it and cWnd
 * falls below a preset threshold. After reaching such a value (or the maximum
 * window is unknown, i.e. the binary search does not start at all) the algorithm
 * switches to probing the new maximum window with a 'slow start' strategy.
 *
 * If a loss occur in either these phases, the current window (before the loss)
 * can be treated as the new maximum, and the reduced (with a multiplicative
 * decrease factor Beta) window size can be used as the new minimum.
 *
 * To maintain the performance of TCP Bic as close as possible with the Linux
 * implementation, and at the same time maintain the friendliness with other TCP
 * flavors, the cWnd is increased only after a certain number of ACKs
 * are received, following RFC 6356. After the slow start phase, and after each
 * new ACK, a value is calculated by the method Update. This number
 * (m_cnt in the code) represents the ACK packets that should be received
 * before increasing the cWnd by one segment. After a trivial check on the
 * arrived ACKs (represented by m_cWndCnt in the code), the
 * cWnd can be increased and m_cWndCnt can be set to zero, or
 * otherwise m_cWndCnt can be increased by one and the
 * cWnd can be left untouched.
 *
 * The binary search on the cWnd size space is done by varying the returned
 * cnt, depending on the internal state of the class (e.g. the last maximum
 * and the current cWnd size).
 *
 * The reference paper for BIC can be found in:
 * http://an.kaist.ac.kr/courses/2006/cs540/reading/bic-tcp.pdf
 *
 * This model has a number of configurable parameters that are exposed as
 * attributes of the TcpBic TypeId.  This model also exports trace sources,
 * for tracking the congestion window, slow start threshold, and the internat
 * state of the protocol.
 *
 * More information on this implementation: http://dl.acm.org/citation.cfm?id=2756518
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
   * \brief State of the congestion control machine: open or loss
   */
  enum BicState
  {
    OPEN     = 0x1, //!< Open state
    LOSS     = 0x2  //!< Loss state
  };

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
  virtual void     ScaleSsThresh (uint8_t scaleFactor);

protected:
  TracedValue<uint32_t> m_cWnd;     //!< Congestion window
  TracedValue<uint32_t> m_ssThresh; //!< Slow start threshold

private:
  uint32_t m_initialCwnd;      //!< Initial cWnd

  // User parameters
  bool     m_fastConvergence;  //!< Enable or disable fast convergence algorithm
  double   m_beta;             //!< Beta for cubic multiplicative increase
  uint32_t m_maxIncr;          //!< Maximum window increment
  uint32_t m_lowWnd;           //!< Lower bound on congestion window
  int      m_smoothPart;       //!< Number of RTT needed to reach Wmax from Wmax-B

  // Bic parameters
  uint32_t     m_cWndCnt;         //!<  cWnd integer-to-float counter
  uint32_t     m_lastMaxCwnd;     //!<  Last maximum cWnd
  uint32_t     m_lastCwnd;        //!<  Last cWnd
  Time         m_epochStart;      //!<  Beginning of an epoch

  TracedValue<uint8_t>  m_bicState; //!< Bic state

  uint8_t  m_b;                     //!< Binary search coefficient
  uint32_t m_retxThresh;            //!< Fast Retransmit threshold

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
   * \brief The congestion avoidance algorithm of BIC is implemented here
   */
  void CongAvoid (void);

  /**
   * \brief Bic window update after a new ack received
   */
  uint32_t Update (void);

  /**
   * \brief Window update after a loss
   */
  void RecalcSsthresh (void);
};

} // namespace ns3
#endif // TCPBIC_H
