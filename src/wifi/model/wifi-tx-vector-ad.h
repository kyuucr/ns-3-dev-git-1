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
#pragma once

#include "wifi/model/wifi-tx-vector.h"
#include "wifi/model/wifi-phy-standard-ad.h"

namespace ns3 {

class WifiTxVectorAd : public WifiTxVector
{
public:
  WifiTxVectorAd();
  /**
   * Create a TXVECTOR with the given parameters.
   *
   * \param mode WifiMode
   * \param powerLevel transmission power level
   * \param retries retries
   * \param preamble preamble type
   * \param guardInterval the guard interval duration in nanoseconds
   * \param nTx the number of TX antennas
   * \param nss the number of spatial STBC streams (NSS)
   * \param ness the number of extension spatial streams (NESS)
   * \param channelWidth the channel width in MHz
   * \param aggregation enable or disable MPDU aggregation
   * \param stbc enable or disable STBC
   */
  WifiTxVectorAd (WifiMode mode,
                uint8_t powerLevel,
                uint8_t retries,
                WifiPreamble preamble,
                uint16_t guardInterval,
                uint8_t nTx,
                uint8_t nss,
                uint8_t ness,
                uint16_t channelWidth,
                bool aggregation,
                bool stbc);
  ~WifiTxVectorAd();

  /**
   * Set BRP Packet Type.
   * \param type The type of BRP packet.
   */
  void SetPacketType (PacketType type);
  /**
   * Get BRP Packet Type.
   * \return The type of BRP packet.
   */
  PacketType GetPacketType (void) const;
  /**
   * Set the length of te training field.
   * \param length The length of the training field.
   */
  void SetTrainngFieldLength (uint8_t length);
  /**
   * Get the length of te training field.
   * \return The length of te training field.
   */
  uint8_t GetTrainngFieldLength (void) const;
  /**
   * Request Beam Tracking.
   */
  void RequestBeamTracking (void);
  /**
   * \return True if Beam Tracking requested, otherwise false.
   */
  bool IsBeamTrackingRequested (void);
  /**
   * In the TXVECTOR, LAST_RSSI indicates the received power level of
   * the last packet with a valid PHY header that was received a SIFS period
   * before transmission of the current packet; otherwise, it is 0.
   *
   * In the RXVECTOR, LAST_RSSI indicates the value of the LAST_RSSI
   * field from the PCLP header of the received packet. Valid values are
   * integers in the range of 0 to 15:
   * — Values of 2 to 14 represent power levels (–71+value×2) dBm.
   * — A value of 15 represents power greater than or equal to –42 dBm.
   * — A value of 1 represents power less than or equal to –68 dBm.
   * — A value of 0 indicates that the previous packet was not received a
   * SIFS period before the current transmission.
   *
   * \param length The length of the training field.
   */
  void SetLastRssi (uint8_t level);
  /**
   * Get the level of Last RSSI.
   * \return The level of Last RSSI.
   */
  uint8_t GetLastRssi (void) const;

private:
  PacketType  m_packetType;
  uint8_t     m_traingFieldLength;
  bool        m_beamTrackingRequest;      //*!< Flag to indicate the need for beam tracking. */
  uint8_t     m_lastRssi;                 //*!< Last Received Signal Strength Indicator. */
};

} // namespace ns3
