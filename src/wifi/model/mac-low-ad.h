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

#include "wifi/model/mac-low.h"

namespace ns3 {

class MacLowTransmissionParametersAd : public MacLowTransmissionParameters
{
public:
  MacLowTransmissionParametersAd();
  void SetAsBoundedTransmission (void);
  bool IsTransmissionBounded (void) const;
  void SetMaximumTransmissionDuration (Time duration);
  Time GetMaximumTransmissionDuration (void) const;
  bool IsCbapAccessPeriod (void) const;
  void SetTransmitInSericePeriod ();
private:
  bool m_boundedTransmission;
  Time m_transmissionDuration;
  bool m_transmitInServicePeriod;
};

class MacLowAd : public MacLow
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Constructor
   */
  MacLowAd();

  /**
   * Set Short Beamforming Interframe Space (SBIFS) of this MacLow.
   *
   * \param sbifs SBIFS of this MacLow
   */
  void SetSbifs (Time sbifs);
  /**
   * Set Medimum Beamforming Interframe Space (SBIFS) of this MacLow.
   *
   * \param mbifs MBIFS of this MacLow
   */
  void SetMbifs (Time mbifs);
  /**
   * Set Long Beamforming Interframe Space (LBIFS) of this MacLow.
   *
   * \param lbifs LBIFS of this MacLow
   */
  void SetLbifs (Time lbifs);
  /**
   * Set Beam Refinement Interframe Space (BRIFS) of this MacLow.
   *
   * \param sbifs SBIFS of this MacLow
   */
  void SetBrifs (Time brifs);
  /**
   * Return Short Beamforming Interframe Space (SBIFS) of this MacLow.
   *
   * \return SBIFS
   */
  Time GetSbifs (void) const;
  /**
   * Return Medium Beamforming Interframe Space (SBIFS) of this MacLow.
   *
   * \return MBIFS
   */
  Time GetMbifs (void) const;
  /**
   * Return Large Beamforming Interframe Space (LBIFS) of this MacLow.
   *
   * \return LBIFS
   */
  Time GetLbifs (void) const;
  /**
   * Return Beamforming Refinement Interframe Space (BRIFS) of this MacLow.
   *
   * \return BRIFS
   */
  Time GetBrifs (void) const;

  typedef Callback<void, const WifiMacHeader &> TransmissionOkCallback;
  /**
   * \param packet packet to send
   * \param hdr 802.11 header for packet to send
   * \param parameters the transmission parameters to use for this packet.
   * \param listener listen to transmission events.
   *
   * Start the transmission of the input packet and notify the listener
   * of transmission events. This function is used for management frame transmission in BTI and ATI.
   */
  void TransmitSingleFrame (Ptr<const Packet> packet,
                            const WifiMacHeader* hdr,
                            MacLowTransmissionParameters params,
                            Ptr<DcaTxop> dca);
  /**
   * Resume Transmission for the current allocation if transmission has been suspended.
   * \param overrideDuration Flagt to indicate if we want to override duration field
   * \param duration The duration of the current Allocation
   * \param listener
   */
//  void ResumeTransmission (bool overrideDuration, Time duration, MacLowTransmissionListener *listener);
  /**
   * Resume Transmission for the current allocation if transmission has been suspended.
   * \param listener
   */
  void ResumeTransmission (Time duration, Ptr<DcaTxop> dca);
  /**
   * Restore Allocation Parameters for specific allocation SP or CBAP.
   * \param allocationId The ID of the allocation.
   */
  void RestoreAllocationParameters (AllocationID allocationId);

  void StoreAllocationParameters (void);
  /**
   * Check whether a transmission has been suspended due to time constraints for the restored allocation.
   * \return True if transmission has been suspended otherwise false.
   */
  bool IsTransmissionSuspended (void) const;

  bool RestoredSuspendedTransmission (void) const;
  /**
   * \param packet packet to send
   * \param hdr 802.11 header for packet to send
   * \param parameters the transmission parameters to use for this packet.
   * \param listener listen to transmission events.
   *
   * Start the transmission of the input packet and notify the listener
   * of transmission events.
   */
  virtual void StartTransmission (Ptr<const Packet> packet,
                                  const WifiMacHeader* hdr,
                                  MacLowTransmissionParameters parameters,
                                  TransmissionOkCallback callback);
  /**
   * Set high MAC
   * \param mac A pointer to the high MAC.
   */
  void SetMacHigh (Ptr<WifiMac> mac);
  /**
   * Calculate DMG transaction duration including packet transmission + acknowledgement.
   * \param packetDuration The duration of the packet to transmit
   */
  Time CalculateDmgTransactionDuration (Time packetDuration);
  /**
   * Calculate DMG transaction duration including packet transmission + acknowledgement.
   * \param packet
   * \param hdr
   * \return The total duration of the transaction
   */
  Time CalculateDmgTransactionDuration (Ptr<Packet> packet, WifiMacHeader &hdr);
  /**
   * Return a TXVECTOR for the DMG Control frame given the destination.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param packet the packet being asked for TXVECTOR
   * \param hdr the WifiMacHeader
   * \return TXVECTOR for the given packet
   */
  virtual WifiTxVector GetDmgTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const;

private:
  /**
   * Return the total DMG CTS size (including FCS trailer).
   *
   * \return the total DMG CTS size
   */
  uint32_t GetDmgCtsSize (void) const;
  /**
   * Get DMG Control TxVector
   * \return
   */
  WifiTxVector GetDmgControlTxVector () const;
  /**
   * Return a TXVECTOR for the CTS-to-self frame.
   * The function consults WifiRemoteStationManager, which controls the rate
   * to different destinations.
   *
   * \param packet the packet that requires CTS-to-self
   * \param hdr the Wifi header of the packet
   * \return TXVECTOR for the CTS-to-self operation
   */
  WifiTxVector GetCtsToSelfTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const;
  /**
   * Return the time required to transmit the DMG CTS (including preamble and FCS).
   *
   * \return the time required to transmit the DMG CTS (including preamble and FCS)
   */
  Time GetDmgCtsDuration (void) const;
  /**
   * Return the time required to transmit the ACK to the specified address
   * given the TXVECTOR of the DATA (including preamble and FCS).
   *
   * \param to
   * \param txVector
   * \return the time required to transmit the ACK (including preamble and FCS)
   */
  Time GetDmgControlDuration (WifiTxVector txVector, uint32_t payloadSize) const;
  /**
   * Send CTS after receiving RTS.
   *
   * \param source
   * \param duration
   * \param rtsTxVector
   * \param rtsSnr
   */
  void SendDmgCtsAfterRts (Mac48Address source, Time duration, WifiTxVector rtsTxVector, double rtsSnr);

  virtual void StartTransmission (Ptr<const Packet> packet, const WifiMacHeader* hdr,
                                  MacLowTransmissionParameters parameters, Ptr<DcaTxop> dca);
  void SendMpdu (Ptr<const Packet> packet, WifiTxVector txVector, Time frameDuration, MpduType mpdutype);

protected:
  virtual void PeelHeader(Ptr<Packet> packet);
  virtual void ScheduleCts(WifiTxVector txVector, double rxSnr);
  virtual void DoReceiveOk (Ptr<Packet> packet, double rxSnr, WifiTxVector txVector, bool ampduSubframe);
  virtual void NotifyNav (Ptr<const Packet> packet,  Ptr<const WifiMacHeader> hdr, WifiPreamble preamble);
  virtual void NotifyDoNavResetNow (Time duration) const;
  virtual void NotifyDoNavStartNow(Time duration) const;
  virtual void NotifyAckTimeoutStartNow (Time duration) const;
  virtual void NotifyAckTimeoutResetNow () const;
  virtual void NotifyCtsTimeoutStartNow (Time duration) const;
  virtual void NotifyCtsTimeoutResetNow () const;
  virtual void ForwardDown (Ptr<const Packet> packet, const WifiMacHeader *hdr, WifiTxVector txVector);
  virtual bool ReceiveMpdu (Ptr<Packet> packet, WifiMacHeader hdr);
  virtual Time GetAPPDUMaxTime () const;
  virtual Time CalculateAMPDUSubrame(Ptr<const Packet> packet, WifiTxVector txVector);

private:
  Time m_sbifs;                         //!< Short Beamformnig Interframe Space.
  Time m_mbifs;                         //!< Medium Beamformnig Interframe Space.
  Time m_lbifs;                         //!< Long Beamformnig Interframe Space.
  Time m_brifs;                         //!< Beam Refinement Protocol Interframe Spacing.

  double mpduSnr;
  TransmissionOkCallback m_transmissionCallback;
  Ptr<WifiMac> m_mac;

  struct SubMpduInfo
  {
    MpduType type;
    WifiMacHeader hdr;
    Ptr<Packet> packet;
    Time mpduDuration;
  };

  /**
     * TracedCallback signature for transmitting MPDUs.
     *
     * \param number The number of MPDUs being transmitted
     */
  typedef void (* TransmittedMpdus)(uint32_t number);
  /**
     * The trace source fired when packets coming into the device
     * are dropped at the Wifi MAC layer.
     *
     * \see class CallBackTraceSource
     */
  TracedCallback<uint32_t> m_transmittedMpdus;

  /* Variables for suspended data transmission for different allocation periods */
  struct AllocationParameters {
    Ptr<Packet> packet;
    WifiMacHeader hdr;
    bool isAmpdu;
    MacLowTransmissionParameters txParams;
    WifiTxVector txVector;
    Ptr<WifiMacQueue> aggregateQueue;
  };

  typedef std::map<AllocationID, AllocationParameters> AllocationPeriodsTable;
  typedef AllocationPeriodsTable::const_iterator AllocationPeriodsTableCI;
  AllocationPeriodsTable m_allocationPeriodsTable;
  AllocationID m_currentAllocationID;
  AllocationParameters m_currentAllocation;
  bool m_transmissionSuspended;               //!< Flag to indicate that we have suspended transmission applicable for 802.11ad only.
  bool m_restoredSuspendedTransmission;       //!< Flag to indicate that we have more time to traansmit more packets.
};

} // namespace ns3
