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
#include "wifi/model/edca-txop-n.h"

namespace ns3 {

class EdcaTxopNAd : public EdcaTxopN
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  EdcaTxopNAd ();
  ~EdcaTxopNAd ();

  /**
   * \param address recipient address of peer station involved in block ack mechanism.
   * \param tid traffic ID.
   *
   * \return the number of packets buffered for a specified agreement.
   *
   * Returns number of packets buffered for a specified agreement.
   */
  uint32_t GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid) const;
  /**
   * \param recipient address of peer station involved in block ack mechanism.
   * \param tid traffic ID.
   *
   * \return the number of packets for a specific agreement that need retransmission.
   *
   * Returns number of packets for a specific agreement that need retransmission.
   */
  uint32_t GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const;

  /**
   * Copy BlockAck Agreements
   * \param recipient
   * \param target
   */
  void CopyBlockAckAgreements (Mac48Address recipient, Ptr<EdcaTxopN> target);

  /**
   * Check if the EDCAF requires access.
   *
   * \return true if the EDCAF requires access,
   *         false otherwise.
   */
  bool NeedsAccess (void) const;
  /**
   * Update CBAP Remaining Time.
   */
  void UpdateRemainingTime (void);
  /**
   * End Current Contention Period.
   */
  void EndCurrentContentionPeriod (void);
  /**
   * Initiate Transmission in this CBAP period.
   * \param allocationID The unique ID of this allocation.
   * \param cbapDuration The duration of this service period in microseconds.
   */
  void InitiateTransmission (AllocationID allocationID, Time cbapDuration);

  virtual void NotifyAccessGranted (void);
  virtual void RestartAccessIfNeeded();
  virtual void StartAccessIfNeeded (void);
  virtual Mac48Address MapSrcAddressForAggregation (const WifiMacHeader &hdr);
  virtual Mac48Address MapDestAddressForAggregation (const WifiMacHeader &hdr);
  virtual void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr);
  virtual void SendBlockAckRequest (const Bar &bar);
  virtual void SendAddBaRequest (Mac48Address recipient, uint8_t tid, uint16_t startSeq, uint16_t timeout, bool immediateBAck);

private:
  /**
   * Reset state of the current EDCA.
   */
  void ResetState (void);

  /* Store packet and header for service period */
  typedef std::pair<Ptr<const Packet>, WifiMacHeader> PacketInformation;
  typedef std::map<AllocationID, PacketInformation> StoredPackets;
  typedef StoredPackets::const_iterator StoredPacketsCI;

  Time m_remainingDuration;         /* The remaining duration till the end of this CBAP allocation*/
  bool m_accessAllowed;             /* Flag to indicate whether the access is allowed for the curent EDCA Queue*/
  bool m_missedACK;
  bool m_firstTransmission;
};

} // namespace ns3
