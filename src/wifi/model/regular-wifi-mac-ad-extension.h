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

#include "ns3/object.h"
#include "ns3/nstime.h"

#include "wifi/model/regular-wifi-mac.h"
#include "wifi/model/wifi-mac-extension-ad.h"

namespace ns3 {

typedef enum {
  FST_INITIAL_STATE = 0,
  FST_SETUP_COMPLETION_STATE,
  FST_TRANSITION_DONE_STATE,
  FST_TRANSITION_CONFIRMED_STATE
} FST_STATES;

typedef struct {
  uint32_t ID;                      //!< A unique ID that corresponds to RA and TA.
  FST_STATES CurrentState;          //!< Current state of the FST State Machine.
  Time STT;                         //!< State Transition Timer.
  bool IsInitiator;                 //!< Is FST Session Initiator.
  BandID NewBandId;                 //!< The new band the station is transferring to.
  uint32_t LLT;                     //!< Link Loss Time for this FST Session.
  EventId LinkLossCountDownEvent;   //!< Event for Link Loss Timeout.
} FstSession;

class RegularWifiMacExtensionAd : virtual public RegularWifiExtensionInterface, public WifiMacExtensionAd
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RegularWifiMacExtensionAd(const Ptr<WifiMac> &mac);

  virtual void TxOk (Ptr<const Packet> currentPacket, const WifiMacHeader &hdr);

  /**
   * This type defines a mapping between between the MAC Address of
   * the Peer FST and FST Session Variables.
   */
  typedef std::map<Mac48Address, FstSession> FstSessionMap;

  /**
   * Setup FST session as initiator.
   * \param staAddress The address of the sta to establish FST session with it.
   */
  void SetupFSTSession (Mac48Address staAddress);
  /**
   * Get Type Of Station.
   * \return station type
   */
  TypeOfStation GetTypeOfStation (void) const;

  Callback<void> m_sessionTransfer; //!< Callback when a session transfer takes place
  bool m_supportMultiBand;       //!< Flag to indicate whether we support multiband operation.
  uint8_t m_fstTimeout;          //!< FST Session TimeOut duration in TUs.

  /** Fast Session Transfer Functions **/

  /**
   * Send FST Setup Response to Initiator.
   * \param to The Mac address of the initiator.
   * \param token The token dialog.
   * \param status The status of the setup operation.
   * \param sessionTransition
   */
  void SendFstSetupResponse (Mac48Address to, uint8_t token, uint16_t status, SessionTransitionElement sessionTransition);
  void SendFstAckRequest (Mac48Address to, uint8_t dialog, uint32_t fstsID);
  void SendFstAckResponse (Mac48Address to, uint8_t dialog, uint32_t fstsID);
  void SendFstTearDownFrame (Mac48Address to);
  /**
   * Get MultiBand Element associated to this STA device.
   * \return The MultiBand Information Element correspodning to this WifiMac.
   */
  virtual Ptr<MultiBandElement> GetMultiBandElement (void) const = 0;
  /**
   * Notify Band Changed
   * \param standard
   * \param address The address of the peer station.
   * \param isInitiator flag to indicate whether we are the initiator of the FST or not.
   */
  void NotifyBandChanged (enum WifiPhyStandard standard, Mac48Address address, bool isInitiator);

  typedef Callback<void, WifiPhyStandard, Mac48Address, bool> BandChangedCallback;

  void RegisterBandChangedCallback (BandChangedCallback callback);

  /* Fast Session Transfer */
  /**
   * ChangeBand
   * \param peerStation
   * \param bandId
   * \param isInitiator
   */
  void ChangeBand (Mac48Address peerStation, BandID bandId, bool isInitiator);
  void MacTxOk (Mac48Address address);
  void MacRxOk (Mac48Address address);

  uint32_t m_llt;                 //!< Link Loss Timeout.
  uint32_t m_fstId;               //!< Fast session transfer ID.
  FstSessionMap m_fstSessionMap;

  /**
   * This Boolean is set \c true if this WifiMac is to model
   * 802.11ad. It is exposed through the attribute system.
   */
  bool m_dmgSupported;
  /**
   * Enable or disable DMG support for the device.
   *
   * \param enable whether DMG is supported
   */
  void SetDmgSupported (bool enable);
  /**
   * Return whether the device supports QoS.
   *
   * \return true if DMG is supported, false otherwise
   */
  bool GetDmgSupported () const;

  virtual void DoReceive (Ptr<Packet> packet, const WifiMacHeader *hdr);
  virtual void SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager);
  virtual void FinishConfigureStandard (WifiPhyStandard standard);

private:
  BandChangedCallback m_bandChangedCallback;
};

} // namespace ns3
