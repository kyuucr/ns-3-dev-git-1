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
#include "tcp-bic.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TcpBic");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpBic);

TypeId
TcpBic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpBic")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpBic> ()
    .SetGroupName ("Internet")
    .AddAttribute ("FastConvergence", "Turn on/off fast convergence.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpBic::m_fastConvergence),
                   MakeBooleanChecker ())
    .AddAttribute ("Beta", "Beta for multiplicative decrease",
                   DoubleValue (0.8),
                   MakeDoubleAccessor (&TcpBic::m_beta),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("MaxIncr", "Limit on increment allowed during binary search",
                   UintegerValue (16),
                   MakeUintegerAccessor (&TcpBic::m_maxIncr),
                   MakeUintegerChecker <uint32_t> (1))
    .AddAttribute ("LowWnd", "Threshold window size (in segments) for engaging BIC response",
                   UintegerValue (14),
                   MakeUintegerAccessor (&TcpBic::m_lowWnd),
                   MakeUintegerChecker <uint32_t> ())
    .AddAttribute ("SmoothPart", "Number of RTT needed to approach cWnd_max from "
                   "cWnd_max-BinarySearchCoefficient. It can be viewed as the gradient "
                   "of the slow start AIM phase: less this value is, "
                   "more steep the increment will be.",
                   IntegerValue (5),
                   MakeIntegerAccessor (&TcpBic::m_smoothPart),
                   MakeIntegerChecker <int> (1))
    .AddAttribute ("BinarySearchCoefficient", "Inverse of the coefficient for the "
                   "binary search. Default 4, as in Linux",
                   UintegerValue (4),
                   MakeUintegerAccessor (&TcpBic::m_b),
                   MakeUintegerChecker <uint8_t> (2))
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                   UintegerValue (3),
                   MakeUintegerAccessor (&TcpBic::m_retxThresh),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpBic::m_cWnd),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource ("SlowStartThreshold",
                     "TCP slow start threshold (bytes)",
                     MakeTraceSourceAccessor (&TcpBic::m_ssThresh),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource ("BicState",
                     "State of TCP BIC",
                     MakeTraceSourceAccessor (&TcpBic::m_bicState),
                     "ns3::TracedValue::Uint8Callback")
  ;
  return tid;
}


TcpBic::TcpBic () : TcpSocketBase ()
{
  m_cWndCnt = 0;
}

void
TcpBic::Reset ()
{
  NS_LOG_FUNCTION (this);

  m_lastMaxCwnd = 0;
  m_epochStart = Time::Min ();
}

void
TcpBic::SetInitialSSThresh (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ssThresh = threshold;
}

uint32_t
TcpBic::GetInitialSSThresh (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ssThresh;
}

void
TcpBic::SetInitialCwnd (uint32_t cWnd)
{
  NS_LOG_FUNCTION (this << cWnd);
  m_initialCwnd = cWnd;
}

uint32_t
TcpBic::GetInitialCwnd (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialCwnd;
}

void
TcpBic::Init ()
{
  NS_LOG_FUNCTION (this);

  m_cWnd = m_initialCwnd * m_segmentSize;
  m_bicState = OPEN;
}

int
TcpBic::Listen (void)
{
  NS_LOG_FUNCTION (this);

  Init ();

  Reset ();

  return TcpSocketBase::Listen ();
}

int
TcpBic::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);

  Init ();

  Reset ();

  return TcpSocketBase::Connect (address);
}

void
TcpBic::NewAck (SequenceNumber32 const& seq)
{
  NS_LOG_FUNCTION (this << seq);

  m_bicState = OPEN;
  CongAvoid ();

  TcpSocketBase::NewAck (seq);
}

void
TcpBic::CongAvoid (void)
{
  NS_LOG_FUNCTION (this);

  if (m_cWnd < m_ssThresh)
    {
      NS_LOG_DEBUG ("SlowStart: Increase cwnd by one segment size");
      m_cWnd += m_segmentSize;
    }
  else
    {
      ++m_cWndCnt;
      uint32_t cnt = Update ();

      /* According to the BIC paper and RFC 6356 even once the new cwnd is
       * calculated you must compare this to the number of ACKs received since
       * the last cwnd update. If not enough ACKs have been received then cwnd
       * cannot be updated.
       */
      if (m_cWndCnt > cnt)
        {
          m_cWnd += m_segmentSize;
          m_cWndCnt = 0;
          NS_LOG_DEBUG ("Increment cwnd to " << m_cWnd / m_segmentSize);
        }
      else
        {
          NS_LOG_DEBUG ("Not enough segments have been ACKed to increment cwnd. Until now " << m_cWndCnt);
        }
    }
}

uint32_t
TcpBic::Update ()
{
  NS_LOG_FUNCTION (this);

  uint32_t segCwnd = m_cWnd / m_segmentSize;
  uint32_t cnt;

  m_lastCwnd = segCwnd;

  NS_LOG_DEBUG ("New ack. cWnd=" << segCwnd);

  if (m_epochStart == Time::Min ())
    {
      m_epochStart = Simulator::Now ();   /* record the beginning of an epoch */
    }

  if (segCwnd < m_lowWnd)
    {
      cnt = segCwnd;
      NS_LOG_DEBUG ("cWnd less than lowWnd (" << m_lowWnd << ")");
      return cnt;
    }

  if (segCwnd < m_lastMaxCwnd)
    {
      double dist = (m_lastMaxCwnd - segCwnd) / m_b;

      NS_LOG_DEBUG ("Under lastMax. lastMaxCwnd=" << m_lastMaxCwnd << " and dist=" << dist);
      if (dist > m_maxIncr)
        {
          /* Linear increase */
          cnt = segCwnd / m_maxIncr;
          NS_LOG_DEBUG ("Linear increase (maxIncr=" << m_maxIncr << "), cnt=" << cnt);
        }
      else if (dist <= 1)
        {
          /* smoothed binary search increase: when our window is really
           * close to the last maximum, we parametrize in m_smoothPart the number
           * of RTT needed to reach that window.
           */
          cnt = (segCwnd * m_smoothPart) / m_b;

          NS_LOG_DEBUG ("Binary search increase (smoothParth=" << m_smoothPart << "), cnt=" << cnt);
        }
      else
        {
          /* binary search increase */
          cnt = segCwnd / dist;

          NS_LOG_DEBUG ("Binary search increase, cnt=" << cnt);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Above last max. lastMaxCwnd=" << m_lastMaxCwnd);
      if (segCwnd < m_lastMaxCwnd + m_b)
        {
          /* slow start AMD linear increase */
          cnt = (segCwnd * m_smoothPart) / m_b;
          NS_LOG_DEBUG ("Slow start AMD, cnt=" << cnt);
        }
      else if (segCwnd < m_lastMaxCwnd + m_maxIncr * (m_b - 1))
        {
          /* slow start */
          cnt = (segCwnd * (m_b - 1)) / (segCwnd - m_lastMaxCwnd);

          NS_LOG_DEBUG ("Slow start, cnt=" << cnt);
        }
      else
        {
          /* linear increase */
          cnt = segCwnd / m_maxIncr;

          NS_LOG_DEBUG ("Linear, cnt=" << cnt);
        }
    }

  /* if in slow start or link utilization is very low. Code taken from Linux
   * kernel, not sure of the source they take it. Usually, it is not reached,
   * since if m_lastMaxCwnd is 0, we are (hopefully) in slow start.
   */
  if (m_lastMaxCwnd == 0)
    {
      if (cnt > 20) /* increase cwnd 5% per RTT */
        {
          cnt = 20;
        }
    }

  if (cnt == 0)
    {
      cnt = 1;
    }

  return cnt;
}

void
TcpBic::RecalcSsthresh ()
{
  NS_LOG_FUNCTION (this);

  if (m_bicState == LOSS)
    {
      NS_LOG_DEBUG ("Already in LOSS state. No cut on window");
      return;
    }

  uint32_t segCwnd = m_cWnd / m_segmentSize;

  NS_LOG_DEBUG ("Loss at cWnd=" << segCwnd);

  Reset ();

  m_epochStart = Time::Min ();    /* end of epoch */

  /* Wmax and fast convergence */
  if (segCwnd < m_lastMaxCwnd && m_fastConvergence)
    {
      m_lastMaxCwnd = m_beta * segCwnd;
      NS_LOG_DEBUG ("Fast Convergence. Last max cwnd: " << m_lastMaxCwnd);
    }
  else
    {
      m_lastMaxCwnd = segCwnd;
      NS_LOG_DEBUG ("Last max cwnd: " << m_lastMaxCwnd);
    }

  if (segCwnd < m_lowWnd)
    {
      m_ssThresh = std::max ((uint32_t) (m_segmentSize * (segCwnd >> 2)), 2U * m_segmentSize);
      NS_LOG_DEBUG ("Less than lowWindow, ssTh and cWnd= " << m_ssThresh / m_segmentSize);
    }
  else
    {
      m_ssThresh = std::max ((uint32_t) (m_segmentSize * (segCwnd * m_beta)), 2U * m_segmentSize);
      NS_LOG_DEBUG ("More than lowWindow, ssTh and cWnd= " << m_ssThresh / m_segmentSize);
    }

  m_cWnd = m_ssThresh;
}

void
TcpBic::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << t << count);
  (void) t;

  /* After 3 DUPAcks, there is a Loss. */
  if (count == m_retxThresh)
    {
      RecalcSsthresh ();
      m_bicState = LOSS;
      DoRetransmit ();
    }
  else if (count > m_retxThresh)
    {
      CongAvoid ();
      SendPendingData (m_connected);
    }
}

void
TcpBic::Retransmit (void)
{
  NS_LOG_FUNCTION (this);

  RecalcSsthresh ();

  m_bicState = LOSS;

  TcpSocketBase::Retransmit ();
}

Ptr<TcpSocketBase>
TcpBic::Fork (void)
{
  NS_LOG_FUNCTION (this);
  return CopyObject<TcpBic> (this);
}

uint32_t
TcpBic::Window (void)
{
  NS_LOG_FUNCTION (this);

  /* Limit the size of in-flight data by cwnd and receiver's rxwin */
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

void
TcpBic::ScaleSsThresh (uint8_t scaleFactor)
{
  NS_LOG_FUNCTION (this << static_cast<int> (scaleFactor));
  m_ssThresh <<= scaleFactor;
}

} // namespace ns3
