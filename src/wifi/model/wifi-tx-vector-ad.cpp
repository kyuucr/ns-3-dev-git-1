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

#include "wifi-tx-vector-ad.h"

namespace ns3 {

WifiTxVectorAd::WifiTxVectorAd () :
  WifiTxVector (),
  m_traingFieldLength (0),
  m_beamTrackingRequest (false),
  m_lastRssi (0)
{

}

WifiTxVectorAd::WifiTxVectorAd (WifiMode mode,
                                uint8_t powerLevel,
                                uint8_t retries,
                                WifiPreamble preamble,
                                uint16_t guardInterval,
                                uint8_t nTx,
                                uint8_t nss,
                                uint8_t ness,
                                uint16_t channelWidth,
                                bool aggregation,
                                bool stbc) :
  WifiTxVector(mode, powerLevel, retries, preamble,
               guardInterval, nTx, nss, ness, channelWidth,
               aggregation, stbc),
  m_traingFieldLength (0),
  m_beamTrackingRequest (false),
  m_lastRssi (0)
{

}

WifiTxVectorAd::~WifiTxVectorAd ()
{

}

void
WifiTxVectorAd::SetPacketType (PacketType type)
{
  m_packetType = type;
}

PacketType
WifiTxVectorAd::GetPacketType (void) const
{
  return m_packetType;
}

void
WifiTxVectorAd::SetTrainngFieldLength (uint8_t length)
{
  m_traingFieldLength = length;
}

uint8_t
WifiTxVectorAd::GetTrainngFieldLength (void) const
{
  return m_traingFieldLength;
}

void
WifiTxVectorAd::RequestBeamTracking (void)
{
  m_beamTrackingRequest = true;
}

bool
WifiTxVectorAd::IsBeamTrackingRequested (void)
{
  return m_beamTrackingRequest;
}

void
WifiTxVectorAd::SetLastRssi (uint8_t level)
{
  m_lastRssi = level;
}

uint8_t
WifiTxVectorAd::GetLastRssi (void) const
{
  return m_lastRssi;
}

} // namespace ns3
