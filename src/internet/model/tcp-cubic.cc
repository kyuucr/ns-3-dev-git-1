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

#define NS_LOG_APPEND_CONTEXT \
  { std::clog << Simulator::Now ().GetSeconds () << " "; }

#include "tcp-cubic.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TcpCubic");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpCubic);

TypeId
TcpCubic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpCubic")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpCubic> ()
    .AddAttribute ("FastConvergence", "Enable (true) or disable (false) fast convergence",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpCubic::m_fastConvergence),
                   MakeBooleanChecker ())
    .AddAttribute ("Beta", "Beta for multiplicative increase",
                   DoubleValue (0.8),
                   MakeDoubleAccessor (&TcpCubic::m_beta),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("HyStart", "Enable (true) or disable (false) hybrid slow start algorithm",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpCubic::m_hystart),
                   MakeBooleanChecker ())
    .AddAttribute ("HyStartLowWindow", "Lower bound cWnd for hybrid slow start (segments)",
                   UintegerValue (16),
                   MakeUintegerAccessor (&TcpCubic::m_hystartLowWindow),
                   MakeUintegerChecker <uint32_t> ())
    .AddAttribute ("HyStartDetect", "Hybrid Slow Start detection mechanisms:" \
                   "1: packet train, 2: delay, 3: both",
                   IntegerValue (3),
                   MakeIntegerAccessor (&TcpCubic::m_hystartDetect),
                   MakeIntegerChecker <int> ())
    .AddAttribute ("HyStartMinSamples", "Number of delay samples for detecting the increase of delay",
                   UintegerValue (8),
                   MakeUintegerAccessor (&TcpCubic::m_hystartMinSamples),
                   MakeUintegerChecker <uint8_t> ())
    .AddAttribute ("HyStartAckDelta", "Spacing between ack's indicating train",
                   TimeValue (MilliSeconds (2)),
                   MakeTimeAccessor (&TcpCubic::m_hystartAckDelta),
                   MakeTimeChecker ())
    .AddAttribute ("HyStartDelayMin", "Minimum time for hystart algorithm",
                   TimeValue (MilliSeconds (4)),
                   MakeTimeAccessor (&TcpCubic::m_hystartDelayMin),
                   MakeTimeChecker ())
    .AddAttribute ("HyStartDelayMax", "Maximum time for hystart algorithm",
                   TimeValue (MilliSeconds (1000)),
                   MakeTimeAccessor (&TcpCubic::m_hystartDelayMax),
                   MakeTimeChecker ())
    .AddAttribute ("CubicDelta", "Delta Time to wait after fast recovery before adjusting param",
                   TimeValue (MilliSeconds (10)),
                   MakeTimeAccessor (&TcpCubic::m_cubicDelta),
                   MakeTimeChecker ())
    .AddAttribute ("CwndAfterLoss", "Congestion window size after a loss (segments)",
                   UintegerValue (1),
                   MakeUintegerAccessor (&TcpCubic::m_cWndAfterLoss),
                   MakeUintegerChecker <uint32_t> ())
    .AddAttribute ("CntClamp", "Counter value when no losses are detected (counter is used" \
                   " when incrementing cWnd in congestion avoidance, to avoid" \
                   " floating point arithmetic). It is the modulo of the (avoided)" \
                   " division",
                   UintegerValue (20),
                   MakeUintegerAccessor (&TcpCubic::m_cntClamp),
                   MakeUintegerChecker <uint8_t> ())
    .AddAttribute ("C", "Cubic Scaling factor",
                   DoubleValue (0.4),
                   MakeDoubleAccessor (&TcpCubic::m_c),
                   MakeDoubleChecker <double> (0.0))
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpCubic::m_cWnd))
  ;
  return tid;
}

TcpCubic::TcpCubic () : TcpSocketBase ()
{
  m_cWndCnt = 0;
}

void
TcpCubic::Reset ()
{
  NS_LOG_FUNCTION (this);

  m_cnt = 0;
  m_lastMaxCwnd = 0;
  m_bicOriginPoint = 0;
  m_bicK = 0;
  m_delayMin = Time::Min ();
  m_epochStart = Time::Min ();
  m_found = 0;
}

void
TcpCubic::HystartReset ()
{
  NS_LOG_FUNCTION (this);

  m_roundStart = m_lastAck = Simulator::Now ();
  m_endSeq = m_nextTxSequence;
  m_currRtt = Time::Min ();
  m_sampleCnt = 0;
}

int
TcpCubic::Listen (void)
{
  NS_LOG_FUNCTION (this);

  Init ();

  Reset ();
  m_lossCwnd = 0;

  if (m_hystart)
    {
      HystartReset ();
    }

  return TcpSocketBase::Listen ();
}

void
TcpCubic::Init ()
{
  NS_LOG_FUNCTION (this);

  m_cWnd = m_initialCwnd * m_segmentSize;

  m_cubicState = OPEN;
}

int
TcpCubic::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);

  Init ();

  Reset ();
  m_lossCwnd = 0;

  if (m_hystart)
    {
      HystartReset ();
    }

  return TcpSocketBase::Connect (address);
}


void
TcpCubic::SetSSThresh (uint32_t threshold)
{
  NS_LOG_FUNCTION (this);
  m_ssThresh = threshold;
}

uint32_t
TcpCubic::GetSSThresh (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ssThresh;
}

void
TcpCubic::SetInitialCwnd (uint32_t cwnd)
{
  NS_LOG_FUNCTION (this);
  m_initialCwnd = cwnd;
}

uint32_t
TcpCubic::GetInitialCwnd (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialCwnd;
}


void
TcpCubic::NewAck (SequenceNumber32 const& seq)
{
  NS_LOG_FUNCTION (this);
  TcpSocketBase::NewAck (seq);

  m_cubicState = OPEN;
  PktsAcked ();
  CongAvoid (seq);

  m_lossCwnd = 0;
}

void
TcpCubic::WindowUpdate ()
{
  NS_LOG_FUNCTION (this);
  Time t;
  uint32_t delta, bicTarget;
  uint64_t offs;

  if (m_epochStart == Time::Min ())
    {
      m_epochStart = Simulator::Now ();   /* record the beginning of an epoch */

      if (m_lastMaxCwnd <= m_cWnd.Get ())
        {
          NS_LOG_DEBUG ("Last Max cWnd < m_cWnd. K=0 and origin=" << m_cWnd);
          m_bicK = 0;
          m_bicOriginPoint = m_cWnd.Get ();
        }
      else
        {
          m_bicK = std::pow (2.5 * (m_lastMaxCwnd - m_cWnd.Get ()), 1 / 3.);
          m_bicOriginPoint = m_lastMaxCwnd;
          NS_LOG_DEBUG ("Wow K=" << m_bicK << " and origin=" << m_lastMaxCwnd);
        }
    }

  t = Simulator::Now () + m_delayMin - m_epochStart;

  if (t.GetMilliSeconds () < m_bicK)       /* t - K */
    {
      offs = m_bicK - t.GetMilliSeconds ();
    }
  else
    {
      offs = t.GetMilliSeconds () - m_bicK;
    }

  /* Constant value taken from Experimental Evaluation of Cubic Tcp, available at
   * eprints.nuim.ie/1716/1/Hamiltonpfldnet2007_cubic_final.pdf */
  delta = m_c * std::pow (offs, 3);

  if (t.GetMilliSeconds () < m_bicK)                /* below origin*/
    {
      bicTarget = m_bicOriginPoint - delta;
    }
  else                                              /* above origin*/
    {
      bicTarget = m_bicOriginPoint + delta;
    }

  //bicTarget = std::min (bicTarget, m_rWnd.Get ());

  if (bicTarget > m_cWnd.Get ())
    {
      m_cnt = m_cWnd.Get () / (bicTarget - m_cWnd.Get ());
    }
  else
    {
      m_cnt = 100 * m_cWnd.Get ();     /* Very small increment */
    }

  if (m_delayMin.GetMilliSeconds () > 0)
    {
      m_cnt = std::max (m_cnt, (uint32_t) (8 * m_cWnd.Get () / (m_delayMin.GetMilliSeconds () * 20)));
    }

  if (m_lossCwnd == 0 && m_cnt > m_cntClamp)
    {
      m_cnt = m_cntClamp; /* When no losses are detected, grow up fast */
    }

  if (m_cnt == 0)
    {
      m_cnt = 1;
    }
}

void
TcpCubic::CongAvoid (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this);
  if (m_cWnd.Get () <= m_ssThresh)
    {
      if (m_hystart && seq > m_endSeq)
        {
          HystartReset ();
        }

      m_cWnd += m_segmentSize;
    }
  else
    {
      WindowUpdate ();

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
TcpCubic::PktsAcked ()
{
  NS_LOG_FUNCTION (this);

  /* Discard delay samples right after fast recovery */
  if (m_epochStart != Time::Min ()
      && (Simulator::Now () - m_epochStart) < m_cubicDelta)
    {
      return;
    }

  Time delay = m_lastRtt;

  if (delay.GetSeconds () == 0.0)
    {
      delay = Time (MilliSeconds (10));
    }

  /* first time call or link delay decreases */
  if (m_delayMin == Time::Min () || m_delayMin > delay)
    {
      m_delayMin = delay;
    }

  /* hystart triggers when cwnd is larger than some threshold */
  if (m_hystart                   &&
      m_cWnd.Get () <= m_ssThresh &&
      m_cWnd.Get () >= m_hystartLowWindow * m_segmentSize)
    {
      HystartUpdate (delay);
    }
}

void
TcpCubic::HystartUpdate (const Time& delay)
{
  NS_LOG_FUNCTION (this);

  if (!(m_found & m_hystartDetect))
    {
      Time now = Simulator::Now ();

      /* first detection parameter - ack-train detection */
      if ((now - m_lastAck) <= m_hystartAckDelta)
        {
          m_lastAck = now;

          if ((now - m_roundStart) > m_delayMin)
            {
              m_found |= PACKET_TRAIN;
            }
        }

      /* obtain the minimum delay of more than sampling packets */
      if (m_sampleCnt < m_hystartMinSamples)
        {
          if (m_currRtt == Time::Min () || m_currRtt > delay)
            {
              m_currRtt = delay;
            }

          ++m_sampleCnt;
        }
      else
        {
          if (m_currRtt > m_delayMin +
              HystartDelayThresh (m_delayMin))
            {
              m_found |= DELAY;
            }
        }
      /*
       * Either one of two conditions are met,
       * we exit from slow start immediately.
       */
      if (m_found & m_hystartDetect)
        {
          NS_LOG_DEBUG ("Exit from SS, immediately :-)");
          m_ssThresh = m_cWnd.Get ();
        }
    }
}

Time
TcpCubic::HystartDelayThresh (Time t) const
{
  Time ret = t;
  if (t > m_hystartDelayMax)
    {
      ret = m_hystartDelayMax;
    }
  else if (t < m_hystartDelayMin)
    {
      ret = m_hystartDelayMin;
    }

  return ret;
}

void
TcpCubic::RecalcSsthresh ()
{
  NS_LOG_FUNCTION (this);

  if (m_cubicState == LOSS)
    {
      return;
    }

  //Reset ();
  //HystartReset ();

  m_epochStart = Time::Min ();    /* end of epoch */

  /* Wmax and fast convergence */
  if (m_cWnd.Get () < m_lastMaxCwnd && m_fastConvergence)
    {
      m_lastMaxCwnd = 0.9 * m_cWnd.Get ();
    }
  else
    {
      m_lastMaxCwnd = m_cWnd.Get ();
    }

  m_lossCwnd = m_cWnd.Get ();

  /* Formula taken from the Linux kernel */
  m_ssThresh = std::max (static_cast<uint32_t> (m_cWnd.Get () * m_beta),
                         2U * m_segmentSize);

  /* Constant value taken from Experimental Evaluation of Cubic Tcp, available at
   * eprints.nuim.ie/1716/1/Hamiltonpfldnet2007_cubic_final.pdf */
  //m_cWnd = 0.8 * m_cWnd.Get ();

  // If m_cWndAfterLoss is 1, this is what happen in the Linux kernel
  // after ssthres is called
  m_cWnd = m_cWndAfterLoss * m_segmentSize;
}

void
TcpCubic::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this);
  (void) t;

  /* Keep the timing information */
  PktsAcked ();

  /* After 3 DUPAcks, there is a Loss. */
  if (count == 3)
    {
      RecalcSsthresh ();
      m_cubicState = LOSS;
      DoRetransmit ();
    }
  else if (count > 3)
    {
      // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      m_cWnd += m_segmentSize;
      SendPendingData (m_connected);
    }
}

void
TcpCubic::Retransmit (void)
{
  NS_LOG_FUNCTION (this);

  RecalcSsthresh ();

  m_cubicState = LOSS;

  TcpSocketBase::Retransmit ();
}

Ptr<TcpSocketBase>
TcpCubic::Fork (void)
{
  return CopyObject<TcpCubic> (this);
}

uint32_t
TcpCubic::Window (void)
{
  NS_LOG_FUNCTION (this);

  /* Limit the size of in-flight data by cwnd and receiver's rxwin */
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

}
