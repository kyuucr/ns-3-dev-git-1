/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 ResiliNets, ITTC, University of Kansas
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
 * Authors: Amir Modarresi <amodarresi@ittc.ku.edu>

 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 */

#include "ns3/test.h"
#include "ns3/log.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-htcp.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpHtcpTestSuite");

/**
 * \brief Testing the congestion avoidance increment on TcpHtcp
 */
class TcpHtcpIncrementTest : public TestCase
{
public:
  TcpHtcpIncrementTest (uint32_t cWnd, uint32_t segmentSize,
                     uint32_t segmentsAcked, Time lastCongestion, Time firstAck,
                     Time secondAck, uint32_t expectedCwnd, const std::string &name);

private:
  virtual void DoRun (void);

  uint32_t m_cWnd;
  uint32_t m_segmentSize;
  uint32_t m_segmentsAcked;
  Time m_lastCongestion;
  Time m_firstAck;
  Time m_secondAck;
  uint32_t m_expectedCwnd;
  Ptr<TcpSocketState> m_state;
};

TcpHtcpIncrementTest::TcpHtcpIncrementTest (uint32_t cWnd, uint32_t segmentSize,
                                      uint32_t segmentsAcked, Time lastCongestion, Time firstAck,
                                      Time secondAck, uint32_t expectedCwnd, const std::string &name)
  : TestCase (name),
    m_cWnd (cWnd),
    m_segmentSize (segmentSize),
    m_segmentsAcked (segmentsAcked),
    m_lastCongestion (lastCongestion),
    m_firstAck (firstAck),
    m_secondAck (secondAck),
    m_expectedCwnd (expectedCwnd)
{
}

/**
 * \brief Since the calculation depends on the throughput and its associated
 * timing, we schedule a few exact events. We get the value from HTCP methods
 * during the simulation and compare them with their associated expected
 * values calculated from the algorithm by hand.
 */
void
TcpHtcpIncrementTest::DoRun ()
{
  NS_LOG_FUNCTION (this);
  m_state = CreateObject<TcpSocketState> ();
  static TracedValue<uint32_t> localCwnd = m_cWnd;
  StateTracedValues tracedValues;
  tracedValues.m_cWnd = &localCwnd;

  m_state->m_segmentSize = m_segmentSize;
  m_state->SetTracedValues (tracedValues);

  Ptr<TcpHtcp> cong = CreateObject<TcpHtcp> ();
  Time lastCongestion;

  NS_LOG_DEBUG ("m_cWnd: " << m_cWnd << " m_segmentSize: " << m_segmentSize <<
                " m_segmentsAcked: " << m_segmentsAcked << " m_lastCongestion" << m_lastCongestion);
  Simulator::Schedule (Time (m_lastCongestion), &TcpHtcp::GetSsThresh, cong,
                       m_state, m_state->GetCwnd ());
  lastCongestion = m_lastCongestion;
  Simulator::Schedule (Time (m_firstAck), &TcpHtcp::PktsAcked, cong, m_state,
                       m_segmentsAcked, Time (ns3::MilliSeconds (80)));
  Simulator::Schedule (Time (m_secondAck), &TcpHtcp::PktsAcked, cong, m_state,
                       m_segmentsAcked, Time (ns3::MilliSeconds (100)));

  Simulator::Run ();
  NS_LOG_DEBUG ("Simulation ran for the scheduled events");

  cong->IncreaseWindow (m_state, m_segmentsAcked);
  NS_LOG_DEBUG ( "m_cwnd from function: " << m_state->GetCwnd () << " expected cWnd calculated: " << m_expectedCwnd);

  NS_TEST_ASSERT_MSG_EQ (m_state->GetCwnd (), m_expectedCwnd,
                         "CWnd has not updated correctly");

  Simulator::Destroy ();
}

/* The following tests simulate conditions after a congestion event and 
 * return to 1/2 ssthresh. After that, two acks are scheduled and the 
 * value of the cWnd is compared at the end of the event.
 * The values in each test have been chosen randomly. The first test 
 * simulates receiving acks for 38 packets with segmentSize=536, 
 * the second one receives ack for 100 packets with segmentSize=1 and
 * the third one receives ack for 50 segment with segmentSize=1446. 
 * The resulting cWnd values of 20383, 40 and 76671 have been
 * calculated by hand from the algorithm for their expected values.
 */

static class TcpHtcpTestSuite : public TestSuite
{
public:
  TcpHtcpTestSuite ()
    : TestSuite ("tcp-htcp-test", UNIT)
  {

    AddTestCase (
      new TcpHtcpIncrementTest (38 * 536, 536, 38, ns3::MilliSeconds (1),
                             ns3::MilliSeconds (900), ns3::MilliSeconds (1000),
                             20383,"TcpHtcp increment test on cWnd "), TestCase::QUICK);
    AddTestCase (
      new TcpHtcpIncrementTest (38, 1, 100, ns3::MilliSeconds (1),
                             ns3::MilliSeconds (900), ns3::MilliSeconds (1100),
                             40,"TcpHtcp increment test on cWnd "), TestCase::QUICK);
    AddTestCase (
      new TcpHtcpIncrementTest (53 * 1446, 1446, 50, ns3::MilliSeconds (1),
                             ns3::MilliSeconds (900), ns3::MilliSeconds (1500),
                             76671,"TcpHtcp increment test on cWnd "), TestCase::QUICK);

  }
} g_TcpHtcpTest;

} // namespace ns3
