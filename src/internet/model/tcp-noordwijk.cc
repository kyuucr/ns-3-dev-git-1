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

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) \
    { std::clog << Simulator::Now ().GetSeconds () << \
        " [node " << m_node->GetId () << "] "; }

#include "tcp-noordwijk.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/abort.h"

NS_LOG_COMPONENT_DEFINE ("TcpNoordwijk");

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (TcpNoordwijk);

#define LAMBDA 2
#define STAB 2
#define BURST_MIN (m_segmentSize * 3)

// Assumption: defBurstWnd <= m_rWnd , so SETUP RIGHT Timer and InitialCwnd

TypeId
TcpNoordwijk::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpNoordwijk")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpNoordwijk> ()
    .AddAttribute ("TxTime", "Default transmission timer",
                   TimeValue (MilliSeconds (500)),
                   MakeTimeAccessor (&TcpNoordwijk::m_initialTxTime),
                   MakeTimeChecker ())
    .AddAttribute ("B", "Congestion thresold",
                   TimeValue (MilliSeconds (200)),
                   MakeTimeAccessor (&TcpNoordwijk::m_congThresold),
                   MakeTimeChecker ())
  ;
  return tid;
}

TcpNoordwijk::TcpNoordwijk () : TcpSocketBase (),
                                m_txTimer (Timer::CANCEL_ON_DESTROY),
                                m_trainReceived (0),
                                m_bytesAcked (0),
                                m_minRtt (Time::Max ()),
                                m_maxRtt (Time::Min ()),
                                m_firstAck (Time::FromInteger (0, Time::MS)),
                                m_lastAckedSegmentInRTO (0),
                                m_bytesRetransmitted (0),
                                m_bytesTransmitted (0),
                                m_restore (false),
                                m_firstDupAck (Time::FromInteger (0, Time::MS))
{
  SetTcpNoDelay (true);

  m_cWnd = m_bWnd = m_initialBWnd = 0;
}

void
TcpNoordwijk::SetSSThresh (uint32_t threshold)
{
  (void) threshold;
  NS_LOG_WARN ("TcpNoordwijk does not perform slow start");
}

uint32_t
TcpNoordwijk::GetSSThresh (void) const
{
  NS_LOG_WARN ("TcpNoordwijk does not perform slow start");
  return 0;
}

void
TcpNoordwijk::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED,
                       "TcpNoordwijk::SetInitialCwnd() cannot change initial "
                       "cwnd after connection started.");
  m_initialBWnd = cwnd;
}

uint32_t
TcpNoordwijk::GetInitialCwnd (void) const
{
  return m_initialBWnd;
}

int
TcpNoordwijk::Connect (const Address &address)
{
  NS_LOG_FUNCTION (this);

  int res = TcpSocketBase::Connect (address);

  InitializeCwnd ();
  StartTxTimer ();

  return res;
}

void
TcpNoordwijk::InitializeCwnd ()
{
  NS_LOG_FUNCTION (this);

  // initialBWnd is in segment, as specified by SetInitialCwnd
  m_initialBWnd = m_initialBWnd * m_segmentSize;
  m_bWnd = m_initialBWnd;

  m_txTime = m_initialTxTime;

  NS_ASSERT (m_bWnd <= 65536); // HardCode this.

  NS_LOG_DEBUG ("bWnd: " << m_bWnd << ", txTime: " <<
                m_initialTxTime.GetMilliSeconds () << " ms");
}

void
TcpNoordwijk::StartTxTimer ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (!m_txTimer.IsRunning ());
  NS_ASSERT (m_txTime.Get ().GetSeconds () != 0.0);

  //NS_ASSERT (m_txTime.Get ().GetSeconds () < 1.0);

  m_txTimer.SetFunction (&TcpNoordwijk::CallSendPendingData, this);
  m_txTimer.SetDelay (m_txTime);
  m_txTimer.Schedule ();
}

uint32_t
TcpNoordwijk::Window ()
{
  NS_LOG_FUNCTION (this);

  // We take care of m_rWnd in CallSendPendingData.
  //NS_LOG_DEBUG (" m_cWnd: " << m_cWnd <<
  //              " m_rWnd: " << m_rWnd <<
  //              " UnAck: " << UnAckDataCount () <<
  //              " m_bWnd: " << m_bWnd);
  NS_ASSERT (m_cWnd - UnAckDataCount () <= m_rWnd);

  return m_cWnd;
}

uint32_t
TcpNoordwijk::AvailableWindow ()
{
  uint32_t availableWindow = TcpSocketBase::AvailableWindow ();

  //if (availableWindow <= m_bWnd)
  //  NS_LOG_DEBUG (" m_cWnd: " << m_cWnd <<
  //                " m_rWnd: " << m_rWnd <<
  //                " UnAck: " << UnAckDataCount () <<
  //                " m_bWnd: " << m_bWnd);

  NS_ASSERT (availableWindow <= m_bWnd);

  return availableWindow;
}

void
TcpNoordwijk::CallSendPendingData ()
{
  // Goal: have the right amount of bytes to send in (cWnd), and
  //       add it to the actual UnAckDataCount to create the m_cWnd to use.

  NS_ASSERT (m_bWnd >= BURST_MIN || m_bWnd == 0);
  uint32_t cWnd = m_bWnd;
  uint32_t bRetr = m_bytesRetransmitted;

  if (m_bytesTransmitted != 0)
    {
      NS_ASSERT (m_bytesTransmitted % m_segmentSize == 0);
      m_historyBWnd[m_historyBWnd.size () - 1] = m_bytesTransmitted;
    }

  m_bytesTransmitted = 0;

  if (cWnd >= m_bytesRetransmitted)
    {
      cWnd -= m_bytesRetransmitted;
      m_bytesRetransmitted = 0;
    }
  else
    {
      NS_LOG_WARN ("TcpNoordwijk::CallSendPendingData, retransmitted more than bWnd");
      cWnd = 0;
      m_bytesRetransmitted -= m_bWnd;
    }

  if (cWnd > m_rWnd)
    {
      cWnd = m_rWnd;
      RoundInf (cWnd);
    }

  m_cWnd = UnAckDataCount () + cWnd;

  if (cWnd != 0)
    {
      m_historyBWnd.push_back (cWnd);
    }

  NS_ASSERT (cWnd % m_segmentSize == 0);
  NS_LOG_DEBUG ("cWnd incr: " << cWnd << " rWnd: " << m_rWnd << " bRetr: " << bRetr);

  SendPendingData (true);
}

bool
TcpNoordwijk::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck);

  if (m_cWnd > UnAckDataCount () && m_cWnd - UnAckDataCount () > std::min (m_rWnd.Get (), m_bWnd))
    {
      m_cWnd = UnAckDataCount () + std::min (m_rWnd.Get (), m_bWnd);
      RoundInf (m_cWnd);
    }

  NS_ASSERT (m_cWnd % m_segmentSize == 0);
  SequenceNumber32 head = m_highTxMark.Get ();

  bool res = TcpSocketBase::SendPendingData (withAck);

  uint32_t bytesTransmitted = m_highTxMark.Get () - head;

  if (res)
    {
      NS_LOG_DEBUG ("From:" << head << " To: " << m_highTxMark <<
                    ", #bytes: " << bytesTransmitted << " bWnd: " << m_bWnd);

      Range32 rangeTransmitted (head, m_highTxMark.Get ());
      PairOfRangeAndTime transmitTime (rangeTransmitted, Simulator::Now ());

      m_historyTransmitTime.insert (m_historyTransmitTime.end (), transmitTime);
    }

  NS_ASSERT (bytesTransmitted <= m_bWnd);
  m_bytesTransmitted += bytesTransmitted;

  if (!m_txTimer.IsRunning ())
    {
      StartTxTimer ();
    }

  return res;
}

MapOfTransmitTime::iterator
TcpNoordwijk::GetIteratorOf (SequenceNumber32 seq)
{
  MapOfTransmitTime::iterator it;

  for (it = m_historyTransmitTime.begin (); it != m_historyTransmitTime.end (); ++it)
    {
      Range32 range = it->first;
      if (range.first <= seq && range.second >= seq)
        {
          return it;
        }
    }

  return it;
}

Time
TcpNoordwijk::GetTransmitTime (SequenceNumber32 seq)
{
  MapOfTransmitTime::iterator it = GetIteratorOf (seq);

  if (it != m_historyTransmitTime.end ())
    {
      NS_LOG_LOGIC ("Seq " << seq << " transmitted " << it->second.GetSeconds ());
      return it->second;
    }

  return Time ();
}

void
TcpNoordwijk::DiscardTimeUpTo (SequenceNumber32 seq)
{
  MapOfTransmitTime::iterator it;

  for (it = m_historyTransmitTime.begin (); it != m_historyTransmitTime.end (); ++it)
    {
      Range32 range = it->first;

      if (range.second <= seq)
        {
          break;
        }

      if (range.first >= seq)
        {
          return;
        }
    }

  if (it != m_historyTransmitTime.end ())
    {
      NS_LOG_LOGIC ("Remove Range from " << it->first.first << " to " << it->first.second);
      m_historyTransmitTime.erase (it);
    }
}

void
TcpNoordwijk::NewAck (SequenceNumber32 const& rAck)
{
  NS_LOG_FUNCTION (this << rAck);

  Time now = Simulator::Now ();

  if (m_state == SYN_RCVD)
    {
      TcpSocketBase::NewAck (rAck);
      return;
    }

  uint32_t bytesAcked = rAck - m_txBuffer.HeadSequence ();
  uint32_t segmentAcked = bytesAcked / m_segmentSize;

  NS_ASSERT (bytesAcked >= 0);

  if (bytesAcked % m_segmentSize == 1)
    {
      --bytesAcked;
    }

  NS_ASSERT (bytesAcked % m_segmentSize == 0);

  // Ack stands for first byte receiver needs.. so to split acks we should start
  // from i=1.
  for (uint32_t i = 1; i <= segmentAcked; ++i)
    {
      SequenceNumber32 ack = m_txBuffer.HeadSequence () + (m_segmentSize * i);
      Time rtt = now - GetTransmitTime (ack);
      DiscardTimeUpTo (ack);

      NS_LOG_DEBUG ("ACK num=" << ack << " rtt=" << rtt.GetMilliSeconds ());

      ACCE (m_segmentSize, rtt, now);
    }

  if (m_cWnd >= bytesAcked)
    {
      m_cWnd -= bytesAcked;
    }
  else
    {
      m_cWnd = UnAckDataCount ();
    }

  TcpSocketBase::NewAck (rAck);

  //NS_LOG_DEBUG (" m_cWnd: " << m_cWnd <<
  //              " m_rWnd: " << m_rWnd <<
  //              " UnAck: " << UnAckDataCount ());
}

void
TcpNoordwijk::ACCE (uint32_t bytesAcked, Time rtt, Time now)
{
  NS_LOG_FUNCTION (this << bytesAcked << rtt.GetMilliSeconds ());
  Time ackTrainDispersion, ackDispersion, deltaRtt;

  if (m_minRtt > rtt)
    {
      m_minRtt = rtt;
    }

  if (m_maxRtt < rtt)
    {
      m_maxRtt = rtt;
    }

  if (m_firstAck.IsZero ())
    {
      m_firstAck = now;
      NS_LOG_DEBUG ("Time first ack " << now.GetSeconds ());
    }

  uint32_t bWnd = m_historyBWnd.front ();

  if (bWnd == 0) // m_historyBWnd is empty
    {
      return;
    }

  m_bytesAcked += bytesAcked;

  if (m_bytesAcked >= bWnd)
    {
      NS_LOG_DEBUG ("Time last ack " << now.GetSeconds ());
      ackTrainDispersion = now - m_firstAck;
      ackDispersion      = MicroSeconds (
          ackTrainDispersion.GetMicroSeconds () / (bWnd / m_segmentSize));
      deltaRtt           = m_maxRtt - m_minRtt;

      if ((bWnd == m_initialBWnd && ackDispersion.GetSeconds () != 0.0)
          || m_ackDispersion.GetSeconds () == 0.0)
        {
          m_ackDispersion = ackDispersion;
        }

      NS_ASSERT (m_ackDispersion.GetSeconds () != 0.0);
      NS_LOG_DEBUG ("ACK train of " << bWnd / m_segmentSize << " acks rcvd. TrainDisp="
                                    << ackTrainDispersion.GetMilliSeconds () << " ms, AckDisp= "
                                    << ackDispersion.GetMicroSeconds () << " us. Using AckDisp="
                                    << m_ackDispersion.GetMicroSeconds () << " us");

      TrainReceived (bWnd, ackTrainDispersion, deltaRtt);

      m_historyBWnd.pop_front ();
      m_bytesAcked -= bWnd;
      m_minRtt     = Time::Max ();
      m_maxRtt     = Time::Min ();
      m_firstAck   = Time::FromInteger (0, Time::MS);

      NS_ASSERT (m_bWnd <= m_initialBWnd);

      if (m_bytesAcked > 0)
        {
          uint32_t delta = bytesAcked - bWnd;
          m_bytesAcked -= delta;
          ACCE (delta, rtt, now);
        }
    }
}

void
TcpNoordwijk::TrainReceived (uint32_t bWnd, Time ackTrainDispersion, Time deltaRtt)
{
  NS_LOG_FUNCTION (this << bWnd << ackTrainDispersion);

  ++m_trainReceived;

  if (m_trainReceived < STAB)
    {
      return;
    }

  NS_LOG_DEBUG ("Stab reached. DeltaRtt=" << deltaRtt.GetMilliSeconds () << " ms");

  if (ackTrainDispersion.GetSeconds () != 0.0 && deltaRtt.GetSeconds () != 0.0)
    {
      if (deltaRtt > m_congThresold)
        {
          RateAdjustment (bWnd, ackTrainDispersion, deltaRtt);
        }
      else
        {
          RateTracking (bWnd, m_ackDispersion);
        }
    }
  else if (m_dupAckTrainDispersion.GetSeconds () != 0.0 && deltaRtt.GetSeconds () != 0.0)
    {
      if (deltaRtt > m_congThresold)
        {
          RateAdjustment (bWnd, m_dupAckTrainDispersion, deltaRtt);
        }
      else
        {
          RateTracking (bWnd, m_dupAckDispersion);
        }
    }

  if (m_restore)
    {
      m_bWnd = m_initialBWnd;
      m_txTime = m_initialTxTime;
      m_restore = false;
    }

  m_trainReceived = 0;
}

/**
 * \brief Rate Adjustment algorithm (there is congestion)
 *
 * Algorithm invoked when there are some symptom of congestion.
 * Definining terms as:
 *
 * - \f$B_{i+1}\f$ as the next burst size
 * - \f$B_{i}\f$ as the actual burst size
 * (opportunely decreased by the number of retransmitted packets)
 * - \f$\Delta_{i}\f$ as the ack train dispersion (in other words, the arrival
 *    time of first ack minus the arrival time of last ack)
 * - \f${\Delta}RTT_{i}\f$ as the difference between last RTT and the minimum RTT
 * - \f$\delta_{i} = \frac{Time_{LastAck} - Time_{m_firstAck}}{B_{i}}\f$ as the
 *    ack dispersion of the \f$i\f$-th burst
 *
 * We could define next burst size as:
 * \f$B_{i+1} = \frac{\Delta_{i}}{\Delta_{i}+{\Delta}RTT_{i}} \cdot B_{i} = \frac{B_{i}}{1+\frac{{\Delta}RTT_{i}}{\Delta_{i}}}\f$
 *
 * We also compute a new TX_TIMER:
 *
 * \f$TX_TIMER_{i+1} = \lambda * B_{0} * \delta_{i}\f$
 *
 * Where \f$\lambda = 1 \f$ if \f$B_{i+1}\f$ is greater than a fixed value
 * (3 packets), \f$\lambda=2\f$ otherwise.
 *
 * \param ackTrainDispersion arrival time of first ack minus the arrival time of last ack
 * \param deltaRtt difference between last RTT and the minimun RTT
 */
void
TcpNoordwijk::RateAdjustment (uint32_t bWnd, const Time& ackTrainDispersion,
                              const Time& deltaRtt)
{
  NS_LOG_FUNCTION (this << ackTrainDispersion.GetMilliSeconds ()
                        << deltaRtt.GetMilliSeconds ());

  NS_ASSERT (deltaRtt.GetSeconds () != 0.0);
  NS_ASSERT (ackTrainDispersion.GetSeconds () != 0.0);
  NS_LOG_DEBUG ("before: " <<
                " bWnd: " << m_bWnd <<
                " tx_timer:" << m_txTime.Get ().GetMilliSeconds () <<
                " ms, ackdisp=" << m_ackDispersion.GetMilliSeconds () <<
                " ms, AckTrainDisp=" << ackTrainDispersion.GetMilliSeconds () <<
                " ms, DRTT:" << deltaRtt.GetMilliSeconds () <<
                " ms");

  uint32_t oldBWnd = m_bWnd;

  double tmp = (ackTrainDispersion.GetSeconds () /
                (ackTrainDispersion.GetSeconds () + deltaRtt.GetSeconds ()));
  NS_ASSERT (tmp <= 1.0);

  m_bWnd = m_bWnd * tmp;

  // The burst window could be not "aligned" with the segment size; round up
  // to the next multiple of the segment size.
  RoundSupBurstWnd ();

  if (m_bWnd > BURST_MIN)
    {
      m_txTime = MilliSeconds ((m_initialBWnd / m_segmentSize) * m_ackDispersion.GetMilliSeconds ());
    }
  else
    {
      m_txTime = MilliSeconds (LAMBDA * (m_initialBWnd / m_segmentSize) * m_ackDispersion.GetMilliSeconds ());
    }

  if (m_txTime.Get ().GetSeconds () == 0.0)
    {
      m_txTime = m_initialTxTime;
    }

  if (oldBWnd > m_bWnd)
    {
      uint32_t diff = oldBWnd - m_bWnd;

      if (m_cWnd - UnAckDataCount () > diff)
        {
          m_cWnd -= diff;
        }
      else
        {
          m_cWnd = UnAckDataCount ();
        }

      NS_ASSERT (m_bWnd >= m_cWnd - UnAckDataCount ());
    }

  RoundTxTime ();

  NS_LOG_DEBUG (" after: " <<
                " bWnd: " << m_bWnd <<
                " tx_timer:" << m_txTime.Get ().GetMilliSeconds () <<
                " ms");
}

/**
 * \brief Rate tracking algorithm (no congestion)
 *
 * This algorithm aims at adapting transmission rate to the maximum allowed
 * rate through the following steps: gradually increase burst size (logarithmic
 * grow) up to the initial burst size, and tx_timer is fixed to the optimal
 * value for default-sized bursts.
 *
 * With
 *
 * - \f$B_{0}\f$ as the default burst size
 * - \f$B_{i+1}\f$ as the next burst size
 * - \f$B_{i}\f$ as the actual burst size
 * - \f$\delta_{i} = \frac{Time_{LastAck} - Time_{m_firstAck}}{B_{i}}\f$ as
 *   the ack dispersion of the \f$i\f$-th burst
 *
 * We could define next burst size as:
 *
 * \f$B_{i+1} = B_{i} + \frac{B_{0}-B_{i}}{2}\f$
 *
 * We update also the tx_timer here, with the following function:
 *
 * \f$TX_TIMER_{i+1} = B_{0} * \delta_{i}\f$
 */
void
TcpNoordwijk::RateTracking (uint32_t bWnd, Time ackDispersion)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (ackDispersion.GetSeconds () != 0.0);

  NS_LOG_DEBUG (" before:" <<
                " bWnd=" << m_bWnd <<
                " B, tx_timer=" << m_txTime.Get ().GetMilliSeconds () <<
                " ms, AckDisp: " << m_ackDispersion.GetMilliSeconds () <<
                " ms, def burst: " << m_initialBWnd);

  uint32_t bytesToAdd = (m_initialBWnd - m_bWnd) / 2;

  m_bWnd += bytesToAdd;

  // The burst window could be not "aligned" with the segment size; round up
  // to the previous multiple of the segment size.
  RoundSupBurstWnd ();

  m_txTime = MicroSeconds ((m_initialBWnd / m_segmentSize) * ackDispersion.GetMicroSeconds ());

  RoundTxTime ();

  NS_LOG_DEBUG (" after:" <<
                " bWnd=" << m_bWnd <<
                " B, tx_timer=" << m_txTime.Get ().GetMilliSeconds () <<
                " ms");
}

void
TcpNoordwijk::RoundSupBurstWnd ()
{
  if (m_bWnd % m_segmentSize != 0)
    {
      uint32_t diff = m_bWnd % m_segmentSize;
      m_bWnd -= diff;

      m_bWnd += m_segmentSize;
    }

  // Stupid coder
  if (m_bWnd > m_initialBWnd)
    {
      m_bWnd -= m_segmentSize;
    }

  NS_ASSERT (m_bWnd <= m_initialBWnd);
}

void
TcpNoordwijk::RoundInf (uint32_t &value)
{
  if (value % m_segmentSize != 0)
    {
      uint32_t diff = value % m_segmentSize;
      value -= diff;
    }
}

void
TcpNoordwijk::RoundTxTime ()
{
  /** \todo m_bWnd *= 0.5 is ugly. Please do little steps */
  while (m_txTime.Get ().GetSeconds () > 1.0)
    {
      m_txTime = m_txTime.Get () / 2;
      m_bWnd = m_bWnd / 2;
      RoundSupBurstWnd ();
    }

  /* Wow. That's bad. We adjust the rate, because txTimer is the same */
  if (m_bWnd < BURST_MIN)
    {
      m_bWnd = BURST_MIN;
    }
}

/**
 * \brief Received a DupAck for a segment size
 *
 * There is (probably) a loss when we detect a triple dupack, so resend
 * the next segment of duplicate ack. Increase also the retransmitted packet
 * count.
 *
 * \param t TcpHeader (unused)
 * \param count Dupack count
 */
void
TcpNoordwijk::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << "t " << count);

  NS_LOG_DEBUG ("Dupack for segment " << t.GetAckNumber () << " count=" << count);

/*  if (m_firstDupAck.IsZero ())
    m_firstDupAck = Simulator::Now ();

  if (count % 20 == 0)
    {
      m_dupAckTrainDispersion = Simulator::Now () - m_firstDupAck;
      m_dupAckDispersion = MicroSeconds(m_dupAckTrainDispersion.GetMicroSeconds () / 20);

      m_firstDupAck = Time::FromInteger (0, Time::S);
      m_restore = true;
    }
*/
// Wait the third dupack
  if (count != 6)
    {
      return;
    }


  //if (m_bytesRetransmitted >= m_bWnd)
  //  return;

  NS_LOG_DEBUG ("Doing a retransmit of " << m_segmentSize << " bytes. Reset TxTimer");

  DoRetransmit ();
  m_bytesRetransmitted += m_segmentSize;

  m_initialTxTime = m_initialTxTime + m_initialTxTime;
  m_bWnd = m_initialBWnd;
  m_txTime = m_initialTxTime;
  m_restore = true;

  if (m_txTimer.IsRunning ())
    {
      Time left = m_txTimer.GetDelayLeft ();
      m_txTimer.Cancel ();
      m_txTimer.SetDelay (m_txTime - left);
    }

  RoundTxTime ();
}

/**
 * \brief Retransmit timeout
 *
 * Retransmit first non ack packet, increase the retransmitted packet count,
 * and set recover flag for the initial values of burst size and tx_timer
 *
 * In case of consecutive RTO expiration, retransmission is repeated while the
 * default tx_timer is doubled.
 */
void
TcpNoordwijk::Retransmit (void)
{
  NS_LOG_FUNCTION (this);

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT)
    {
      return;
    }
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark)
    {
      return;
    }

  NS_LOG_DEBUG ("Retransmit");

//  if (m_txBuffer.HeadSequence () == m_lastAckedSegmentInRTO)
//    {
  m_initialTxTime = m_initialTxTime + m_initialTxTime;
  if (m_initialTxTime.GetSeconds () > 1.0)
    {
      m_initialTxTime = Seconds (0.9);
    }

  m_txTime = m_initialTxTime;
  m_restore = true;

  if (m_txTimer.IsRunning ())
    {
      Time left = m_txTimer.GetDelayLeft ();
      m_txTimer.Cancel ();
      m_txTimer.SetDelay (m_txTime - left);
    }
//    }

  m_lastAckedSegmentInRTO = m_txBuffer.HeadSequence ();

  TcpSocketBase::Retransmit ();

  m_bytesRetransmitted += m_segmentSize;
  m_cWnd = UnAckDataCount ();
}

Ptr<TcpSocketBase>
TcpNoordwijk::Fork (void)
{
  return CopyObject<TcpNoordwijk> (this);
}
