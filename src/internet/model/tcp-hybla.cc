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

#include "tcp-hybla.h"
#include "ns3/log.h"
#include "ns3/node.h"


NS_LOG_COMPONENT_DEFINE ("TcpHybla");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpHybla);

TypeId
TcpHybla::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpHybla")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpHybla> ()
    .AddAttribute ("RRTT", "Reference RTT",
                   TimeValue (MilliSeconds (50)),
                   MakeTimeAccessor (&TcpHybla::m_rRtt),
                   MakeTimeChecker ())
  ;
  return tid;
}

TcpHybla::TcpHybla () : TcpNewReno ()
{
  m_minRtt = Time::Max ();

  m_rho = 1.0;
  m_cWndCnt = 0;

}

void
TcpHybla::InitializeCwnd ()
{
  NS_LOG_FUNCTION (this);

  /* 1st Rho measurement based on initial srtt */
  RecalcParam ();

  /* set minimum rtt as this is the 1st ever seen */
  m_minRtt = m_rtt->GetEstimate();

  m_initialCWnd *= m_rho;
  m_initialSsThresh *= (m_rho*m_segmentSize);

  TcpHybla::InitializeCwnd ();
}

void
TcpHybla::RecalcParam ()
{
  Time rtt = m_rtt->GetEstimate ();

  double oldRho = m_rho;

  m_rho = std::max ((double)rtt.GetMilliSeconds () / m_rRtt.GetMilliSeconds (), 1.0);

  if (m_rho >= 20)
    {
      m_rho = 1.0;
    }

  /* Bring back the ssThresh to the original value, without m_rho multiplied */
  m_ssThresh /= (oldRho*m_segmentSize);

  /* Now update ssThresh */
  m_ssThresh *= (m_rho*m_segmentSize);

  NS_LOG_DEBUG ("Calculated rho=" << m_rho);
}

void
TcpHybla::NewAck (const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this);

  double increment;
  bool isSlowstart = false;

  Time rtt = m_rtt->GetEstimate ();

  /*  Recalculate rho only if this srtt is the lowest */
  if (rtt < m_minRtt)
    {
      RecalcParam ();
      m_minRtt = rtt;
    }

  uint32_t segCwnd = m_cWnd / m_segmentSize;

  NS_LOG_DEBUG ("NewAck, cwnd is " << segCwnd);

  if (m_cWnd.Get () < m_ssThresh.Get ())
    {
      /*
       * slow start
       * INC = 2^RHO - 1
       */
      isSlowstart = true;
      NS_ASSERT (m_rho > 0.0);
      increment = std::pow (2, m_rho) - 1;
      NS_LOG_DEBUG ("Slow start: inc=" << increment);
    }
  else
    {
      /*
       * congestion avoidance
       * INC = RHO^2 / W
       */
      NS_ASSERT (m_cWnd.Get () != 0);
      increment = std::pow (m_rho, 2) / ((double) segCwnd);
      NS_LOG_DEBUG ("Cong avoid: inc=" << increment);
    }

  NS_ASSERT (increment >= 0.0);

  uint32_t byte = increment*m_segmentSize;

  /* clamp down slowstart cwnd to ssthresh value. */
  if (isSlowstart)
    {
      m_cWnd = std::min (m_cWnd.Get () + byte, m_ssThresh.Get ());
      NS_LOG_DEBUG ("Slow start: new cwnd=" << m_cWnd/m_segmentSize << "ssth= " << m_ssThresh/m_segmentSize);
    }
  else
    {
      m_cWndCnt += increment;

      while (m_cWndCnt > 1)
        {
          m_cWnd += m_segmentSize;
          m_cWndCnt -= 1;
        }

      NS_LOG_DEBUG (Simulator::Now ().GetSeconds() << " Cong avoid: new cwnd=" << m_cWnd/m_segmentSize << "ssth= " << m_ssThresh/m_segmentSize);
      NS_LOG_DEBUG ("m_ss=" << m_segmentSize);
    }

  TcpSocketBase::NewAck (seq);
}

/* Cut cwnd and enter fast recovery mode upon triple dupack */
void
TcpHybla::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
      m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_recover = m_highTxMark;
      m_inFastRec = true;
      NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_cWnd <<
                   ", ssthresh to " << m_ssThresh << " at fast recovery seqnum " << m_recover);
      DoRetransmit ();
    }
  else if (m_inFastRec)
    { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      //m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Dupack in fast recovery mode. Increase cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    }
  else if (!m_inFastRec && m_limitedTx && m_txBuffer->SizeFromSequence (m_nextTxSequence) > 0)
    { // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
      NS_LOG_INFO ("Limited transmit");
      uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
      m_nextTxSequence += sz;                    // Advance next tx sequence
    };
}

Ptr<TcpSocketBase>
TcpHybla::Fork (void)
{
  return CopyObject<TcpHybla> (this);
}


} // namespace ns3
