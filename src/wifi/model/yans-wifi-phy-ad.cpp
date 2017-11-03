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
#include "yans-wifi-phy-ad.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiPhyAd");

NS_OBJECT_ENSURE_REGISTERED (YansWifiPhyAd);

TypeId
YansWifiPhyAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::YansWifiPhyAd")
    .SetParent<YansWifiPhy> ()
    .SetGroupName ("WifiAd")
    .AddConstructor<YansWifiPhyAd> ()
  ;
  return tid;
}

YansWifiPhyAd::YansWifiPhyAd () :
  YansWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

YansWifiPhyAd::~YansWifiPhyAd ()
{
  NS_LOG_FUNCTION (this);
}

void
YansWifiPhyAd::StartTrnTx (WifiTxVector txVector, uint8_t fieldsRemaining)
{
  NS_LOG_DEBUG ("Start TRN transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");
  m_channel->SendTrn (this, GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain (), txVector, fieldsRemaining);
}

} // namespace ns3
