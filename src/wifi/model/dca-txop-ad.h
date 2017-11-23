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

#include "wifi/model/dca-txop.h"

namespace ns3 {

class DcaTxopAd : public DcaTxop
{
public:
  DcaTxopAd();

  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, Ptr<const Packet>, const WifiMacHeader&> TxPacketOk;

  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed successfully.
   */
  void SetTxOkCallback (TxPacketOk callback);
  /**
   * \param callback the callback to invoke when a
   * transmission without ACK was completed successfully.
   */
  void SetTxOkNoAckCallback (TxOk callback) ;

  /**
   * Initiate Transmission in this CBAP period.
   * \param allocationID The unique ID of this allocation.
   * \param cbapDuration The duration of this service period in microseconds.
   */
  void InitiateTransmission (AllocationID allocationID, Time cbapDuration);
  /**
   * End Current CBAP Period.
   */
  void EndCurrentContentionPeriod (void);

protected:
  /**
   * Check if the DCF requires access.
   *
   * \return true if the DCF requires access,
   *         false otherwise
   */
  virtual bool NeedsAccess (void) const;
  virtual void RestartAccessIfNeeded (void);
  virtual void StartAccessIfNeeded (void);
  virtual void NotifyAccessGranted (void);
  virtual void StartNextFragment (void);
  virtual void EndTxNoAck (void);
  bool ShouldRtsAndAckBeDisabled () const;

private:
  TxPacketOk m_txPacketOkCallback;
  TxOk m_txOkNoAckCallback; //!< the transmit OK No ACK callback
  /* Store packet and header for service period */
  typedef std::pair<Ptr<const Packet>, WifiMacHeader> PacketInformation;
  typedef std::map<AllocationID, PacketInformation> StoredPackets;
  typedef StoredPackets::const_iterator StoredPacketsCI;
  StoredPackets m_storedPackets;

  AllocationID m_allocationID;      /* Allocation ID for the current contention period */
  Time m_transmissionStarted;   /* The time of the initiation of transmission */
  Time m_cbapDuration;          /* The remaining duration till the end of this CBAP allocation*/
};

} // namespace ns3
