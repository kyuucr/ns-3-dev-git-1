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

#include "wifi/model/wifi-mac-header.h"

namespace ns3 {

class WifiMacHeaderAd : public WifiMacHeader
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  WifiMacHeaderAd();
  ~WifiMacHeaderAd();

  /**
   * Set Type/Subtype values for a DMG beacon header.
   */
  void SetDMGBeacon (void);
  /**
   * Set Type/Subtype values for an action no ack header.
   */
  void SetActionNoAck (void);
  /**
   * Set as DMG PPDU.
   */
  void SetAsDmgPpdu (void);
  /**
   * Get the type of A-MSDU>
   * \return
   */
  void SetQosAmsduType (uint8_t type);
  /**
   * The RDG/More PPDU subfield of the HT Control field is interpreted differently depending on whether it is
   * transmitted by an RD initiator or an RD responder, as defined in Table 8-13.
   * \return
   */
  void SetQosRdGrant (bool value);
  /**
   * The AC Constraint subfield of the QoS Control field for DMG frames indicates whether the mapped AC of
   * an RD data frame is constrained to a single AC, as defined in Table 8-12
   * \return
   */
  void SetQosAcConstraint (bool value);
  /**
   * Return true if the Type is Extension.
   *
   * \return true if Type is Extension, false otherwise
   */
  bool IsExtension (void) const;
  /**
   * Return true if the header is a DMG CTS header.
   *
   * \return true if the header is a DMG CTS header, false otherwise
   */
  bool IsDmgCts (void) const;
  /**
   * Return true if the header is a DTS header.
   *
   * \return true if the header is a DTS header, false otherwise
   */
  bool IsDmgDts (void) const;
  /**
   * Return true if the header is a DMG Beacon header.
   *
   * \return true if the header is a DMG Beacon header, false otherwise
   */
  bool IsDMGBeacon (void) const;
  /**
   * Return true if the header is a SSW header.
   *
   * \return true if the header is a SSW header, false otherwise
   */
  bool IsSSW (void) const;
  /**
   * Return true if the header is a SSW-Feedback header.
   *
   * \return true if the header is a SSW-Feedback header, false otherwise
   */
  bool IsSSW_FBCK (void) const;
  /**
   * Return true if the header is a SSW-ACK header.
   *
   * \return true if the header is a SSW-ACK header, false otherwise
   */
  bool IsSSW_ACK (void) const;
  /**
   * Return true if the header is a Poll header.
   *
   * \return true if the header is a Poll header, false otherwise
   */
  bool IsPollFrame (void) const;
  /**
   * Return true if the header is a SPR header.
   *
   * \return true if the header is a SPR header, false otherwise
   */
  bool IsSprFrame (void) const;
  /**
   * Return true if the header is a Grant header.
   *
   * \return true if the header is a Grant header, false otherwise
   */
  bool IsGrantFrame (void) const;
  /**
   * Return true if the header is an Action No Ack header.
   *
   * \return true if the header is an Action No Ack header, false otherwise
   */
  bool IsActionNoAck () const;
  /**
   * Return the type of A-MSDU
   *
   * \return the type of A-MSDU
   */
  bool GetQosAmsduType (void) const;
  /**
   * Check if it is RD Grant.
   *
   * \return True if RDG is set, otherwise false.
   */
  bool IsQosRdGrant (void) const;
  /**
   * \return
   */
  bool GetQosAcConstraint (void) const;
  /**
   * Request Beam Refinement Option.
   */
  void RequestBeamRefinement (void);
  /**
   * Disable Beam Refinement Option.
   */
  void DisableBeamRefinement (void);

  bool IsBeamRefinementRequested (void) const;
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
   * Disable Beam Tracking Capability.
   */
  void DisableBeamTracking (void);
  /**
   * \return True if Beam Tracking requested, otherwise false.
   */
  bool IsBeamTrackingRequested (void) const;

  virtual void SetType (WifiMacType type);
  virtual WifiMacType GetType (void) const;
  virtual uint32_t GetSize (void) const;

protected:
  virtual uint16_t GetFrameControl (void) const;
  virtual uint16_t GetQosControl (void) const;
  virtual void SetFrameControl (uint16_t control);
  virtual void SetQosControl (uint16_t qos);
private:
  /* DMG QoS Control Field */
  bool m_dmgPpdu;
  uint8_t m_qosAmsduType;
  uint8_t m_qosRdg;
  uint8_t m_qosAcConstraint;
  /* New fields for DMG Support. */
  uint32_t m_ctrlFrameExtension;
  bool m_beamRefinementRequired;
  bool m_beamTrackingRequired;
  PacketType m_brpPacketType;
  uint8_t m_trainingFieldLength;
};

} // namespace ns3
