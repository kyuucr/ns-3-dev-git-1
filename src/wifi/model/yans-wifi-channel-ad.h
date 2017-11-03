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

#include "wifi/model/yans-wifi-channel.h"

namespace ns3 {

class YansWifiChannelAd : public YansWifiChannel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  YansWifiChannelAd ();
  virtual ~YansWifiChannelAd ();

  /**
   * Send TRN Field.
   * \param sender the device from which the packet is originating.
   * \param txPowerDbm the tx power associated to the packet.
   * \param txVector the TXVECTOR associated to the packet.
   */
  void SendTrn (Ptr<YansWifiPhy> sender, double txPowerDbm, WifiTxVector txVector, uint8_t fieldsRemaining) const;

  /**
   * Add bloackage on a certain path between two WifiPhy objects.
   * \param srcWifiPhy
   * \param dstWifiPhy
   */
  void AddBlockage (double (*blockage)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy);
  void RemoveBlockage (void);
  void AddPacketDropper (bool (*dropper)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy);
  void RemovePacketDropper (void);
  /**
   * Load received signal strength file (Experimental mode).
   * \param fileName The path to the file.
   * \param updateFreuqnecy The update freuqnecy of the results.
   */
  void LoadReceivedSignalStrengthFile (std::string fileName, Time updateFreuqnecy);
  /**
   * Update current signal strength value
   */
  void UpdateSignalStrengthValue (void);

  virtual void Send (Ptr<YansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, Time duration) const;

private:
  /**
   * \param i index of the corresponding YansWifiPhy in the PHY list.
   * \param txVector the TXVECTOR of the packet.
   * \param txPowerDbm the transmitted signal strength [dBm].
   */
  void ReceiveTrn (uint32_t i, Ptr<YansWifiPhy> sender, WifiTxVector txVector, double txPowerDbm, uint8_t fieldsRemaining) const;

  double (*m_blockage) ();              //!< Blockage model.
  bool (*m_packetDropper) ();           //!< Packet Dropper Model.
  Ptr<WifiPhy> m_srcWifiPhy;
  Ptr<WifiPhy> m_dstWifiPhy;
  std::vector<double> m_receivedSignalStrength;    //!< List of received signal strength.
  uint64_t m_currentSignalStrengthIndex;           //!< Index of the current signal strength.
  bool m_experimentalMode;                         //!< Experimental mode used for injecting signal strength values.
  Time m_updateFrequency;                          //!< Update frequency of the results.
};

} // namespace ns3
