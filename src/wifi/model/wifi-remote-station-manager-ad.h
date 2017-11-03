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

#include "wifi/model/wifi-remote-station-manager.h"

namespace ns3 {

class WifiRemoteStationManagerAd : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  WifiRemoteStationManagerAd();
  ~WifiRemoteStationManagerAd();

  /**
   * Records DMG capabilities of the remote station.
   *
   * \param from the address of the station being recorded
   * \param dmgCapabilities the DMG capabilities of the station
   */
  void AddStationDmgCapabilities (Mac48Address from, Ptr<DmgCapabilities> dmgCapabilities);
  /**
   * Enable or disable DMT capability support.
   *
   * \param enable enable or disable DMG capability support
   */
  void SetDmgSupported (bool enable);
  /**
   * Return whether the device has DMG capability support enabled.
   *
   * \return true if DMG capability support is enabled, false otherwise
   */
  bool HasDmgSupported (void) const;
  /**
   * \param address remote address
   * \param header MAC header
   * \param packet the packet to send
   *
   * \return the transmission mode to use to send this DMG packet
   */
  WifiTxVector GetDmgTxVector (Mac48Address address, const WifiMacHeader *header, Ptr<const Packet> packet);
  /**
   * \return the DMG Control transmission mode (MCS0).
   */
  WifiTxVector GetDmgControlTxVector (void);
  /**
   * \return the DMG Lowest SC transmission mode (MCS1).
   */
  WifiTxVector GetDmgLowestScVector (void);
  /**
   * Return the number of associated stations.
   *
   * \return Number of associated stations.
   */
  uint32_t GetNAssociatedStation (void) const;
  /**
   * Return the value of the last received SNR.
   *
   * \return the value of the last received SNR.
   */
  double GetRxSnr (void) const;

  void RegisterTxOkCallback (Callback<void, Mac48Address> callback);
  void RegisterRxOkCallback (Callback<void, Mac48Address> callback);

  virtual WifiTxVector GetDataTxVector (Mac48Address address, const WifiMacHeader *header,
                                        Ptr<const Packet> packet);
  virtual void ReportRtsOk (Mac48Address address, const WifiMacHeader *header,
                            double ctsSnr, WifiMode ctsMode, double rtsSnr);
  virtual void ReportDataOk (Mac48Address address, const WifiMacHeader *header,
                             double ackSnr, WifiMode ackMode, double dataSnr);
  virtual void ReportRxOk (Mac48Address address, const WifiMacHeader *header,
                           double rxSnr, WifiMode txMode);
  virtual WifiMode GetControlAnswerMode (Mac48Address address, WifiMode reqMode);

protected:
  /**
   * Return the states of the assoicated stations.
   *
   * \return List of associated stations.
   */
  StationStates* GetStationStates (void);

private:
  bool m_dmgSupported;  //!< Flag if DMG capability is supported
  double m_rxSnr;       //!< The value of the last received SNR.
  /**
   * The trace source fired when the transmission of a single MPDU has successed.
   */
  TracedCallback<Mac48Address> m_macTxOk;

  Callback<void, Mac48Address> m_txCallbackOk;
  Callback<void, Mac48Address> m_rxCallbackOk;
};

} //namespace ns3
