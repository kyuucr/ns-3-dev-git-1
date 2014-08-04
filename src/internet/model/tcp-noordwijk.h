/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Natale Patriciello <natale.patriciello@gmail.com>
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

#ifndef TCPNOORDWIJK_H
#define TCPNOORDWIJK_H

#include "ns3/tcp-socket-base.h"
#include "ns3/timer.h"

#include <deque>
#include <map>

namespace ns3 {

typedef std::pair<SequenceNumber32, SequenceNumber32> Range32;
typedef std::pair<Range32, Time> PairOfRangeAndTime;
typedef std::map<Range32, Time> MapOfTransmitTime;

/** \brief TcpNoordwijk implementation
 *
 * TCP Naordwijk is a new transport protocol designed to optimize
 * performance in a controlled environment whose characteristics
 * are fairly known and managed, such as DVB-RCS link between I-PEPs.
 * Main requirements in the protocol design were the optimization of
 * the web traffic performance, while keeping good performance for
 * large file transfers, and efficent bandwidth utilization over DAMA
 * schemes. To achieve such goals, TCP Noordwijk proposes a novel
 * sender-only modification to the standard TCP algorithms based
 * on a burst transmission.
 *
 * To follow the protocol design and how it works, start from the function
 * TcpNoordwijk::SendPendingData.
 *
 * \see SendPendingData
 */
class TcpNoordwijk : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);

  TcpNoordwijk ();

protected:
  virtual int Connect (const Address &address);
  virtual void NewAck (SequenceNumber32 const& rAck); // Update buffers w.r.t. ACK
  virtual Ptr<TcpSocketBase> Fork (void); // Call CopyObject<TcpReno> to clone me
  virtual void DupAck (const TcpHeader& t, uint32_t count);
  virtual void Retransmit (void); // Retransmit timeout
  virtual bool SendPendingData (bool withAck = false); // Send as much as the window allows
  virtual uint32_t Window (void); // Return the max possible number of unacked bytes
  virtual uint32_t AvailableWindow (void);

  // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetSSThresh (uint32_t threshold);
  virtual uint32_t GetSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;

  /** Initial burst window */
  uint32_t m_initialBWnd;

  /** Actual burst window */
  uint32_t m_bWnd;
  std::deque<uint32_t> m_historyBWnd;

  /** Actual congestion window */
  uint32_t m_cWnd;

  /** Initial tx time */
  Time m_initialTxTime;

  Timer m_txTimer;

  /** Actual tx timer */
  TracedValue<Time> m_txTime;

  virtual void RateTracking (uint32_t bWnd, Time ackDispersion);
  virtual void RateAdjustment (uint32_t bWnd, const Time& delta, const Time& deltaRtt);

  void RoundSupBurstWnd ();
  void RoundInf (uint32_t &value);
  void RoundTxTime ();

private:
  void StartTxTimer ();
  void CallSendPendingData ();
  void InitializeCwnd (void);

  MapOfTransmitTime::iterator GetIteratorOf (SequenceNumber32 seq);
  Time GetTransmitTime (SequenceNumber32 seq);
  void DiscardTimeUpTo (SequenceNumber32 seq);

  void ACCE (uint32_t bytesAcked, Time rtt, Time now);
  void TrainReceived (uint32_t bWnd, Time ackTrainDispersion, Time deltaRtt);

  uint8_t m_trainReceived;
  uint32_t m_bytesAcked;
  Time m_minRtt;
  Time m_maxRtt;
  Time m_firstAck;
  SequenceNumber32 m_lastAckedSegmentInRTO;

  Time m_congThresold;
  Time m_ackDispersion;

  uint32_t m_bytesRetransmitted;
  uint32_t m_bytesTransmitted;

  bool m_restore;

  MapOfTransmitTime m_historyTransmitTime;

  Time m_firstDupAck;
  Time m_dupAckDispersion;
  Time m_dupAckTrainDispersion;
};

} // namespace ns3

#endif // TCPNOORDWIJK_H
