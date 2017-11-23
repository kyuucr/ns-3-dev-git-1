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

#include "ns3/header.h"
#include "dmg-capabilities.h"
#include "dmg-information-elements.h"
#include "ctrl-headers.h"
#include "ext-headers.h"
#include "fields-headers.h"
#include "common-header.h"

namespace ns3 {

/**
 * \ingroup wifi
 * Implement the header for action frames of type DMG ADDTS request.
 */
class DmgAddTSRequestFrame : public Header
{
public:
  DmgAddTSRequestFrame ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
  * The Dialog Token field.
  * \param token
  */
  void SetDialogToken (uint8_t token);
  void SetDmgTspecElement (DmgTspecElement &element);

  uint8_t GetDialogToken (void) const;
  DmgTspecElement GetDmgTspec (void) const;

private:
  uint8_t m_dialogToken;
  DmgTspecElement m_dmgTspecElement;

};

/**
 * \ingroup wifi
 * Implement the header for action frames of type DMG ADDTS request.
 */
class DmgAddTSResponseFrame : public Header
{
public:
  DmgAddTSResponseFrame ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
  * The Dialog Token field.
  * \param token
  */
  void SetDialogToken (uint8_t token);
  void SetStatusCode (StatusCode status);
  void SetTsDelay (TsDelayElement &element);
  void SetDmgTspecElement (DmgTspecElement &element);

  uint8_t GetDialogToken (void) const;
  StatusCode GetStatusCode (void) const;
  TsDelayElement GetTsDelay (void) const;
  DmgTspecElement GetDmgTspec (void) const;

private:
  uint8_t m_dialogToken;
  StatusCode m_status;
  TsDelayElement m_tsDelayElement;
  DmgTspecElement m_dmgTspecElement;
};

/**
 * \ingroup wifi
 * Implement the header for action frames of type DELTS (8.5.3.4).
 */
class DelTsFrame : public Header
{
public:
  DelTsFrame ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetReasonCode (uint16_t reason);
  void SetDmgAllocationInfo (DmgAllocationInfo info);

  uint16_t GetReasonCode (void) const;
  DmgAllocationInfo GetDmgAllocationInfo (void) const;

private:
  uint8_t m_tsInfo[3];
  uint16_t m_reasonCode;
  DmgAllocationInfo m_dmgAllocationInfo;

};

/**
 * \ingroup wifi
 * The Radio Measurement Request frame uses the Action frame body format. It is transmitted by a STA
 * requesting another STA to make one or more measurements on one or more channels.
 */
class RadioMeasurementRequest : public Header
{
public:
  RadioMeasurementRequest ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to a nonzero value chosen by the STA sending the radio measurement request
   * to identify the request/report transaction.
   * \param token
   */
  void SetDialogToken (uint8_t token);
  /**
   * The Number of Repetitions field contains the requested number of repetitions for all the Measurement
   * Request elements in this frame. A value of 0 in the Number of Repetitions field indicates Measurement
   * Request elements are executed once without repetition. A value of 65 535 in the Number of Repetitions field
   * indicates Measurement Request elements are repeated until the measurement is cancelled or superseded.
   * \param repetitions
   */
  void SetNumberOfRepetitions (uint16_t repetitions);

  void AddMeasurementRequestElement (Ptr<WifiInformationElement> elem);

  uint8_t GetDialogToken (void) const;
  uint16_t GetNumberOfRepetitions (void) const;
  WifiInfoElementList GetListOfMeasurementRequestElement (void) const;

private:
  uint8_t m_dialogToken;
  uint16_t m_numOfRepetitions;
  WifiInfoElementList m_list;

};

/**
 * \ingroup wifi
 * The Radio Measurement Report frame uses the Action frame body format. It is transmitted by a STA in
 * response to a Radio Measurement Request frame or by a STA providing a triggered autonomous measurement report.
 */
class RadioMeasurementReport : public Header
{
public:
  RadioMeasurementReport ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to the value in the corresponding Radio Measurement Request frame. If the
   * Radio Measurement Report frame is not being transmitted in response to a Radio Measurement Request
   * frame then the Dialog token is set to 0.
   * \param token
   */
  void SetDialogToken (uint8_t token);
  void AddMeasurementReportElement (Ptr<WifiInformationElement> elem);

  uint8_t GetDialogToken (void) const;
  WifiInfoElementList GetListOfMeasurementReportElement (void) const;

private:
  uint8_t m_dialogToken;
  WifiInfoElementList m_list;

};

/**
 * \ingroup wifi
 * The Link Measurement Request frame uses the Action frame body format and is transmitted by a STA to
 * request another STA to respond with a Link Measurement Report frame to enable measurement of link path
 * loss and estimation of link margin.
 */
class LinkMeasurementRequest : public Header
{
public:
  LinkMeasurementRequest ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to a nonzero value chosen by the STA sending the request to identify the
   * transaction.
   * \param token
   */
  void SetDialogToken (uint8_t token);
  /**
   * The Transmit Power Used field is set to the transmit power used to transmit the frame containing the Link
   * Measurement Request, as described in 8.4.1.20.
   * \param power
   */
  void SetTransmitPowerUsed (uint8_t power);
  /**
   * The Max Transmit Power field provides the upper limit on the transmit power as measured at the output of
   * \the antenna connector to be used by the transmitting STA on its operating channel. This field is described in
   * 8.4.1.19. The Max Transmit Power field is a twos complement signed integer and is 1 octet in length,
   * providing an upper limit, in a dBm scale, on the transmit power as measured at the output of the antenna
   * connector to be used by the transmitting STA on its operating channel. The maximum tolerance for the value
   * reported in Max Transmit Power field is ±5 dB. The value of the Max Transmit Power field is equal to the
   * minimum of the maximum powers at which the STA is permitted to transmit in the operating channel by
   * device capability, policy, and regulatory authority.
   * \param power
   */
  void SetMaxTransmitPower (uint8_t power);
  void AddSubElement (Ptr<WifiInformationElement> elem);

  uint8_t GetDialogToken (void) const;
  uint8_t GetTransmitPowerUsed (uint8_t power) const;
  uint8_t GetMaxTransmitPower (uint8_t power) const;
  /**
   * Get a specific SubElement by ID.
   * \param id The ID of the Wifi SubElement.
   * \return
   */
  Ptr<WifiInformationElement> GetSubElement (WifiInformationElementId id);
  /**
   * Get List of SubElement associated with this frame.
   * \return
   */
  WifiInformationElementMap GetListOfSubElements (void) const;

private:
  uint8_t m_dialogToken;
  uint8_t m_transmitPowerUsed;
  uint8_t m_maxTransmitPower;
  WifiInformationElementMap m_map;

};

/**
 * \ingroup wifi
 * The Link Measurement Report frame uses the Action frame body format and is transmitted by a STA in
 * response to a Link Measurement Request frame.
 */
class LinkMeasurementReport : public Header
{
public:
  LinkMeasurementReport ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDialogToken (uint8_t token);
  void SetTpcReportElement (uint32_t elem);
  void SetReceiveAntennaId (uint8_t id);
  void SetTransmitAntennaId (uint8_t id);
  void SetRCPI (uint8_t value);
  void SetRSNI (uint8_t value);
  void AddSubElement (Ptr<WifiInformationElement> elem);

  uint8_t GetDialogToken (void) const;
  uint32_t GetTpcReportElement (void) const;
  uint8_t GetReceiveAntennaId (void) const;
  uint8_t GetTransmitAntennaId (void) const;
  uint8_t GetRCPI (void) const;
  uint8_t GetRSNI (void) const;
  /**
   * Get a specific SubElement by ID.
   * \param id The ID of the Wifi SubElement.
   * \return
   */
  Ptr<WifiInformationElement> GetSubElement (WifiInformationElementId id);
  /**
   * Get List of SubElement associated with this frame.
   * \return
   */
  WifiInformationElementMap GetListOfSubElements (void) const;

private:
  uint8_t m_dialogToken;
  uint32_t m_tpcElement;
  uint8_t m_receiveAntId;
  uint8_t m_transmitAntId;
  uint8_t m_rcpi;
  uint8_t m_rsni;
  WifiInformationElementMap m_map;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Public QAB Frame.
 */
class ExtQabFrame : public Header
{
public:
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const = 0;
  virtual uint32_t Deserialize (Buffer::Iterator start) = 0;

  void SetDialogToken (uint8_t token);
  void SetRequesterApAddress (Mac48Address address);
  void SetResponderApAddress (Mac48Address address);

  uint8_t GetDialogToken (void) const;
  Mac48Address GetRequesterApAddress (void) const;
  Mac48Address GetResponderApAddress (void) const;

protected:
  uint8_t m_dialogToken;
  Mac48Address m_requester;
  Mac48Address m_responder;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Public QAB Request (8.5.8.25).
 * The QAB Request Action frame is transmitted by an AP to another AP to schedule quiet periods that
 * facilitate the detection of other system operating in the same band.
 */
class ExtQabRequestFrame : public ExtQabFrame
{
public:
  ExtQabRequestFrame ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  void SetQuietPeriodRequestElement (QuietPeriodRequestElement &element);

  QuietPeriodRequestElement GetQuietPeriodRequestElement (void) const;

private:
  QuietPeriodRequestElement m_element;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Public QAB Response (8.5.8.26).
 * A QAB Response frame is sent in response to a QAB Request frame.
 */
class ExtQabResponseFrame : public ExtQabFrame
{
public:
  ExtQabResponseFrame ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  void SetQuietPeriodResponseElement (QuietPeriodResponseElement &element);

  QuietPeriodResponseElement GetQuietPeriodResponseElement (void) const;

private:
  QuietPeriodResponseElement m_element;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type  Common Information frame.
 */
class ExtInformationFrame : public Header, public MgtFrame
{
public:
  ExtInformationFrame ();

  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const = 0;
  virtual uint32_t Deserialize (Buffer::Iterator start) = 0;

  /**
   * Set the MAC address of the STA whose information is being requested. If this frame is sent to
   * the PCP and the value of the Subject Address field is the broadcast address, then the STA is
   * requesting information regarding all associated STAs.
   * \param address The Mac address
   */
  void SetSubjectAddress (Mac48Address address);
  /**
   * Set the request element which contains a list of requested information IDs.
   * @param elem the Request Element.
   */
  void SetRequestInformationElement (Ptr<RequestElement> elem);
  /**
   * The DMG Capabilities element carries information about the transmitter STA and other STAs known
   * to the transmitter STA.
   * \param elem The DMG Capabilities Information Element.
   */
  void AddDmgCapabilitiesElement (Ptr<DmgCapabilities> elem);

  Mac48Address GetSubjectAddress (void) const;
  Ptr<RequestElement> GetRequestInformationElement (void) const;
  DmgCapabilitiesList GetDmgCapabilitiesList (void) const;

protected:
  Mac48Address m_subjectAddress;
  Ptr<RequestElement> m_requestElement;
  DmgCapabilitiesList m_dmgCapabilitiesList;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Information Request frame (8.5.20.4).
 */
class ExtInformationRequest : public ExtInformationFrame
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Information Request frame (8.5.20.4).
 */
class ExtInformationResponse : public ExtInformationFrame
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

};

enum HandoverReason
{
  LeavingPBSS = 0,
  LowPowerPCP = 1,
  QualifiedSTA = 2,
  BecomePCP = 3
};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type handover request frame.
 */
class ExtHandoverRequestHeader : public Header
{
public:
  ExtHandoverRequestHeader ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the reason that the current PCP intends to do the PCP handover.
   * \param reason
   */
  void SetHandoverReason (enum HandoverReason reason);
  /**
   * Set the number of beacon intervals, excluding the beacon interval in which this frame
   * is transmitted, remaining until the handover takes effect.
   * \param length
   */
  void SetHandoverRemainingBI (uint8_t remaining);

  enum HandoverReason GetHandoverReason (void) const;
  uint8_t GetHandoverRemainingBI (void) const;

private:
  uint8_t m_handoverReason;
  uint8_t m_remainingBI;

};

enum HandoverRejectReason
{
  LowPower = 0,
  HandoverInProgress = 1,
  InvalidBI = 2,
  UnspecifiedReason = 3
};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type handover response frame.
 */
class ExtHandoverResponseHeader : public Header
{
public:
  ExtHandoverResponseHeader ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set whether the STA accepted the handover request.
   * \param  result A value of 0 indicates that the STA accepts the handover request.
   * A value of 1 indicates that the STA does not accept the handover request.
   */
  void SetHandoverResult (bool result);
  /**
   *
   */
  void SetHandoverRejectReason (enum HandoverRejectReason reason);

  bool GetHandoverResult (void) const;
  enum HandoverRejectReason GetHandoverRejectReason (void) const;

private:
  bool m_handoverResult;
  uint8_t m_handoverRejectReason;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Relay Search Request frame.
 */
class ExtRelaySearchRequestHeader : public Header
{
public:
  ExtRelaySearchRequestHeader ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDialogToken (uint8_t token);
  void SetDestinationRedsAid (uint16_t aid);

  uint8_t GetDialogToken (void) const;
  uint16_t GetDestinationRedsAid (void) const;

private:
  uint8_t m_dialogToken;
  uint16_t m_aid;

};

typedef std::map<uint16_t, RelayCapabilitiesInfo> RelayCapableStaList;

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Relay Search Response frame.
 */
class ExtRelaySearchResponseHeader : public Header
{
public:
  ExtRelaySearchResponseHeader ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDialogToken (uint8_t token);
  void SetStatusCode (uint16_t status);
  void AddRelayCapableStaInfo (uint8_t aid, RelayCapabilitiesInfo &element);
  void SetRelayCapableList (RelayCapableStaList &list);

  uint8_t GetDialogToken (void) const;
  uint16_t GetStatusCode (void) const;
  RelayCapableStaList GetRelayCapableList (void) const;

private:
  uint8_t m_dialogToken;
  uint16_t m_statusCode;
  RelayCapableStaList m_list;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Multi-relay Channel Measurement Request frame.
 * The Multi-Relay Channel Measurement Request frame is transmitted by a STA initiating relay operation to
 * the recipient STA in order to obtain Channel Measurements between the recipient STA and the other STA
 * participating in the relay operation.
 *
 */
class ExtMultiRelayChannelMeasurementRequest : public Header
{
public:
  ExtMultiRelayChannelMeasurementRequest ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to a nonzero value chosen by the STA sending the Multi-Relay Channel
   * Measurement Request frame to identify the request/report transaction.
   * \param token
   */
  void SetDialogToken (uint8_t token);

  uint8_t GetDialogToken (void) const;

private:
  uint8_t m_dialogToken;

};

typedef std::vector<Ptr<ExtChannelMeasurementInfo> > ChannelMeasurementInfoList;

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Relay Search Response frame.
 */
class ExtMultiRelayChannelMeasurementReport : public Header
{
public:
  ExtMultiRelayChannelMeasurementReport ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDialogToken (uint8_t token);
  void AddChannelMeasurementInfo (Ptr<ExtChannelMeasurementInfo> element);
  void SetChannelMeasurementList (ChannelMeasurementInfoList &list);

  uint8_t GetDialogToken (void) const;
  ChannelMeasurementInfoList GetChannelMeasurementInfoList (void) const;

private:
  uint8_t m_dialogToken;
  ChannelMeasurementInfoList m_list;

};

/**
* \ingroup wifi
* Implement a generic header for extension frames of type RLS (Relay Link Setup) frame.
*/
class ExtRlsFrame : public Header
{
public:
 ExtRlsFrame ();

 virtual uint32_t GetSerializedSize (void) const;
 virtual void Serialize (Buffer::Iterator start) const = 0;
 virtual uint32_t Deserialize (Buffer::Iterator start) = 0;

 /**
  * Set the Destination AID field value .
  * \param aid the AID of the target destination.
  */
 void SetDestinationAid (uint16_t aid);
 /**
  * Set the Relay AID field value.
  * \param aid the AID of the RDS.
  */
 void SetRelayAid (uint16_t aid);
 /**
  * The Source AID field value.
  * \param aid  is the AID of the initiating STA.
  */
 void SetSourceAid (uint16_t aid);

 uint16_t GetDestinationAid (void) const;
 uint16_t GetRelayAid (void) const;
 uint16_t GetSourceAid (void) const;

protected:
 uint16_t m_destinationAid;
 uint16_t m_relayAid;
 uint16_t m_sourceAid;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type RLS (Relay Link Setup) Request frame.
 */
class ExtRlsRequest : public ExtRlsFrame
{
public:
  ExtRlsRequest ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to a nonzero value chosen by the STA sending the RLS Request frame to
   * identify the request/response transaction.
   * \param token
   */
  void SetDialogToken (uint8_t token);
  /**
   * The Destination Capability Information field indicates the Relay capabilities info field within the Relay
   * capabilities element of the target destination REDS as defined in 8.4.2.150.
   * \param elem
   */
  void SetDestinationCapabilityInformation (RelayCapabilitiesInfo &elem);
  /**
   * The Relay Capability Information field indicates the Relay capabilities info field within the Relay
   * capabilities element of the selected RDS as defined in 8.4.2.150.
   * \param elem
   */
  void SetRelayCapabilityInformation (RelayCapabilitiesInfo &elem);
  /**
   * The Source Capability Information field indicates the Relay capabilities info field within the Relay
   * capabilities element of the originator of the request as defined in 8.4.2.150.
   * \param elem
   */
  void SetSourceCapabilityInformation (RelayCapabilitiesInfo &elem);
  /**
   * The Relay Transfer Parameter Set element is defined in 8.4.2.151.
   * \param elem
   */
  void SetRelayTransferParameterSet (Ptr<RelayTransferParameterSetElement> elem);

  uint8_t GetDialogToken (void) const;
  RelayCapabilitiesInfo GetDestinationCapabilityInformation (void) const;
  RelayCapabilitiesInfo GetRelayCapabilityInformation (void) const;
  RelayCapabilitiesInfo GetSourceCapabilityInformation (void) const;
  Ptr<RelayTransferParameterSetElement> GetRelayTransferParameterSet (void) const;

private:
  uint8_t m_dialogToken;
  RelayCapabilitiesInfo m_destinationCapability;
  RelayCapabilitiesInfo m_relayCapability;
  RelayCapabilitiesInfo m_sourceCapability;
  Ptr<RelayTransferParameterSetElement> m_relayParameter;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type RLS (Relay Link Setup) Response frame.
 */
class ExtRlsResponse : public Header
{
public:
  ExtRlsResponse ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to a nonzero value chosen by the STA sending the RLS Request frame to
   * identify the request/response transaction.
   * \param token
   */
  void SetDialogToken (uint8_t token);
  /**
   * The Destination Status Code field is included when the destination REDS transmits this RLS Response
   * frame as a result of RLS Request. It is defined in 8.4.1.9.
   * \param aid the AID of the target destination.
   */
  void SetDestinationStatusCode (uint16_t status);
  /**
   * The Relay Status Code field is included when the relay REDS transmits this RLS Response frame as a result
   * of RLS Request. It is defined in 8.4.1.9.
   * \param aid the AID of the selected RDS.
   */
  void SetRelayStatusCode (uint16_t status);

  uint8_t GetDialogToken (void) const;
  uint16_t GetDestinationStatusCode (void) const;
  uint16_t GetRelayStatusCode (void) const;

private:
  uint8_t m_dialogToken;
  uint16_t m_destinationStatusCode;
  uint16_t m_relayStatusCode;
  bool m_insertRelayStatus;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type RLS (Relay Link Setup) Announcment frame.
* The RLS Announcement frame is sent to announce the successful RLS.
*/
class ExtRlsAnnouncment : public ExtRlsFrame
{
public:
 ExtRlsAnnouncment ();

 /**
  * Register this type.
  * \return The TypeId.
  */
 static TypeId GetTypeId (void);
 // Inherited
 virtual TypeId GetInstanceTypeId (void) const;
 virtual void Print (std::ostream &os) const;
 virtual uint32_t GetSerializedSize (void) const;
 void Serialize (Buffer::Iterator start) const;
 uint32_t Deserialize (Buffer::Iterator start);

 /**
  * Set the status code on the RLS.
  * \param status the status of the RLS
  */
 void SetStatusCode (uint16_t status);

 uint16_t GetStatusCode (void) const;

private:
 uint16_t m_status;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type RLS (Relay Link Setup) Teardown frame.
* The RLS Teardown frame is sent to terminate a relay operation.
*/
class ExtRlsTearDown : public ExtRlsFrame
{
public:
 ExtRlsTearDown ();

 /**
  * Register this type.
  * \return The TypeId.
  */
 static TypeId GetTypeId (void);
 // Inherited
 virtual TypeId GetInstanceTypeId (void) const;
 virtual void Print (std::ostream &os) const;
 virtual uint32_t GetSerializedSize (void) const;
 void Serialize (Buffer::Iterator start) const;
 uint32_t Deserialize (Buffer::Iterator start);

 /**
  * Set the reason code.
  * \param reason the reason of the RLS teardown
  */
 void SetReasonCode (uint16_t reason);

 uint16_t GetReasonCode (void) const;

private:
 uint16_t m_reasonCode;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Relay ACK Request frame format (8.5.20.18).
 * The Relay ACK Request frame is sent by a source REDS to an RDS participating in a relay operation in
 * order to determine whether all frames forwarded through the RDS were successfully received by the
 * destination REDS also participating in the relay operation. This frame is used only when the RDS is
 * operated in HD-DF mode and relay operation is link switching type.
 */
class ExtRelayAckRequest : public CtrlBAckRequestHeader
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Relay ACK Request frame format (8.5.20.19).
 * The Relay ACK Response frame is sent by an RDS to a source REDS participating in a relay operation in
 * order to report which frames have been received by the destination REDS also participating in the relay
 * operation. This frame is used only when the RDS is operated in HD-DF mode and relay operation is link
 * switching type.
 */
class ExtRelayAckResponse : public CtrlBAckResponseHeader
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type TPA (Transmission Time-Point Adjustment) Request frame.
 * The TPA Request frame is sent by a destination REDS participating in operation based on link cooperating
 * type to both the RDS and source REDS that belong to the same group as the destination REDS in order for
 * them to send back their own TPA Response frames at the separately pre-defined times. Also, a source REDS
 * sends a TPA Request frame to the RDS that is selected by the source REDS in order for the RDS to feedback
 * its own TPA Response frame at the pre-defined time.
 */
class ExtTpaRequest : public Header
{
public:
  ExtTpaRequest ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * The Dialog Token field is set to a nonzero value chosen by the STA sending the TPA Request frame to
   * identify the request/response transaction.
   * \param token
   */
  void SetDialogToken (uint8_t token);
  /**
   * The Timing Offset is 2 octets in length and indicates the amount of time, in nanoseconds, that the STA
   * identified in the RA of the frame is required to change the timing offset of its transmissions so that they
   * arrive at the expected time at the transmitting STA.
   */
  void SetTimingOffset (uint16_t offset);
  /**
   * The Sampling Frequency Offset is 2 octets in length and indicates the amount by which to change the
   * sampling frequency offset of the burst transmission so that bursts arrive at the destination DMG STA with
   * no sampling frequency offset. The unit is 0.01 ppm. The Sampling Frequency Offset field is reserved when
   * set to 0.
   * \param offset the smapling frequency offset
   */
  void SetSamplingFrequencyOffset (uint16_t offset);

  uint8_t GetDialogToken (void) const;
  uint16_t GetTimingOffset (void) const;
  uint16_t GetSamplingFrequencyOffset (void) const;

private:
  uint8_t m_dialogToken;
  uint16_t m_timingOffset;
  uint16_t m_samplingOffset;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type FST (Fast Session Transfer) Setup frame.
*/
class ExtFstSetupFrame : public Header
{
public:
  ExtFstSetupFrame ();

  // Inherited
  virtual TypeId GetInstanceTypeId (void) const = 0;
  virtual void Print (std::ostream &os) const = 0;
  virtual uint32_t GetSerializedSize (void) const = 0;
  virtual void Serialize (Buffer::Iterator start) const = 0;
  virtual uint32_t Deserialize (Buffer::Iterator start) = 0;

  /**
  * The Dialog Token field is set to a nonzero value chosen by the STA sending the TPA Request frame to
  * identify the request/response transaction.
  * \param token
  */
  void SetDialogToken (uint8_t token);
  /**
  * Set the Session Transition field defined in 8.4.2.147.
  * \param elem the sessio transition element
  */
  void SetSessionTransition (SessionTransitionElement &elem);
  /**
  * Set the Multi-band field contains the Multi-Band element as defined in 8.4.2.140. The regulatory information
  * contained in the Multi-band element is applicable to all the fields and elements contained in the frame.
  * \param elem the multi-band element
  */
  void SetMultiBand (Ptr<MultiBandElement> elem);
  void SetWakeupSchedule (Ptr<WakeupScheduleElement> elem);
  void SetAwakeWindow (Ptr<AwakeWindowElement> elem);
  void SetSwitchingStream (Ptr<SwitchingStreamElement> elem);

  uint8_t GetDialogToken (void) const;
  SessionTransitionElement GetSessionTransition (void) const;
  Ptr<MultiBandElement> GetMultiBand (void) const;
  Ptr<WakeupScheduleElement> GetWakeupSchedule (void) const;
  Ptr<AwakeWindowElement> GetAwakeWindow (void) const;
  Ptr<SwitchingStreamElement> GetSwitchingStream (void) const;

protected:
  uint8_t m_dialogToken;
  SessionTransitionElement m_sessionTransition;
  Ptr<MultiBandElement> m_multiBand;
  Ptr<WakeupScheduleElement> m_wakeupSchedule;
  Ptr<AwakeWindowElement> m_awakeWindow;
  Ptr<SwitchingStreamElement> m_switchingStream;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type FST (Fast Session Transfer) Setup Request frame.
* The FST Setup Request frame is an Action frame of category FST. The FST Setup Request frame allows an
* initiating STA to announce to a peer STA whether the initiating STA intends to enable FST for the session
* between the initiating STA and the peer STA (10.32).
*/
class ExtFstSetupRequest : public ExtFstSetupFrame
{
public:
  ExtFstSetupRequest ();

  /**
  * Register this type.
  * \return The TypeId.
  */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
  * The Link loss timeout (LLT) field is 32 bits and indicates the compressed maximum duration counted from
  * the last time an MPDU was received by the initiating STA from the peer STA until the initiating STA
  * decides to initiate FST.
  * \param llt
  */
  void SetLlt (uint32_t llt);

  uint32_t GetLlt (void) const;

private:
  uint32_t m_llt;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type FST (Fast Session Transfer) Setup Response frame.
* The FST Setup Response frame is an Action frame of category FST. This frame is transmitted in response to
* the reception of an FST Setup Request frame.
*/
class ExtFstSetupResponse : public ExtFstSetupFrame
{
public:
  ExtFstSetupResponse ();

  /**
  * Register this type.
  * \return The TypeId.
  */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
  * \param status
  */
  void SetStatusCode (uint16_t status);
  /**
  * \return Status of the FST Setup
  */
  uint16_t GetStatusCode (void) const;

private:
  uint16_t m_statusCode;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type FST (Fast Session Transfer) Tear Down frame.
* This frame is transmitted to delete an established FST session between STAs.
*/
class ExtFstTearDown : public Header
{
public:
  ExtFstTearDown ();

  /**
  * Register this type.
  * \return The TypeId.
  */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
  * The FSTS ID field contains the identification of the FST session established between the STAs identified by
  * the TA and RA fields of this frame
  * \param token
  */
  void SetFstsID (uint32_t id);

  uint32_t GetFstsID (void) const;

private:
  uint32_t m_fstsID;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type FST (Fast Session Transfer) ACK Request frame.
* This frame is transmitted in the frequency band an FST session is transferred to and confirms
* the FST session transfer
*/
class ExtFstAckRequest : public Header
{
public:
  ExtFstAckRequest ();

  /**
  * Register this type.
  * \return The TypeId.
  */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
  * The Dialog Token field is set to a nonzero value chosen by the STA sending the FST ACK Request frame to
  * identify the request/response transaction.
  * \param token
  */
  void SetDialogToken (uint8_t token);
  /**
  * The FSTS ID field contains the identification of the FST session established between the STAs identified by
  * the TA and RA fields of this frame
  * \param token
  */
  void SetFstsID (uint32_t id);

  uint8_t GetDialogToken (void) const;
  uint32_t GetFstsID (void) const;

private:
  uint8_t m_dialogToken;
  uint32_t m_fstsID;

};

/**
* \ingroup wifi
* Implement the header for extension frames of type FST (Fast Session Transfer) ACK Request frame.
* The FST Ack Response frame is an Action frame of category FST. This frame is transmitted in response to
* the reception of an FST Ack Request frame
*/
class ExtFstAckResponse : public ExtFstAckRequest
{
public:
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Unprotected DMG Announce frame (8.5.22.2).
 * The Announce frame is an Action or an Action No Ack frame of category Unprotected DMG. Announce frames
 * can be transmitted during the ATI of a beacon interval and can perform functions including of a DMG
 * Beacon frame, but since this frame does not have to be transmitted as a sector sweep to a STA, it can
 * provide much more spectrally efficient access than using a DMG Beacon frame.
 */
class ExtAnnounceFrame : public Header, public MgtFrame
{
public:
  ExtAnnounceFrame ();

  /**
  * Register this type.
  * \return The TypeId.
  */
  static TypeId GetTypeId (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the Timestamp for this frame.
   * \param timestamp The Timestamp for this frame
   */
  void SetTimestamp (uint8_t timestamp);
  /**
   * Set the Beacon Interval.
   * \param interval the Beacon Interval.
   */
  void SetBeaconInterval (uint16_t interval);

  uint8_t GetTimestamp (void) const;
  uint16_t GetBeaconInterval (void) const;

protected:
  uint8_t m_timestamp;
  uint16_t m_beaconInterval;

};

/**
 * \ingroup wifi
 * Implement the header for extension frames of type Unprotected DMG BRP Frame (8.5.22.3).
 * The BRP frame is an Action No Ack frame.
 */
class ExtBrpFrame : public Header
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  ExtBrpFrame (void);
  // Inherited
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDialogToken (uint8_t token);
  void SetBrpRequestField (BRP_Request_Field &field);
  void SetBeamRefinementElement (BeamRefinementElement &element);
  void AddChannelMeasurementFeedback (ChannelMeasurementFeedbackElement *element);

  uint8_t GetDialogToken (void) const;
  BRP_Request_Field GetBrpRequestField (void) const;
  BeamRefinementElement GetBeamRefinementElement (void) const;
  ChannelMeasurementFeedbackElementList GetChannelMeasurementFeedbackList (void) const;

private:
  uint8_t m_dialogToken;
  BRP_Request_Field m_brpRequestField;
  BeamRefinementElement m_element;
  ChannelMeasurementFeedbackElementList m_list;

};

} // namespace ns3
