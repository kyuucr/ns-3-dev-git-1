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


#define BICTCP_BETA_SCALE    1024   /* Scale factor beta calculation
                                     * max_cwnd = cwnd * beta
                                     */
#define BICTCP_B        4    /*
                              * In binary search,
                              * go to point (max+min)/N
                              */

TypeId
TcpBic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpBic")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpBic> ()
    .AddAttribute ("FastConvergence", "Turn on/off fast convergence",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpBic::m_fastConvergence),
                   MakeBooleanChecker ())
    .AddAttribute ("Beta", "Beta for multiplicative increase",
                   IntegerValue (819),   /* = 819/1024 (BICTCP_BETA_SCALE) */
                   MakeIntegerAccessor (&TcpBic::m_beta),
                   MakeIntegerChecker <int> ())
    .AddAttribute ("MaxIncr", "Limit on increment allowed during binary search",
                   UintegerValue (16),
                   MakeUintegerAccessor (&TcpBic::m_maxIncr),
                   MakeUintegerChecker <uint32_t> ())
    .AddAttribute ("LowWnd", "Lower bound on congestion window (for TCP friendliness)",
                   UintegerValue (14),
                   MakeUintegerAccessor (&TcpBic::m_lowWnd),
                   MakeUintegerChecker <uint32_t> ())
    .AddAttribute ("SmoothPart", "log(B/(B*Smin))/log(B/(B-1))+B, # of RTT from Wmax-B to Wmax",
                   IntegerValue (20),
                   MakeIntegerAccessor (&TcpBic::m_smoothPart),
                   MakeIntegerChecker <int> ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpBic::m_cWnd),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource ("SlowStartThreshold",
                     "TCP slow start threshold (bytes)",
                     MakeTraceSourceAccessor (&TcpBic::m_ssThresh),
                     "ns3::TracedValue::Uint32Callback")
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

  m_cnt = 0;
  m_lastMaxCwnd = 0;
  m_epochStart = Time::Min ();
}

void
TcpBic::SetInitialSSThresh (uint32_t threshold)
{
  NS_LOG_FUNCTION (this);
  m_ssThresh = threshold;
}

uint32_t
TcpBic::GetInitialSSThresh (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ssThresh;
}

void
TcpBic::SetInitialCwnd (uint32_t cwnd)
{
  NS_LOG_FUNCTION (this);
  m_initialCwnd = cwnd;
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
  m_lossCwnd = 0;

  return TcpSocketBase::Listen ();
}

int
TcpBic::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);

  Init ();

  Reset ();
  m_lossCwnd = 0;

  return TcpSocketBase::Connect (address);
}

void
TcpBic::NewAck (SequenceNumber32 const& seq)
{
  NS_LOG_FUNCTION (this);
  TcpSocketBase::NewAck (seq);

  CongAvoid ();

  m_lossCwnd = 0;
}

void
TcpBic::CongAvoid (void)
{
  NS_LOG_FUNCTION (this);
  if (m_cWnd.Get () <= m_ssThresh)
    {
      m_cWnd += m_segmentSize;
    }
  else
    {
      Update ();

      /* Manage the increment in a proper way, avoiding floating point arithmetic */
      if (m_cWndCnt > m_cnt)
        {
          m_cWnd += m_segmentSize;
          m_cWndCnt = 0;
        }
      else
        {
          m_cWndCnt++;
        }
    }
}

void
TcpBic::Update ()
{
  NS_LOG_FUNCTION (this);

  m_lastCwnd = m_cWnd.Get ();
  m_lastTime = Simulator::Now ();

  if (m_epochStart == Time::Min ())
    {
      m_epochStart = Simulator::Now ();   /* record the beginning of an epoch */
    }

  if (m_cWnd.Get () < m_lowWnd)
    {
      m_cnt = m_cWnd.Get ();
      return;
    }

  if (m_cWnd.Get () < m_lastMaxCwnd)
    {
      uint32_t dist = (m_lastMaxCwnd - m_cWnd.Get ()) / BICTCP_B;

      if (dist > m_maxIncr)
        {
          /* Linear increase */
          m_cnt = m_cWnd.Get () / m_maxIncr;
        }
      else if (dist <= 1)
        {
          /* binary search increase */
          m_cnt = (m_cWnd.Get () * m_smoothPart) / BICTCP_B;
        }
      else
        {
          /* binary search increase */
          m_cnt = m_cWnd.Get () / dist;
        }
    }
  else
    {
      if (m_cWnd.Get () < m_lastMaxCwnd + BICTCP_B)
        {
          /* slow start AMD linear increase */
          m_cnt = (m_cWnd.Get () * m_smoothPart) / BICTCP_B;
        }
      else if (m_cWnd.Get () < m_lastMaxCwnd + m_maxIncr * (BICTCP_B - 1))
        {
          /* slow start */
          m_cnt = (m_cWnd.Get () * (BICTCP_B - 1)) / (m_cWnd.Get () - m_lastMaxCwnd);
        }
      else
        {
          /* linear increase */
          m_cnt = m_cWnd / m_maxIncr;
        }
    }

  /* if in slow start or link utilization is very low */
  if (m_lastMaxCwnd == 0)
    {
      if (m_cnt > 20) /* increase cwnd 5% per RTT */
        {
          m_cnt = 20;
        }
    }

  if (m_cnt == 0)
    {
      m_cnt = 1;
    }
}

void
TcpBic::RecalcSsthresh ()
{
  NS_LOG_FUNCTION (this);

  if (m_bicState == LOSS)
    {
      return;
    }

  //Reset ();

  m_epochStart = Time::Min ();    /* end of epoch */

  /* Wmax and fast convergence */
  if (m_cWnd.Get () < m_lastMaxCwnd && m_fastConvergence)
    {
      m_lastMaxCwnd = (m_cWnd.Get () * (BICTCP_BETA_SCALE + m_beta)) / (2 * BICTCP_BETA_SCALE);
    }
  else
    {
      m_lastMaxCwnd = m_cWnd.Get ();
    }

  m_lossCwnd = m_cWnd.Get ();

  if (m_cWnd.Get () < m_lowWnd)
    {
      m_ssThresh = std::max (m_cWnd.Get () >> 2, 2U * m_segmentSize);
    }
  else
    {
      m_ssThresh = std::max ((uint32_t)(m_cWnd.Get () * m_beta / BICTCP_BETA_SCALE), 2U * m_segmentSize);
    }

  m_cWnd = 1 * m_segmentSize;
}

void
TcpBic::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this);
  (void) t;

  /* After 3 DUPAcks, there is a Loss. */
  if (count == 3)
    {
      RecalcSsthresh ();
      m_bicState = LOSS;
      DoRetransmit ();
    }
  else if (count > 3)
    {
      // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      m_cWnd += m_segmentSize;
      //congAvoid();
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
  return CopyObject<TcpBic> (this);
}

uint32_t
TcpBic::Window (void)
{
  NS_LOG_FUNCTION (this);

  /* Limit the size of in-flight data by cwnd and receiver's rxwin */
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

} // namespace ns3
