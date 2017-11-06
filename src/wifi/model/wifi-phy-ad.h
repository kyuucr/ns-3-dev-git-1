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

#include "wifi/model/wifi-phy.h"

namespace ns3 {

/* BRP PHY Parameters */
#define aBRPminSCblocks     18
#define aBRPTRNBlock        4992
#define aSCGILength         64
#define aBRPminOFDMblocks   20
#define aSCBlockSize        512
#define TRNUnit             NanoSeconds (ceil (aBRPTRNBlock * 0.57))
#define OFDMSCMin           (aBRPminSCblocks * aSCBlockSize + aSCGILength) * 0.57
#define OFDMBRPMin          aBRPminOFDMblocks * 242

typedef uint8_t TimeBlockMeasurement;
typedef std::list<TimeBlockMeasurement> TimeBlockMeasurementList;
typedef TimeBlockMeasurementList::const_iterator TimeBlockMeasurementListCI;


class WifiPhyAd : public WifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WifiPhyAd();
  ~WifiPhyAd();

  /**
   * \param txVector TxVector companioned by this transmission.
   * \param fieldsRemaining The number of TRN Fields remaining till the end of transmission.
   */
  virtual void StartTrnTx (WifiTxVector txVector, uint8_t fieldsRemaining) = 0;
  /**
   * Send TRN Field to the destinated station.
   * \param txVector TxVector companioned by this transmission.
   * \param fieldsRemaining The number of TRN Fields remaining till the end of transmission.
   */
  void SendTrnField (WifiTxVector txVector, uint8_t fieldsRemaining);
  /**
   * Start receiving TRN Field
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveTrnField (WifiTxVector txVector, double rxPowerDbm, uint8_t fieldsRemaining);
  /**
   * Receive TRN Field
   * \param event The event related to the reception of this TRN Field.
   */
  void EndReceiveTrnField (uint8_t sectorId, uint8_t antennaId,
                           WifiTxVector txVector, uint8_t fieldsRemaining, Ptr<InterferenceHelper::Event> event);
  /**
   * This method is called once all the TRN Fields are received.
   */
  void EndReceiveTrnFields (void);
  /**
   * @brief PacketTxEnd
   * @param packet
   */
  void PacketTxEnd (Ptr<Packet> packet);
  /**
   * \return the current state of the PHY layer.
   */
  State GetPhyState (void) const;
  /**
   * \param size the number of bytes in the packet to send
   * \param txvector the transmission parameters used for this packet
   * \param preamble the type of preamble to use for this packet.
   *
   * \return the number of bits in the PPDU.
   */
  uint64_t CaluclateTransmittedBits (uint32_t size, WifiTxVector txvector);
  /**
   * \return the duration of the last received packet.
   */
  Time GetLastRxDuration (void) const;
  /**
   * \return the duration of the last transmitted packet.
   */
  Time GetLastTxDuration (void) const;

  /**
   *
   * \return the total number of transmitted bits.
   */
  uint64_t GetTotalTransmittedBits () const;
  /**
   * Return a WifiMode for Control PHY.
   *
   * \return a WifiMode for Control PHY.
   */
  static WifiMode GetDMG_MCS0 ();
  /**
   * Return a WifiMode for SC PHY with MCS1.
   *
   * \return a WifiMode for SC PHY with MCS1.
   */
  static WifiMode GetDMG_MCS1 ();
  /**
   * Return a WifiMode for SC PHY with MCS2.
   *
   * \return a WifiMode for SC PHY with MCS2.
   */
  static WifiMode GetDMG_MCS2 ();
  /**
   * Return a WifiMode for SC PHY with MCS3.
   *
   * \return a WifiMode for SC PHY with MCS3.
   */
  static WifiMode GetDMG_MCS3 ();
  /**
   * Return a WifiMode for SC PHY with MCS4.
   *
   * \return a WifiMode for SC PHY with MCS4.
   */
  static WifiMode GetDMG_MCS4 ();
  /**
   * Return a WifiMode for SC PHY with MCS5.
   *
   * \return a WifiMode for SC PHY with MCS5.
   */
  static WifiMode GetDMG_MCS5 ();
  /**
   * Return a WifiMode for SC PHY with MCS6.
   *
   * \return a WifiMode for SC PHY with MCS6.
   */
  static WifiMode GetDMG_MCS6 ();
  /**
   * Return a WifiMode for SC PHY with MCS7.
   *
   * \return a WifiMode for SC PHY with MCS7.
   */
  static WifiMode GetDMG_MCS7 ();
  /**
   * Return a WifiMode for SC PHY with MCS8.
   *
   * \return a WifiMode for SC PHY with MCS8.
   */
  static WifiMode GetDMG_MCS8 ();
  /**
   * Return a WifiMode for SC PHY with MCS9.
   *
   * \return a WifiMode for SC PHY with MCS9.
   */
  static WifiMode GetDMG_MCS9 ();
  /**
   * Return a WifiMode for SC PHY with MCS10.
   *
   * \return a WifiMode for SC PHY with MCS10.
   */
  static WifiMode GetDMG_MCS10 ();
  /**
   * Return a WifiMode for SC PHY with MCS11.
   *
   * \return a WifiMode for SC PHY with MCS11.
   */
  static WifiMode GetDMG_MCS11 ();
  /**
   * Return a WifiMode for SC PHY with MCS12.
   *
   * \return a WifiMode for SC PHY with MCS12.
   */
  static WifiMode GetDMG_MCS12 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS13.
   *
   * \return a WifiMode for OFDM PHY with MCS13.
   */
  static WifiMode GetDMG_MCS13 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS14.
   *
   * \return a WifiMode for OFDM PHY with MCS14.
   */
  static WifiMode GetDMG_MCS14 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS15.
   *
   * \return a WifiMode for OFDM PHY with MCS15.
   */
  static WifiMode GetDMG_MCS15 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS16.
   *
   * \return a WifiMode for OFDM PHY with MCS16.
   */
  static WifiMode GetDMG_MCS16 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS17.
   *
   * \return a WifiMode for OFDM PHY with MCS17.
   */
  static WifiMode GetDMG_MCS17 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS18.
   *
   * \return a WifiMode for OFDM PHY with MCS18.
   */
  static WifiMode GetDMG_MCS18 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS19.
   *
   * \return a WifiMode for OFDM PHY with MCS19.
   */
  static WifiMode GetDMG_MCS19 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS20.
   *
   * \return a WifiMode for OFDM PHY with MCS20.
   */
  static WifiMode GetDMG_MCS20 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS21.
   *
   * \return a WifiMode for OFDM PHY with MCS21.
   */
  static WifiMode GetDMG_MCS21 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS22.
   *
   * \return a WifiMode for OFDM PHY with MCS22.
   */
  static WifiMode GetDMG_MCS22 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS23.
   *
   * \return a WifiMode for OFDM PHY with MCS23.
   */
  static WifiMode GetDMG_MCS23 ();
  /**
   * Return a WifiMode for OFDM PHY with MCS24.
   *
   * \return a WifiMode for OFDM PHY with MCS24.
   */
  static WifiMode GetDMG_MCS24 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS25.
   *
   * \return a WifiMode for Low Power SC PHY with MCS25.
   */
  static WifiMode GetDMG_MCS25 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS26.
   *
   * \return a WifiMode for Low Power SC PHY with MCS26.
   */
  static WifiMode GetDMG_MCS26 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS27.
   *
   * \return a WifiMode for Low Power SC PHY with MCS27.
   */
  static WifiMode GetDMG_MCS27 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS28.
   *
   * \return a WifiMode for Low Power SC PHY with MCS28.
   */
  static WifiMode GetDMG_MCS28 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS29.
   *
   * \return a WifiMode for Low Power SC PHY with MCS29.
   */
  static WifiMode GetDMG_MCS29 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS30.
   *
   * \return a WifiMode for Low Power SC PHY with MCS30.
   */
  static WifiMode GetDMG_MCS30 ();
  /**
   * Return a WifiMode for Low Power SC PHY with MCS31.
   *
   * \return a WifiMode for Low Power SC PHY with MCS31.
   */
  static WifiMode GetDMG_MCS31 ();
  /**
   * Sets the antenna type.
   *
   * \param antenna the antenna type this PHY is associated with.
   */
  void SetAntenna (Ptr<AbstractAntenna> antenna);
  /**
   * Return the antenna type this PHY is associated with.
   *
   * \return Return the antenna type this PHY is associated with.
   */
  Ptr<AbstractAntenna> GetAntenna (void) const;
  /**
   * Sets the steerable antenna type.
   *
   * \param antenna the steerable antenna type this PHY is associated with.
   */
  void SetDirectionalAntenna (Ptr<DirectionalAntenna> antenna);
  /**
   * Return the steerable antenna type this PHY is associated with.
   *
   * \return Return the antenna type this PHY is associated with.
   */
  Ptr<DirectionalAntenna> GetDirectionalAntenna (void) const;
  /**
   * Public method used to fire a PhyTxBeginStats trace.
   * Implemented for encapsulation purposes.
   *
   * \param packet the packet being transmitted.
   * \param macAddress the MAC address of the transmitting station.
   * \param packetType the type of the packet (A-MPDU Aggregated).
   * \param duration the duration of the packet.
   */
  void NotifyTxBeginStats (Ptr<const Packet> packet, Address macAddress, uint32_t packetType, Time duration);

  /**
   * Report SNR Callback
   */
  typedef Callback<void, uint8_t, uint8_t, uint8_t, double, bool> ReportSnrCallback;
  /**
   * Register Report SNR Callback.
   * \param callback
   */
  virtual void RegisterReportSnrCallback (ReportSnrCallback callback);
  /**
   * This method is called when the DMG STA works in RDS mode.
   * \param srcSector The sector to use for communicating with the Source REDS.
   * \param srcAntenna The antenna to use for communicating with the Source REDS.
   * \param dstSector The sector to use for communicating with the Destination REDS.
   * \param dstAntenna The antenna to use for communicating with the Destination REDS.
   */
  virtual void ActivateRdsOpereation (uint8_t srcSector, uint8_t srcAntenna,
                                      uint8_t dstSector, uint8_t dstAntenna);
  /**
   * Disable RDS Opereation.
   */
  virtual void ResumeRdsOperation (void);
  /**
   * Disable RDS Opereation.
   */
  virtual void SuspendRdsOperation (void);
  /**
   * Report Measurement Ready Callback
   */
  typedef Callback<void, TimeBlockMeasurementList> ReportMeasurementCallback;
  virtual void StartMeasurement (uint16_t measurementDuration, uint8_t blocks);
  virtual void RegisterMeasurementResultsReady (ReportMeasurementCallback callback);

  virtual void ConfigureStandard (WifiPhyStandard standard);
  virtual WifiMode GetPlcpHeaderMode (WifiTxVector txVector) const;
  virtual Time GetPlcpHeaderDuration (WifiTxVector txVector) const;
  virtual Time GetPlcpPreambleDuration (WifiTxVector txVector) const;
  virtual Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype, uint8_t incFlag);
  virtual Time CalculateTxDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype, uint8_t incFlag);
  virtual void SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, MpduType mpdutype = NORMAL_MPDU);
  virtual void StartReceivePreambleAndHeader (Ptr<Packet> packet, double rxPowerW, Time rxDuration);

protected:
   virtual void ConfigureDefaultsForStandard (WifiPhyStandard standard);

protected:
  Ptr<AbstractAntenna> m_antenna;                 //!< Antenna type used in the PHY layer.
  Ptr<DirectionalAntenna> m_directionalAntenna;   //!< Antenna type used in the PHY layer.
  uint64_t m_totalBits;                           //!< Total number of bits sent over the air.
  Time m_txDuration;                              //!< Total time spent in sending packets with their preamble.
  Time m_rxDuration;                              //!< Duration of the last received packet.
private:
  /**
   * Configure YansWifiPhy with appropriate channel frequency and
   * supported rates for 802.11ad standard.
   */
  void Configure80211ad (void);
  /**
   * The last bit of the PSDU has arrived.
   *
   * \param packet the packet that the last bit has arrived
   * \param preamble the preamble of the arriving packet
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \param event the corresponding event of the first time the packet arrives
   */
  void EndPsduReceive (Ptr<Packet> packet, WifiPreamble preamble, MpduType mpdutype, Ptr<InterferenceHelper::Event> event);
  /**
   * The last bit of the PSDU has arrived but we are still waiting for the TRN Fields to be received.
   *
   * \param packet the packet that the last bit has arrived
   * \param preamble the preamble of the arriving packet
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \param event the corresponding event of the first time the packet arrives
   */
  void EndPsduOnlyReceive (Ptr<Packet> packet, PacketType packetType, WifiPreamble preamble, MpduType mpdutype, Ptr<InterferenceHelper::Event> event);

  virtual void MeasurementUnitEnded (void);
  virtual void EndMeasurement (void);

private:
  bool m_supportOFDM;                   //!< Flag to indicate whether we support OFDM PHY layer.
  bool m_supportLpSc;                   //!< Flag to indicate whether we support LP-SC PHY layer.
  bool m_rdsActivated;                    //!< Flag to indicate if RDS is activated;
  ReportSnrCallback m_reportSnrCallback;  //!< Callback to support
  bool m_psduSuccess;                     //!< Flag if the PSDU has been received successfully.
  uint8_t m_srcSector;                    //!< The ID of the sector used for communication with the source REDS.
  uint8_t m_srcAntenna;                   //!< The ID of the Antenna used for communication with the source REDS.
  uint8_t m_dstSector;                    //!< The ID of the sector used for communication with the destination REDS.
  uint8_t m_dstAntenna;                   //!< The ID of the Antenna used for communication with the destination REDS.
  uint8_t m_rdsSector;
  uint8_t m_rdsAntenna;
  Time m_lastTxDuration;
  /* For channel measurement */
  uint16_t m_measurementUnit;
  uint8_t m_measurementBlocks;
  ReportMeasurementCallback m_reportMeasurementCallback;
  TimeBlockMeasurementList m_measurementList;

};

} // namespace ns3
