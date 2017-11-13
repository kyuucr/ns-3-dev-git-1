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
#include "wifi-mac-extension-ad.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMacExtensionAd");
NS_OBJECT_ENSURE_REGISTERED (WifiMacExtensionAd);

TypeId
WifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacExtensionAd")
    .SetParent<Object> ()
    .SetGroupName ("WifiAd")
  ;
  return tid;
}

WifiMacExtensionAd::WifiMacExtensionAd(const Ptr<WifiMac> &mac)
  : WifiMacExtensionInterface ()
{
  m_mac = mac;
}

/*
 * Physical Layer Characteristics for 802.11ad.
 */
void
WifiMacExtensionAd::Configure80211ad (void)
{
  m_mac->SetRifs (MicroSeconds (1));	/* aRIFSTime in Table 21-31 */
  m_mac->SetSifs (MicroSeconds (3));	/* aSIFSTime in Table 21-31 */
  m_mac->SetSlot (MicroSeconds (5));	/* aSlotTime in Table 21-31 */
  m_mac->SetMaxPropagationDelay (NanoSeconds (100));	/* aAirPropagationTime << 0.1usec in Table 21-31 */
  m_mac->SetPifs(m_mac->GetSifs() + m_mac->GetSlot());	/* 802.11-2007 9.2.10 */
  m_mac->SetEifsNoDifs (m_mac->GetSifs () + NanoSeconds (3062));
  /* ACK is sent using SC-MCS1, ACK/CTS Size = 14Bytes, PayloadDuration = 619ns, TotalTx = 3062*/
  m_mac->SetCtsTimeout (m_mac->GetSifs () + NanoSeconds (3062) + m_mac->GetSlot () + m_mac->GetMaxPropagationDelay () * 2);
  m_mac->SetAckTimeout (m_mac->GetSifs () + NanoSeconds (3062) + m_mac->GetSlot () + m_mac->GetMaxPropagationDelay () * 2);
  /* BlockAck is sent using SC-MCS1/4, we assume MCS1 in our calculation */
  /* BasicBlockAck Size = 152Bytes */
  m_mac->SetBasicBlockAckTimeout (m_mac->GetSifs () + m_mac->GetSlot () + NanoSeconds (4255) + m_mac->GetDefaultMaxPropagationDelay () * 2);
  /* CompressedBlockAck Size = 32Bytes */
  m_mac->SetCompressedBlockAckTimeout (m_mac->GetSifs () + m_mac->GetSlot () + NanoSeconds (3062) + m_mac->GetDefaultMaxPropagationDelay () * 2);
}

void
WifiMacExtensionAd::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);

  if (standard == WIFI_PHY_STANDARD_80211ad)
      {
        Configure80211ad ();
      }
}

} // namespace ns3
