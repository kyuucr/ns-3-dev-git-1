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
#include "wifi-mac-ad.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMacAd");

NS_OBJECT_ENSURE_REGISTERED (WifiMacAd);

TypeId
WifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacAd")
    .SetParent<WifiMac> ()
    .SetGroupName ("WifiAd")
  ;
  return tid;
}

WifiMacAd::WifiMacAd()
  : WifiMac ()
{

}

void
WifiMacAd::RegisterBandChangedCallback (BandChangedCallback callback)
{
  m_bandChangedCallback = callback;
}

WifiPhyStandard
WifiMacAd::GetCurrentstandard (void) const
{
  return m_standard;
}

/*
 * Physical Layer Characteristics for 802.11ad.
 */
void
WifiMacAd::Configure80211ad (void)
{
  SetRifs (MicroSeconds (1));	/* aRIFSTime in Table 21-31 */
  SetSifs (MicroSeconds (3));	/* aSIFSTime in Table 21-31 */
  SetSlot (MicroSeconds (5));	/* aSlotTime in Table 21-31 */
  SetMaxPropagationDelay (NanoSeconds (100));	/* aAirPropagationTime << 0.1usec in Table 21-31 */
  SetPifs(GetSifs() + GetSlot());	/* 802.11-2007 9.2.10 */
  SetEifsNoDifs (GetSifs () + NanoSeconds (3062));
  /* ACK is sent using SC-MCS1, ACK/CTS Size = 14Bytes, PayloadDuration = 619ns, TotalTx = 3062*/
  SetCtsTimeout (GetSifs () + NanoSeconds (3062) + GetSlot () + GetMaxPropagationDelay () * 2);
  SetAckTimeout (GetSifs () + NanoSeconds (3062) + GetSlot () + GetMaxPropagationDelay () * 2);
  /* BlockAck is sent using SC-MCS1/4, we assume MCS1 in our calculation */
  /* BasicBlockAck Size = 152Bytes */
  SetBasicBlockAckTimeout (GetSifs () + GetSlot () + NanoSeconds (4255) + GetDefaultMaxPropagationDelay () * 2);
  /* CompressedBlockAck Size = 32Bytes */
  SetCompressedBlockAckTimeout (GetSifs () + GetSlot () + NanoSeconds (3062) + GetDefaultMaxPropagationDelay () * 2);
}

void
WifiMacAd::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  m_standard = standard;

  if (standard == WIFI_PHY_STANDARD_80211ad)
      {
        Configure80211ad ();
      }
  else
      {
        WifiMac::ConfigureStandard(standard);
      }
}

} // namespace ns3
