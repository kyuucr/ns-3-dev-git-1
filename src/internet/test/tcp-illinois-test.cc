/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
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
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#include "ns3/test.h"
#include "ns3/log.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-illinois.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpIllinoisTestSuite");

/**
 * \brief Testing TcpIllinois C-AIMD algorithm
 */
class TcpIllinoisTest : public TestCase
{
public:
  TcpIllinoisTest (uint32_t cWnd,
                   uint32_t ssThresh,
                   uint32_t segmentSize,
                   uint32_t cntRtt,
                   Time maxRtt,
                   uint32_t segmentsAcked,
                   SequenceNumber32 nextTxSeq,
                   SequenceNumber32 lastAckedSeq,
                   const std::string &name);

private:
  virtual void DoRun ();
  void IncreaseWindow (Ptr<TcpIllinois> cong);
  void RecalcParam (Ptr<TcpIllinois> cong);
  Time CalculateMaxDelay ();
  Time CalculateAvgDelay ();
  void CalculateAlpha (Ptr<TcpIllinois> cong, double da, double dm);
  void CalculateBeta (Ptr<TcpIllinois> cong, double da, double dm);
  void GetSsThresh ();

  uint32_t m_cWnd;
  uint32_t m_ssThresh;
  uint32_t m_segmentSize;
  Time m_baseRtt;
  Time m_maxRtt;
  uint32_t m_segmentsAcked;
  SequenceNumber32 m_nextTxSeq;
  SequenceNumber32 m_lastAckedSeq;
  double m_alpha;
  double m_beta;
  uint32_t m_cntRtt;
  Time m_sumRtt;
  bool m_rttAbove;
  uint8_t m_rttLow;
  uint32_t m_ackCnt;
};

TcpIllinoisTest::TcpIllinoisTest (uint32_t cWnd,
                                  uint32_t ssThresh,
                                  uint32_t segmentSize,
                                  uint32_t cntRtt,
                                  Time maxRtt,
                                  uint32_t segmentsAcked,
                                  SequenceNumber32 nextTxSeq,
                                  SequenceNumber32 lastAckedSeq,
                                  const std::string & name)
  : TestCase (name),
    m_cWnd (cWnd),
    m_ssThresh (ssThresh),
    m_segmentSize (segmentSize),
    m_baseRtt (MilliSeconds (100)),
    m_maxRtt (maxRtt),
    m_segmentsAcked (segmentsAcked),
    m_nextTxSeq (nextTxSeq),
    m_lastAckedSeq (lastAckedSeq),
    m_alpha (0.0),
    m_beta (0.0),
    m_cntRtt (cntRtt),
    m_sumRtt (0),
    m_rttAbove (false),
    m_rttLow (0),
    m_ackCnt (0)
{
}

void
TcpIllinoisTest::DoRun ()
{
  TracedValue<uint32_t> localCwnd = m_cWnd;
  TracedValue<uint32_t> localSsThresh = m_ssThresh;
  TracedValue<SequenceNumber32> localNextTx = m_nextTxSeq;
  StateTracedValues tracedValues;
  tracedValues.m_cWnd = &localCwnd;
  tracedValues.m_nextTxSequence = &localNextTx;
  tracedValues.m_ssThresh = &localSsThresh;

  Ptr<TcpSocketState> state = CreateObject <TcpSocketState> ();
  state->SetTracedValues (tracedValues);
  state->m_segmentSize = m_segmentSize;
  state->m_lastAckedSeq = m_lastAckedSeq;

  Ptr<TcpIllinois> cong = CreateObject <TcpIllinois> ();

  // Set baseRtt to 100 ms
  cong->PktsAcked (state, m_segmentsAcked, m_baseRtt);

  m_sumRtt += m_baseRtt;

  // Set maxRtt and update sumRtt based on cntRtt value
  for (uint32_t count = 1; count < m_cntRtt; ++count)
    {
      cong->PktsAcked (state, m_segmentsAcked, m_maxRtt);
      m_sumRtt += m_maxRtt;
    }

  /*
   * Test cWnd modification during additive increase
   */
  cong->IncreaseWindow (state, m_segmentsAcked);
  IncreaseWindow (cong);
  NS_TEST_ASSERT_MSG_EQ (state->GetCwnd (), m_cWnd,
                         "CWnd has not updated correctly");

  /*
   * Test ssThresh modification during multiplicative decrease
   */
  uint32_t ssThresh = cong->GetSsThresh (state, m_cWnd);
  GetSsThresh ();
  NS_TEST_ASSERT_MSG_EQ (ssThresh, m_ssThresh,
                         "SsThresh has not updated correctly");


}

void
TcpIllinoisTest::IncreaseWindow (Ptr<TcpIllinois> cong)
{
  uint32_t segCwnd = m_cWnd / m_segmentSize;

  if (m_lastAckedSeq >= m_nextTxSeq)
    {
      RecalcParam (cong);
    }
  if (m_cWnd < m_ssThresh)
    { // NewReno slow start
      if (m_segmentsAcked >= 1)
        {
          m_cWnd += m_segmentSize;
          m_segmentsAcked -= 1;
        }
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_cWnd <<
                   " ssthresh " << m_ssThresh);
    }
  else
    {
      uint32_t oldCwnd = segCwnd;

      if (m_segmentsAcked > 0)
        {
          m_ackCnt += m_segmentsAcked * m_alpha;
        }

      while (m_ackCnt >= segCwnd)
        {
          m_ackCnt -= segCwnd;
          segCwnd += 1;
        }

      if (segCwnd != oldCwnd)
        {
          m_cWnd = segCwnd * m_segmentSize;
          NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd <<
                       " ssthresh " << m_ssThresh);
        }
    }

}

void
TcpIllinoisTest::RecalcParam (Ptr<TcpIllinois> cong)
{
  DoubleValue alphaBase;
  cong->GetAttribute ("AlphaBase", alphaBase);
  UintegerValue winThresh;
  cong->GetAttribute ("WinThresh", winThresh);

  if (m_cWnd < winThresh.Get ())
    {
      NS_LOG_INFO ("cWnd < winThresh, set alpha & beta to base values");
      m_alpha = alphaBase.Get ();
    }
  else if (m_cntRtt > 0)
    {
      double dm = (double) CalculateMaxDelay ().GetMilliSeconds ();
      double da = (double) CalculateAvgDelay ().GetMilliSeconds ();

      NS_LOG_INFO ("Updated to dm = " << dm << " da = " << da);

      CalculateAlpha (cong, da, dm);
      CalculateBeta (cong, da, dm);
    }
}

Time
TcpIllinoisTest::CalculateMaxDelay ()
{
  return (m_maxRtt - m_baseRtt);
}

Time
TcpIllinoisTest::CalculateAvgDelay ()
{
  return (m_sumRtt / m_cntRtt - m_baseRtt);
}

void
TcpIllinoisTest::CalculateAlpha (Ptr<TcpIllinois> cong, double da, double dm)
{
  DoubleValue alphaMax;
  cong->GetAttribute ("AlphaMax", alphaMax);
  UintegerValue theta;
  cong->GetAttribute ("Theta", theta);
  DoubleValue alphaMin;
  cong->GetAttribute ("AlphaMin", alphaMin);

  double d1 = dm / 100;

  if (da <= d1)
    {
      if (!m_rttAbove)
        {
          m_alpha = alphaMax.Get ();
        }
      if (++m_rttLow >= theta.Get ())
        {
          m_rttLow = 0;
          m_rttAbove = false;
          m_alpha = alphaMax.Get ();
        }
    }
  else
    {
      m_rttAbove = true;
      dm -= d1;
      da -= d1;
      m_alpha = (dm * alphaMax.Get ()) / (dm + (da * (alphaMax.Get () - alphaMin.Get ())) / alphaMin.Get ());
    }
  NS_LOG_INFO ("Updated to alpha = " << m_alpha);
}

void
TcpIllinoisTest::CalculateBeta (Ptr<TcpIllinois> cong, double da, double dm)
{
  DoubleValue betaMin;
  cong->GetAttribute ("BetaMin", betaMin);
  DoubleValue betaMax;
  cong->GetAttribute ("BetaMax", betaMax);

  double d2, d3;
  d2 = dm / 10;
  d3 = (8 * dm) / 10;

  if (da <= d2)
    {
      m_beta = betaMin.Get ();
    }
  else if (da > d2 && da < d3)
    {
      m_beta = (betaMin.Get () * d3 - betaMax.Get () * d2 + (betaMax.Get () - betaMin.Get ()) * da) / (d3 - d2);
    }

  else if (da >= d3 || d3 <= d2)
    {
      m_beta = betaMax.Get ();
    }
  NS_LOG_INFO ("Updated to beta = " << m_beta);
}

void
TcpIllinoisTest::GetSsThresh ()
{
  uint32_t segCwnd = m_cWnd / m_segmentSize;
  uint32_t ssThresh = std::max (2.0, (1.0 - m_beta) * segCwnd);

  NS_LOG_DEBUG ("Calculated ssThresh (in segments) = " << ssThresh);

  m_ssThresh = ssThresh * m_segmentSize;
}

// -------------------------------------------------------------------
static class TcpIllinoisTestSuite : public TestSuite
{
public:
  TcpIllinoisTestSuite () : TestSuite ("tcp-illinois-test", UNIT)
  {
    AddTestCase (new TcpIllinoisTest (38 * 1446, 40 * 1446, 1446, 2, MilliSeconds (105), 2, SequenceNumber32 (2893), SequenceNumber32 (5785),
                                      "Illinois test on cWnd and ssThresh when in slow start"),
                 TestCase::QUICK);
    AddTestCase (new TcpIllinoisTest (60 * 346, 40 * 346, 346, 2, MilliSeconds (100), 2, SequenceNumber32 (2893), SequenceNumber32 (5785),
                                      "Illinois test on cWnd and ssThresh when avg queueing delay is at minimum"),
                 TestCase::QUICK);
    AddTestCase (new TcpIllinoisTest (38 * 1446, 40 * 1446, 1446, 5, MilliSeconds (110), 2, SequenceNumber32 (2893), SequenceNumber32 (5785),
                                      "Illinois test on cWnd and ssThresh when avg queueing delay is at maximum"),
                 TestCase::QUICK);
    AddTestCase (new TcpIllinoisTest (40 * 1446, 38 * 1446, 1446, 2, MilliSeconds (105), 55, SequenceNumber32 (2893), SequenceNumber32 (5785),
                                      "Illinois test on cWnd and ssThresh when avg queueing delay is in between its min & max"),
                 TestCase::QUICK);
  }
} g_tcpIllinoisTest;

} // namespace ns3
