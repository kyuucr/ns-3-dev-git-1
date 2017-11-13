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
#include "ap-wifi-mac-extension-ad.h"
#include "ap-wifi-mac-ad.h"

namespace ns3 {

ApWifiMacExtensionAd::ApWifiMacExtensionAd(const Ptr<WifiMac> &mac)
  : RegularWifiMacExtensionAd(mac)
{

}

Ptr<MultiBandElement>
ApWifiMacExtensionAd::GetMultiBandElement (void) const
{
  Ptr<MultiBandElement> multiband = Create<MultiBandElement> ();
  multiband->SetStaRole (ROLE_AP);
  multiband->SetStaMacAddressPresent (false); /* The same MAC address is used across all the bands */
  multiband->SetBandID (Band_2_4GHz);
  multiband->SetOperatingClass (18);          /* Europe */
  multiband->SetChannelNumber (DynamicCast<ApWifiMacAd> (m_mac)->m_phy->GetChannelNumber ());
  multiband->SetBssID (DynamicCast<ApWifiMacAd> (m_mac)->GetAddress ());
  multiband->SetBeaconInterval (DynamicCast<ApWifiMacAd> (m_mac)->m_beaconInterval.GetMicroSeconds ());
  multiband->SetConnectionCapability (1);     /* AP */
  multiband->SetFstSessionTimeout (m_fstTimeout);
  return multiband;
}

void
ApWifiMacExtensionAd::ExtendBeacon(MgtBeaconHeader *header)
{
  if (m_supportMultiBand)
    {
      header->AddWifiInformationElement (GetMultiBandElement ());
    }
}

} // namespace ns3
