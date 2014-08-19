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
    //.AddAttribute ("LimitedTransmit", "Enable limited transmit",
    //                BooleanValue (false),
    //                MakeBooleanAccessor (&TcpHybla::m_limitedTx),
    //                MakeBooleanChecker ())
  ;
  return tid;
}

TcpHybla::TcpHybla () : TcpNewReno ()
{
  m_minRtt = Time::Max ();

  m_rho = 1.0;
}

void
TcpHybla::InitializeCwnd ()
{
  NS_LOG_FUNCTION (this);

  /* 1st Rho measurement based on initial srtt */
  RecalcParam ();

  /* set minimum rtt as this is the 1st ever seen */
  m_minRtt = m_rtt->GetCurrentEstimate ();

  TcpNewReno::InitializeCwnd ();

}

void
TcpHybla::RecalcParam ()
{
  Time rtt = m_lastRtt.Get ();

  if (rtt.IsZero ())
    {
      return;
    }

  m_rho = std::max ((double)rtt.GetMilliSeconds () / m_rRtt.GetMilliSeconds (), 1.0);
}

void
TcpHybla::NewAck (const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this);

  double increment;
  bool isSlowstart = false;

  Time rtt = m_lastRtt.Get ();
  if (rtt.IsZero ())
    {
      rtt = m_rtt->GetCurrentEstimate ();
    }

  /*  Recalculate rho only if this srtt is the lowest */
  if (rtt < m_minRtt)
    {
      RecalcParam ();
      m_minRtt = rtt;
    }

  if (m_cWnd.Get () < m_ssThresh.Get ())
    {
      /*
       * slow start
       *      INC = 2^RHO - 1
       */
      isSlowstart = true;
      NS_ASSERT (m_rho > 0.0);
      increment = std::pow (2, m_rho) - 1;
    }
  else
    {
      /*
       * congestion avoidance
       * INC = RHO^2 / W
       */
      NS_ASSERT (m_cWnd.Get () != 0);
      increment = std::pow (m_rho, 2) / ((double) m_cWnd.Get () / m_segmentSize);
    }

  NS_ASSERT (increment >= 0.0);

  m_cWnd += m_segmentSize * increment;

  /* clamp down slowstart cwnd to ssthresh value. */
  if (isSlowstart)
    {
      m_cWnd = std::min (m_cWnd.Get (), m_ssThresh.Get ());
    }

  TcpSocketBase::NewAck (seq);
}

Ptr<TcpSocketBase>
TcpHybla::Fork (void)
{
  return CopyObject<TcpHybla> (this);
}


} // namespace ns3
