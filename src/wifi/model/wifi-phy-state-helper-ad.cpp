/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 * ns-3 upstreaming process: Natale Patriciello <natale.patriciello@gmail.com>
 */
#include "wifi-phy-state-helper-ad.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhyStateHelperAd");

NS_OBJECT_ENSURE_REGISTERED (WifiPhyStateHelperAd);

TypeId
WifiPhyStateHelperAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyStateHelperAd")
    .SetParent<WifiPhyStateHelper> ()
    .SetGroupName ("WifiAd")
    .AddConstructor<WifiPhyStateHelperAd> ()
  ;
  return tid;
}

WifiPhyStateHelperAd::WifiPhyStateHelperAd()
{

}

void
WifiPhyStateHelperAd::ReportPsduEndOk (Ptr<Packet> packet, double snr, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << packet << snr << txVector);
  m_rxOkTrace (packet, snr, txVector.GetMode (), txVector.GetPreambleType ());
  if (!m_rxOkCallback.IsNull ())
    {
      m_rxOkCallback (packet, snr, txVector);
    }
}

void
WifiPhyStateHelperAd::SwitchFromRxEndOk (void)
{
  NS_LOG_FUNCTION (this);
  NotifyRxEndOk ();
  DoSwitchFromRx ();
}

void
WifiPhyStateHelperAd::ReportPsduEndError (Ptr<Packet> packet, double snr)
{
  NS_LOG_FUNCTION (this << packet << snr);
  m_rxErrorTrace (packet, snr);
  if (!m_rxErrorCallback.IsNull ())
    {
      m_rxErrorCallback (packet, snr);
    }
}

void
WifiPhyStateHelperAd::SwitchFromRxEndError (void)
{
  NS_LOG_FUNCTION (this);
  NotifyRxEndOk ();
  DoSwitchFromRx ();
}

} // namespace ns3
