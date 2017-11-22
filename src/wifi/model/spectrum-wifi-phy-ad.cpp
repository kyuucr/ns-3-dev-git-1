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

#include "spectrum-wifi-phy-ad.h"

namespace ns3 {

SpectrumWifiPhyAd::SpectrumWifiPhyAd()
  : SpectrumWifiPhy()
{

}

void
SpectrumWifiPhyAd::StartTrnTx (WifiTxVector txVector, uint8_t fieldsRemaining)
{
  NS_UNUSED(txVector);
  NS_UNUSED(fieldsRemaining);
}

double
SpectrumWifiPhyAd::GetBandBandwidth (void) const
{
  if (GetStandard() == WIFI_PHY_STANDARD_80211ad)
    // Use OFDM subcarrier width of 5.15625 MHz as band granularity
    return 5156250;
  else
    return SpectrumWifiPhy::GetBandBandwidth();
}

} // namespace ns3
