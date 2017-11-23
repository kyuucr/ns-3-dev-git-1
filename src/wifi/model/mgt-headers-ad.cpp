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

#include "mgt-headers-ad.h"
#include "ns3/log.h"

/***************************************************
*               Add TS Request Frame
****************************************************/

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DmgAddTSRequestFrame);

DmgAddTSRequestFrame::DmgAddTSRequestFrame ()
  : m_dialogToken (1)
{
}

TypeId
DmgAddTSRequestFrame::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgAddTSRequestFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgAddTSRequestFrame> ()
  ;
  return tid;
}

TypeId
DmgAddTSRequestFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DmgAddTSRequestFrame::Print (std::ostream &os) const
{
}

uint32_t
DmgAddTSRequestFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1;                                      //Dialog token
  size += m_dmgTspecElement.GetSerializedSize (); //DMG TSPEC
  return size;
}

void
DmgAddTSRequestFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i = m_dmgTspecElement.Serialize (i);
}

uint32_t
DmgAddTSRequestFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  i = m_dmgTspecElement.Deserialize (i);
  return i.GetDistanceFrom (start);
}

void
DmgAddTSRequestFrame::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
DmgAddTSRequestFrame::SetDmgTspecElement (DmgTspecElement &element)
{
  m_dmgTspecElement = element;
}

uint8_t
DmgAddTSRequestFrame::GetDialogToken (void) const
{
  return m_dialogToken;
}

DmgTspecElement
DmgAddTSRequestFrame::GetDmgTspec (void) const
{
  return m_dmgTspecElement;
}

/***************************************************
*               Add TS Response Frame
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (DmgAddTSResponseFrame);

DmgAddTSResponseFrame::DmgAddTSResponseFrame ()
  : m_dialogToken (1)
{
}

TypeId
DmgAddTSResponseFrame::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgAddTSResponseFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgAddTSResponseFrame> ()
  ;
  return tid;
}

TypeId
DmgAddTSResponseFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DmgAddTSResponseFrame::Print (std::ostream &os) const
{
}

uint32_t
DmgAddTSResponseFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1;                                      //Dialog token
  size += m_status.GetSerializedSize ();          //Status Code
  size += m_tsDelayElement.GetSerializedSize ();  //TS Delay
  size += m_dmgTspecElement.GetSerializedSize (); //DMG TSPEC
  return size;
}

void
DmgAddTSResponseFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i = m_status.Serialize (i);
  i = m_tsDelayElement.Serialize (i);
  i = m_dmgTspecElement.Serialize (i);
}

uint32_t
DmgAddTSResponseFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  i = m_status.Deserialize (i);
  i = m_tsDelayElement.Deserialize (i);
  i = m_dmgTspecElement.Deserialize (i);
  return i.GetDistanceFrom (start);
}

void
DmgAddTSResponseFrame::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
DmgAddTSResponseFrame::SetStatusCode (StatusCode status)
{
  m_status = status;
}

void
DmgAddTSResponseFrame::SetTsDelay (TsDelayElement &element)
{
  m_tsDelayElement = element;
}

void
DmgAddTSResponseFrame::SetDmgTspecElement (DmgTspecElement &element)
{
  m_dmgTspecElement = element;
}

uint8_t
DmgAddTSResponseFrame::GetDialogToken (void) const
{
  return m_dialogToken;
}

StatusCode
DmgAddTSResponseFrame::GetStatusCode (void) const
{
  return m_status;
}

TsDelayElement
DmgAddTSResponseFrame::GetTsDelay (void) const
{
  return m_tsDelayElement;
}

DmgTspecElement
DmgAddTSResponseFrame::GetDmgTspec (void) const
{
  return m_dmgTspecElement;
}

/***************************************************
*               Delete TS Frame (8.5.3.4)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (DelTsFrame);

DelTsFrame::DelTsFrame ()
  : m_reasonCode (0)
{
}

TypeId
DelTsFrame::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DelTsFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DelTsFrame> ()
  ;
  return tid;
}

TypeId
DelTsFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
DelTsFrame::Print (std::ostream &os) const
{
}

uint32_t
DelTsFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 3;                                        //TS Info
  size += 2;                                        //Reason Code
  size += m_dmgAllocationInfo.GetSerializedSize (); //DMG Allocation Info
  return size;
}

void
DelTsFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.Write (m_tsInfo, 3);
  i.WriteHtolsbU16 (m_reasonCode);
  i = m_dmgAllocationInfo.Serialize (i);
}

uint32_t
DelTsFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i.Read (m_tsInfo, 3);
  m_reasonCode = i.ReadLsbtohU16 ();
  i = m_dmgAllocationInfo.Deserialize (i);
  return i.GetDistanceFrom (start);
}

void
DelTsFrame::SetReasonCode (uint16_t reason)
{
  m_reasonCode = reason;
}

void
DelTsFrame::SetDmgAllocationInfo (DmgAllocationInfo info)
{
  m_dmgAllocationInfo = info;
}

uint16_t
DelTsFrame::GetReasonCode (void) const
{
  return m_reasonCode;
}

DmgAllocationInfo
DelTsFrame::GetDmgAllocationInfo (void) const
{
  return m_dmgAllocationInfo;
}

/***************************************************
*      Radio Measurement Request Frame (8.5.7.2)
****************************************************/

RadioMeasurementRequest::RadioMeasurementRequest ()
  : m_dialogToken (0),
    m_numOfRepetitions (0)
{
}

TypeId
RadioMeasurementRequest::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RadioMeasurementRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<RadioMeasurementRequest> ()
  ;
  return tid;
}

TypeId
RadioMeasurementRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
RadioMeasurementRequest::Print (std::ostream &os) const
{

}

uint32_t
RadioMeasurementRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 3;
  for (WifiInfoElementList::const_iterator iter = m_list.begin (); iter != m_list.end (); iter++)
    {
      size += (*iter)->GetSerializedSize ();
    }
  return size;
}

void
RadioMeasurementRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_numOfRepetitions);
  for (WifiInfoElementList::const_iterator iter = m_list.begin (); iter != m_list.end (); iter++)
    {
      i = (*iter)->Serialize (i);
    }
}

uint32_t
RadioMeasurementRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Ptr<DirectionalChannelQualityRequestElement> element;
  m_dialogToken = i.ReadU8 ();
  m_numOfRepetitions = i.ReadLsbtohU16 ();
  while (!i.IsEnd ())
    {
      element = Create<DirectionalChannelQualityRequestElement> ();
      i = element->Deserialize (i);
      m_list.push_back (element);
  }
  return i.GetDistanceFrom (start);
}

void
RadioMeasurementRequest::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
RadioMeasurementRequest::SetNumberOfRepetitions (uint16_t repetitions)
{
  m_numOfRepetitions = repetitions;
}

void
RadioMeasurementRequest::AddMeasurementRequestElement (Ptr<WifiInformationElement> elem)
{
  m_list.push_back (elem);
}

uint8_t
RadioMeasurementRequest::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint16_t
RadioMeasurementRequest::GetNumberOfRepetitions (void) const
{
  return m_numOfRepetitions;
}

WifiInfoElementList
RadioMeasurementRequest::GetListOfMeasurementRequestElement (void) const
{
  return m_list;
}

/***************************************************
*      Radio Measurement Request Frame (8.5.7.3)
****************************************************/

RadioMeasurementReport::RadioMeasurementReport ()
  : m_dialogToken (0)
{
}

TypeId
RadioMeasurementReport::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RadioMeasurementReport")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<RadioMeasurementReport> ()
  ;
  return tid;
}

TypeId
RadioMeasurementReport::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
RadioMeasurementReport::Print (std::ostream &os) const
{

}

uint32_t
RadioMeasurementReport::GetSerializedSize (void) const
{
  uint32_t size = 1;
  for (WifiInfoElementList::const_iterator iter = m_list.begin (); iter != m_list.end (); iter++)
    {
      size += (*iter)->GetSerializedSize ();
    }
  return size;
}

void
RadioMeasurementReport::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  for (WifiInfoElementList::const_iterator iter = m_list.begin (); iter != m_list.end (); iter++)
    {
      i = (*iter)->Serialize (i);
    }
}

uint32_t
RadioMeasurementReport::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Ptr<DirectionalChannelQualityReportElement> element;
  m_dialogToken = i.ReadU8 ();
  while (!i.IsEnd ())
    {
      element = Create<DirectionalChannelQualityReportElement> ();
      i = element->Deserialize (i);
      m_list.push_back (element);
  }
  return i.GetDistanceFrom (start);
}

void
RadioMeasurementReport::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
RadioMeasurementReport::AddMeasurementReportElement (Ptr<WifiInformationElement> elem)
{
  m_list.push_back (elem);
}

uint8_t
RadioMeasurementReport::GetDialogToken (void) const
{
  return m_dialogToken;
}

WifiInfoElementList
RadioMeasurementReport::GetListOfMeasurementReportElement (void) const
{
  return m_list;
}

/***************************************************
*      Link Measurement Request Frame (8.5.7.5)
****************************************************/

LinkMeasurementRequest::LinkMeasurementRequest ()
  : m_dialogToken (0),
    m_transmitPowerUsed (0),
    m_maxTransmitPower (0)
{
}

TypeId
LinkMeasurementRequest::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkMeasurementRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<LinkMeasurementRequest> ()
  ;
  return tid;
}

TypeId
LinkMeasurementRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
LinkMeasurementRequest::Print (std::ostream &os) const
{

}

uint32_t
LinkMeasurementRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 3;
  for (WifiInformationElementMap::const_iterator iter = m_map.begin (); iter != m_map.end (); iter++)
    {
      size += iter->second->GetSerializedSize ();
    }
  return size;
}

void
LinkMeasurementRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteU8 (m_transmitPowerUsed);
  i.WriteU8 (m_maxTransmitPower);
  for (WifiInformationElementMap::const_iterator iter = m_map.begin (); iter != m_map.end (); iter++)
    {
      iter->second->Serialize (i);
    }
}

uint32_t
LinkMeasurementRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Ptr<WifiInformationElement> element;
  uint8_t id, length;
  m_dialogToken = i.ReadU8 ();
  m_transmitPowerUsed = i.ReadU8 ();
  m_maxTransmitPower = i.ReadU8 ();
  while (!i.IsEnd ())
    {
      i = DeserializeElementID (i, id, length);
      switch (id)
        {
          case IE_DMG_LINK_MARGIN:
            {
              element = Create<LinkMarginElement> ();
              break;
            }
          case IE_DMG_LINK_ADAPTATION_ACKNOWLEDGMENT:
            {
              element = Create<LinkAdaptationAcknowledgment> ();
              break;
            }
        }
      i = element->DeserializeElementBody (i, length);
      m_map[id] = element;
  }
  return i.GetDistanceFrom (start);
}

void
LinkMeasurementRequest::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
LinkMeasurementRequest::SetTransmitPowerUsed (uint8_t power)
{
  m_transmitPowerUsed = power;
}

void
LinkMeasurementRequest::SetMaxTransmitPower (uint8_t power)
{
  m_maxTransmitPower = power;
}

void
LinkMeasurementRequest::AddSubElement (Ptr<WifiInformationElement> elem)
{
  m_map[elem->ElementId ()] = elem;
}

uint8_t
LinkMeasurementRequest::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint8_t
LinkMeasurementRequest::GetTransmitPowerUsed (uint8_t power) const
{
  return m_transmitPowerUsed;
}

uint8_t
LinkMeasurementRequest::GetMaxTransmitPower (uint8_t power) const
{
  return m_maxTransmitPower;
}

Ptr<WifiInformationElement>
LinkMeasurementRequest::GetSubElement (WifiInformationElementId id)
{
  return m_map[id];
}

WifiInformationElementMap
LinkMeasurementRequest::GetListOfSubElements (void) const
{
  return m_map;
}

/***************************************************
*      Link Measurement Report Frame (8.5.7.5)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (LinkMeasurementReport);

LinkMeasurementReport::LinkMeasurementReport ()
  : m_dialogToken (0)
{
}

TypeId
LinkMeasurementReport::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkMeasurementReport")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<LinkMeasurementReport> ()
  ;
  return tid;
}

TypeId
LinkMeasurementReport::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
LinkMeasurementReport::Print (std::ostream &os) const
{
  os << "Dialog Token=" << m_dialogToken;
}

uint32_t
LinkMeasurementReport::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 9;
  for (WifiInformationElementMap::const_iterator iter = m_map.begin (); iter != m_map.end (); iter++)
    {
      size += iter->second->GetSerializedSize ();
    }
  return size;
}

void
LinkMeasurementReport::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU32 (m_tpcElement);
  i.WriteU8 (m_receiveAntId);
  i.WriteU8 (m_transmitAntId);
  i.WriteU8 (m_rcpi);
  i.WriteU8 (m_rsni);
  for (WifiInformationElementMap::const_iterator iter = m_map.begin (); iter != m_map.end (); iter++)
    {
      iter->second->Serialize (i);
    }
}

uint32_t
LinkMeasurementReport::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Ptr<WifiInformationElement> element;
  uint8_t id, length;
  m_dialogToken = i.ReadU8 ();
  m_tpcElement = i.ReadLsbtohU32 ();
  m_receiveAntId = i.ReadU8 ();
  m_transmitAntId = i.ReadU8 ();
  m_rcpi = i.ReadU8 ();
  m_rsni = i.ReadU8 ();
  while (!i.IsEnd ())
    {
      i = DeserializeElementID (i, id, length);
      switch (id)
        {
          case IE_DMG_LINK_MARGIN:
            {
              element = Create<LinkMarginElement> ();
              break;
            }
          case IE_DMG_LINK_ADAPTATION_ACKNOWLEDGMENT:
            {
              element = Create<LinkAdaptationAcknowledgment> ();
              break;
            }
        }
      i = element->DeserializeElementBody (i, length);
      m_map[id] = element;
  }
  return i.GetDistanceFrom (start);
}

void
LinkMeasurementReport::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
LinkMeasurementReport::SetTpcReportElement (uint32_t elem)
{
  m_tpcElement = elem;
}

void
LinkMeasurementReport::SetReceiveAntennaId (uint8_t id)
{
  m_receiveAntId = id;
}

void
LinkMeasurementReport::SetTransmitAntennaId (uint8_t id)
{
  m_transmitAntId = id;
}

void
LinkMeasurementReport::SetRCPI (uint8_t value)
{
  m_rcpi = value;
}

void
LinkMeasurementReport::SetRSNI (uint8_t value)
{
  m_rsni = value;
}

void
LinkMeasurementReport::AddSubElement (Ptr<WifiInformationElement> elem)
{
  m_map[elem->ElementId ()] = elem;
}

uint8_t
LinkMeasurementReport::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint32_t
LinkMeasurementReport::GetTpcReportElement (void) const
{
  return m_tpcElement;
}

uint8_t
LinkMeasurementReport::GetReceiveAntennaId (void) const
{
  return m_receiveAntId;
}

uint8_t
LinkMeasurementReport::GetTransmitAntennaId (void) const
{
  return m_transmitAntId;
}

uint8_t
LinkMeasurementReport::GetRCPI (void) const
{
  return m_rcpi;
}

uint8_t
LinkMeasurementReport::GetRSNI (void) const
{
  return m_rsni;
}

Ptr<WifiInformationElement>
LinkMeasurementReport::GetSubElement (WifiInformationElementId id)
{
  return m_map[id];
}

WifiInformationElementMap
LinkMeasurementReport::GetListOfSubElements (void) const
{
  return m_map;
}

/***************************************************
*               Common QAB Frame
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtQabFrame);

void
ExtQabFrame::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Requestor AP Address = " << m_requester
     << ", Responder AP Address = " << m_responder;
}

uint32_t
ExtQabFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;

  size += 1; //Dialog Token
  size += 6; //Requestor AP Address
  size += 6; //Responder AP Address

  return size;
}

void
ExtQabFrame::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtQabFrame::SetRequesterApAddress (Mac48Address address)
{
  m_requester = address;
}

void
ExtQabFrame::SetResponderApAddress (Mac48Address address)
{
  m_responder = address;
}

uint8_t
ExtQabFrame::GetDialogToken (void) const
{
  return m_dialogToken;
}

Mac48Address
ExtQabFrame::GetRequesterApAddress (void) const
{
  return m_requester;
}

Mac48Address
ExtQabFrame::GetResponderApAddress (void) const
{
  return m_responder;
}

/***************************************************
*           QAB Request Frame (8.5.8.25)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtQabRequestFrame);

ExtQabRequestFrame::ExtQabRequestFrame ()
{
  m_dialogToken = 0;
}

TypeId
ExtQabRequestFrame::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtQabRequestFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtQabRequestFrame> ()
  ;
  return tid;
}

void
ExtQabRequestFrame::Print (std::ostream &os) const
{
  ExtQabFrame::Print (os);
  m_element.Print (os);
}

uint32_t
ExtQabRequestFrame::GetSerializedSize (void) const
{
  return ExtQabFrame::GetSerializedSize () + m_element.GetSerializedSize ();
}

TypeId
ExtQabRequestFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtQabRequestFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_dialogToken);
  WriteTo (i, m_requester);
  WriteTo (i, m_responder);
  i = m_element.Serialize (i);
}

uint32_t
ExtQabRequestFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_dialogToken = i.ReadU8 ();
  ReadFrom (i, m_requester);
  ReadFrom (i, m_responder);
  i = m_element.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
ExtQabRequestFrame::SetQuietPeriodRequestElement (QuietPeriodRequestElement &element)
{
  m_element = element;
}

QuietPeriodRequestElement
ExtQabRequestFrame::GetQuietPeriodRequestElement (void) const
{
  return m_element;
}

/***************************************************
*           QAB Response Frame (8.5.8.26)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtQabResponseFrame);

ExtQabResponseFrame::ExtQabResponseFrame ()
{
  m_dialogToken = 0;
}

void
ExtQabResponseFrame::Print (std::ostream &os) const
{
  ExtQabFrame::Print (os);
  m_element.Print (os);
}

uint32_t
ExtQabResponseFrame::GetSerializedSize (void) const
{
  return ExtQabFrame::GetSerializedSize () + m_element.GetSerializedSize ();
}

TypeId
ExtQabResponseFrame::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtQabResponseFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtQabResponseFrame> ()
  ;
  return tid;
}

TypeId
ExtQabResponseFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtQabResponseFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_dialogToken);
  WriteTo (i, m_requester);
  WriteTo (i, m_responder);
  i = m_element.Serialize (i);
}

uint32_t
ExtQabResponseFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_dialogToken = i.ReadU8 ();
  ReadFrom (i, m_requester);
  ReadFrom (i, m_responder);
  i = m_element.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
ExtQabResponseFrame::SetQuietPeriodResponseElement (QuietPeriodResponseElement &element)
{
  m_element = element;
}

QuietPeriodResponseElement
ExtQabResponseFrame::GetQuietPeriodResponseElement (void) const
{
  return m_element;
}

/***************************************************
*           Common Information Frame
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtInformationFrame);

ExtInformationFrame::ExtInformationFrame ()
{
}

void
ExtInformationFrame::Print (std::ostream &os) const
{

}

uint32_t
ExtInformationFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 6; //Subject Address
  size += m_requestElement->GetSerializedSize (); //Request Information
  // DMG Capabilities List
  size += m_dmgCapabilitiesList.size () * 19; /* The whole DMG Capabilities Element Size */
  // Wifi Information Element List (Optional)
  size += GetInformationElementsSerializedSize ();
  return size;
}

void
ExtInformationFrame::SetSubjectAddress (Mac48Address address)
{
  m_subjectAddress = address;
}

void
ExtInformationFrame::SetRequestInformationElement (Ptr<RequestElement> elem)
{
  m_requestElement = elem;
}

void
ExtInformationFrame::AddDmgCapabilitiesElement (Ptr<DmgCapabilities> elem)
{
  m_dmgCapabilitiesList.push_back (elem);
}

Mac48Address
ExtInformationFrame::GetSubjectAddress (void) const
{
  return m_subjectAddress;
}

Ptr<RequestElement>
ExtInformationFrame::GetRequestInformationElement (void) const
{
  return m_requestElement;
}

DmgCapabilitiesList
ExtInformationFrame::GetDmgCapabilitiesList (void) const
{
  return m_dmgCapabilitiesList;
}

/***************************************************
*         Information Request Frame (8.5.20.4)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtInformationRequest);

TypeId
ExtInformationRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtInformationRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtInformationRequest> ()
  ;
  return tid;
}

TypeId
ExtInformationRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtInformationRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  WriteTo (i, m_subjectAddress);
  i = m_requestElement->Serialize (i);
  for (DmgCapabilitiesList::const_iterator j = m_dmgCapabilitiesList.begin (); j != m_dmgCapabilitiesList.end () ; j++)
    {
      i = (*j)->Serialize (i);
    }
  i = SerializeInformationElements (i);
}

uint32_t
ExtInformationRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Buffer::Iterator m;
  Ptr<DmgCapabilities> dmgCapabilities;

  ReadFrom (i, m_subjectAddress);
  m_requestElement = Create<RequestElement> ();
  i = m_requestElement->Deserialize (i);

  /* Deserialize DMG Capabilities Elements */
  while (!i.IsEnd ())
    {
      m = i;
      dmgCapabilities = Create<DmgCapabilities> ();
      i = dmgCapabilities->DeserializeIfPresent (i);
      if (i.GetDistanceFrom (m) != 0)
        {
          m_dmgCapabilitiesList.push_back (dmgCapabilities);
        }
      else
        {
          break;
        }
    }

  /* Deserialize Infomration Elements */
  i = DeserializeInformationElements (i);

  return i.GetDistanceFrom (start);
}

/***************************************************
*         Information Response Frame (8.5.20.5)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtInformationResponse);

TypeId
ExtInformationResponse::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtInformationResponse")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtInformationResponse> ()
  ;
  return tid;
}

TypeId
ExtInformationResponse::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtInformationResponse::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  WriteTo (i, m_subjectAddress);
  for (DmgCapabilitiesList::const_iterator j = m_dmgCapabilitiesList.begin (); j != m_dmgCapabilitiesList.end () ; j++)
    {
      i = (*j)->Serialize (i);
    }
  i = m_requestElement->Serialize (i);
  i = SerializeInformationElements (i);
}

uint32_t
ExtInformationResponse::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Buffer::Iterator m;
  Ptr<DmgCapabilities> dmgCapabilities;

  ReadFrom (i, m_subjectAddress);
  /* Deserialize DMG Capabilities Elements */
  while (!i.IsEnd ())
    {
      m = i;
      dmgCapabilities = Create<DmgCapabilities> ();
      i = dmgCapabilities->DeserializeIfPresent (i);
      if (i.GetDistanceFrom (m) != 0)
        {
          m_dmgCapabilitiesList.push_back (dmgCapabilities);
        }
      else
        {
          break;
        }
    }

  m_requestElement = Create<RequestElement> ();
  i = m_requestElement->Deserialize (i);

  /* Deserialize Infomration Elements */
  i = DeserializeInformationElements (i);

  return i.GetDistanceFrom (start);
}

/***************************************************
*                 Handover Request
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtHandoverRequestHeader);

ExtHandoverRequestHeader::ExtHandoverRequestHeader ()
  : m_handoverReason (LeavingPBSS),
    m_remainingBI (0)
{
}

TypeId
ExtHandoverRequestHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtHandoverRequestHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtHandoverRequestHeader> ()
  ;
  return tid;
}

TypeId
ExtHandoverRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtHandoverRequestHeader::Print (std::ostream &os) const
{
  os << "Handover Reason = " << m_handoverReason
     << ", m_remainingBI = " << m_remainingBI;
}

uint32_t
ExtHandoverRequestHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Handover Reason
  size += 1; //Handover Remaining BI
  return size;
}

void
ExtHandoverRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_handoverReason);
  i.WriteU8 (m_remainingBI);
}

uint32_t
ExtHandoverRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_handoverReason = i.ReadU8 ();
  m_remainingBI = i.ReadU8 ();
  return i.GetDistanceFrom (start);
}

void
ExtHandoverRequestHeader::SetHandoverReason (enum HandoverReason reason)
{
  m_handoverReason = reason;
}

void
ExtHandoverRequestHeader::SetHandoverRemainingBI (uint8_t remaining)
{
  m_remainingBI = remaining;
}

enum HandoverReason
ExtHandoverRequestHeader::GetHandoverReason (void) const
{
  return static_cast<enum HandoverReason> (m_handoverReason);
}

uint8_t
ExtHandoverRequestHeader::GetHandoverRemainingBI (void) const
{
  return m_remainingBI;
}

/***************************************************
*                 Handover Response
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtHandoverResponseHeader);

ExtHandoverResponseHeader::ExtHandoverResponseHeader ()
  : m_handoverResult (1),
    m_handoverRejectReason (LowPower)
{
}

TypeId
ExtHandoverResponseHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtHandoverResponseHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtHandoverResponseHeader> ()
  ;
  return tid;
}

TypeId
ExtHandoverResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtHandoverResponseHeader::Print (std::ostream &os) const
{
  os << "Handover Result = " << m_handoverResult
     << ", Handover Reject Reason = " << m_handoverRejectReason;
}

uint32_t
ExtHandoverResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Handover Result
  size += 1; //Handover Reject Reason
  return size;
}

void
ExtHandoverResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_handoverResult);
  i.WriteU8 (m_handoverRejectReason);
}

uint32_t
ExtHandoverResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_handoverResult = i.ReadU8 ();
  m_handoverRejectReason = i.ReadU8 ();
  return i.GetDistanceFrom (start);
}

void
ExtHandoverResponseHeader::SetHandoverResult (bool result)
{
  m_handoverResult = result;
}

void
ExtHandoverResponseHeader::SetHandoverRejectReason (enum HandoverRejectReason reason)
{
  m_handoverRejectReason = reason;
}

bool
ExtHandoverResponseHeader::GetHandoverResult (void) const
{
  return m_handoverResult;
}

enum HandoverRejectReason
ExtHandoverResponseHeader::GetHandoverRejectReason (void) const
{
  return static_cast<enum HandoverRejectReason> (m_handoverRejectReason);
}

/***************************************************
*               Relay Search Request
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRelaySearchRequestHeader);

ExtRelaySearchRequestHeader::ExtRelaySearchRequestHeader ()
  : m_dialogToken (0),
    m_aid (0)
{
}

TypeId
ExtRelaySearchRequestHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRelaySearchRequestHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRelaySearchRequestHeader> ()
  ;
  return tid;
}

TypeId
ExtRelaySearchRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtRelaySearchRequestHeader::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Destination REDS AID = " << m_aid;
}

uint32_t
ExtRelaySearchRequestHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog Token
  size += 2; //Destination REDS AID
  return size;
}

void
ExtRelaySearchRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_aid);
}

uint32_t
ExtRelaySearchRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  m_aid = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

void
ExtRelaySearchRequestHeader::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtRelaySearchRequestHeader::SetDestinationRedsAid (uint16_t aid)
{
  m_aid = aid;
}

uint8_t
ExtRelaySearchRequestHeader::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint16_t
ExtRelaySearchRequestHeader::GetDestinationRedsAid (void) const
{
  return m_aid;
}

/***************************************************
*               Relay Search Response
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRelaySearchResponseHeader);

ExtRelaySearchResponseHeader::ExtRelaySearchResponseHeader ()
  : m_dialogToken (0),
    m_statusCode (0)
{
}

TypeId
ExtRelaySearchResponseHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRelaySearchResponseHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRelaySearchResponseHeader> ()
  ;
  return tid;
}

TypeId
ExtRelaySearchResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtRelaySearchResponseHeader::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Status Code = " << m_statusCode;
}

uint32_t
ExtRelaySearchResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog Token
  size += 2; //Status Code
  if (m_statusCode == 0)
    {
      size += m_list.size () * 3; //Relay Capable STA Info
    }
  return size;
}

void
ExtRelaySearchResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_statusCode);
  if (m_statusCode == 0)
    {
      for (RelayCapableStaList::const_iterator item = m_list.begin (); item != m_list.end (); item++)
        {
          i.WriteU8 ((item->first & 0xFF));
          i = item->second.Serialize (i);
        }
    }
}

uint32_t
ExtRelaySearchResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  RelayCapabilitiesInfo info;
  uint16_t aid;
  m_dialogToken = i.ReadU8 ();
  m_statusCode = i.ReadLsbtohU16 ();
  if (!i.IsEnd ())
    {
      do
        {
          aid = i.ReadU8 ();
          i = info.Deserialize (i);
          m_list[aid] = info;
        }
      while (!i.IsEnd ());
    }
  return i.GetDistanceFrom (start);
}

void
ExtRelaySearchResponseHeader::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtRelaySearchResponseHeader::SetStatusCode (uint16_t code)
{
  m_statusCode = code;
}

void
ExtRelaySearchResponseHeader::AddRelayCapableStaInfo (uint8_t aid, RelayCapabilitiesInfo &element)
{
  m_list[aid] = element;
}

void
ExtRelaySearchResponseHeader::SetRelayCapableList (RelayCapableStaList &list)
{
  m_list = list;
}

uint8_t
ExtRelaySearchResponseHeader::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint16_t
ExtRelaySearchResponseHeader::GetStatusCode (void) const
{
  return m_statusCode;
}

RelayCapableStaList
ExtRelaySearchResponseHeader::GetRelayCapableList (void) const
{
  return m_list;
}

/***************************************************
*     Multi Relay Channel Measurement Request
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtMultiRelayChannelMeasurementRequest);

ExtMultiRelayChannelMeasurementRequest::ExtMultiRelayChannelMeasurementRequest ()
  : m_dialogToken (0)
{
}

TypeId
ExtMultiRelayChannelMeasurementRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtMultiRelayChannelMeasurementRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtMultiRelayChannelMeasurementRequest> ()
  ;
  return tid;
}

TypeId
ExtMultiRelayChannelMeasurementRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtMultiRelayChannelMeasurementRequest::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken;
}

uint32_t
ExtMultiRelayChannelMeasurementRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog Token
  return size;
}

void
ExtMultiRelayChannelMeasurementRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
}

uint32_t
ExtMultiRelayChannelMeasurementRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  return i.GetDistanceFrom (start);
}

void
ExtMultiRelayChannelMeasurementRequest::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

uint8_t
ExtMultiRelayChannelMeasurementRequest::GetDialogToken (void) const
{
  return m_dialogToken;
}

/***************************************************
*     Multi Relay Channel Measurement Report
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtMultiRelayChannelMeasurementReport);

ExtMultiRelayChannelMeasurementReport::ExtMultiRelayChannelMeasurementReport ()
  : m_dialogToken (0)
{
}

TypeId
ExtMultiRelayChannelMeasurementReport::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtMultiRelayChannelMeasurementReport")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtMultiRelayChannelMeasurementReport> ()
  ;
  return tid;
}

TypeId
ExtMultiRelayChannelMeasurementReport::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtMultiRelayChannelMeasurementReport::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken;
}

uint32_t
ExtMultiRelayChannelMeasurementReport::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1;                  //Dialog Token
  size += m_list.size () * 4; //Channel Measurement Info
  return size;
}

void
ExtMultiRelayChannelMeasurementReport::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  Ptr<ExtChannelMeasurementInfo> info;
  i.WriteU8 (m_dialogToken);
  for (ChannelMeasurementInfoList::const_iterator item = m_list.begin (); item != m_list.end (); item++)
    {
      info = *item;
      i = info->Serialize (i);
    }
}

uint32_t
ExtMultiRelayChannelMeasurementReport::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Ptr<ExtChannelMeasurementInfo> element;
  m_dialogToken = i.ReadU8 ();
  while (!i.IsEnd ())
    {
      element = Create<ExtChannelMeasurementInfo> ();
      i = element->Deserialize (i);
      m_list.push_back (element);
    }
  return i.GetDistanceFrom (start);
}

void
ExtMultiRelayChannelMeasurementReport::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtMultiRelayChannelMeasurementReport::AddChannelMeasurementInfo (Ptr<ExtChannelMeasurementInfo> element)
{
  m_list.push_back (element);
}

void
ExtMultiRelayChannelMeasurementReport::SetChannelMeasurementList (ChannelMeasurementInfoList &list)
{
  m_list = list;
}

uint8_t
ExtMultiRelayChannelMeasurementReport::GetDialogToken (void) const
{
  return m_dialogToken;
}

ChannelMeasurementInfoList
ExtMultiRelayChannelMeasurementReport::GetChannelMeasurementInfoList (void) const
{
  return m_list;
}

/***************************************************
*        Generic Relay Link Setup Frame
****************************************************/

ExtRlsFrame::ExtRlsFrame ()
  : m_destinationAid (0),
    m_relayAid (0),
    m_sourceAid (0)
{
}

uint32_t
ExtRlsFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 2; //Destination AID
  size += 2; //Relay AID
  size += 2; //Source AID
  return size;
}

void
ExtRlsFrame::SetDestinationAid (uint16_t aid)
{
  m_destinationAid = aid;
}

void
ExtRlsFrame::SetRelayAid (uint16_t aid)
{
  m_relayAid = aid;
}

void
ExtRlsFrame::SetSourceAid (uint16_t aid)
{
  m_sourceAid = aid;
}

uint16_t
ExtRlsFrame::GetDestinationAid (void) const
{
  return m_destinationAid;
}

uint16_t
ExtRlsFrame::GetRelayAid (void) const
{
  return m_relayAid;
}

uint16_t
ExtRlsFrame::GetSourceAid (void) const
{
  return m_sourceAid;
}

/***************************************************
*        Relay Link Setup Request 8.5.20.14
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRlsRequest);

ExtRlsRequest::ExtRlsRequest ()
  : m_dialogToken (0)
{
}

TypeId
ExtRlsRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRlsRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRlsRequest> ()
  ;
  return tid;
}

TypeId
ExtRlsRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtRlsRequest::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Destination AID = " << m_destinationAid
     << ", Relay AID = " <<  m_relayAid
     << ", Source AID = " <<  m_sourceAid;
}

uint32_t
ExtRlsRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog Token
  size += ExtRlsFrame::GetSerializedSize ();
  size += m_destinationCapability.GetSerializedSize (); //Destination Capability
  size += m_relayCapability.GetSerializedSize ();       //Relay Capability
  size += m_sourceCapability.GetSerializedSize ();      //Source Capability
  size += m_relayParameter->GetSerializedSize ();       //Relay Transfer
  return size;
}

void
ExtRlsRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_destinationAid);
  i.WriteHtolsbU16 (m_relayAid);
  i.WriteHtolsbU16 (m_sourceAid);

  i = m_destinationCapability.Serialize (i);
  i = m_relayCapability.Serialize (i);
  i = m_sourceCapability.Serialize (i);
  i = m_relayParameter->Serialize (i);
}

uint32_t
ExtRlsRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_dialogToken = i.ReadU8 ();
  m_destinationAid = i.ReadLsbtohU16 ();
  m_relayAid = i.ReadLsbtohU16 ();
  m_sourceAid = i.ReadLsbtohU16 ();

  i = m_destinationCapability.Deserialize (i);
  i = m_relayCapability.Deserialize (i);
  i = m_sourceCapability.Deserialize (i);

  m_relayParameter = Create<RelayTransferParameterSetElement> ();
  i = m_relayParameter->Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
ExtRlsRequest::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtRlsRequest::SetDestinationCapabilityInformation (RelayCapabilitiesInfo &elem)
{
  m_destinationCapability = elem;
}

void
ExtRlsRequest::SetRelayCapabilityInformation (RelayCapabilitiesInfo &elem)
{
  m_relayCapability = elem;
}

void
ExtRlsRequest::SetSourceCapabilityInformation (RelayCapabilitiesInfo &elem)
{
  m_sourceCapability = elem;
}

void
ExtRlsRequest::SetRelayTransferParameterSet (Ptr<RelayTransferParameterSetElement> elem)
{
  m_relayParameter = elem;
}

uint8_t
ExtRlsRequest::GetDialogToken (void) const
{
  return m_dialogToken;
}

RelayCapabilitiesInfo
ExtRlsRequest::GetDestinationCapabilityInformation (void) const
{
  return m_destinationCapability;
}

RelayCapabilitiesInfo
ExtRlsRequest::GetRelayCapabilityInformation (void) const
{
  return m_relayCapability;
}

RelayCapabilitiesInfo
ExtRlsRequest::GetSourceCapabilityInformation (void) const
{
  return m_sourceCapability;
}

Ptr<RelayTransferParameterSetElement>
ExtRlsRequest::GetRelayTransferParameterSet (void) const
{
  return m_relayParameter;
}

/***************************************************
*        Relay Link Setup Response 8.5.20.15
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRlsResponse);

ExtRlsResponse::ExtRlsResponse ()
  : m_dialogToken (0),
    m_destinationStatusCode (0),
    m_relayStatusCode (0),
    m_insertRelayStatus (false)
{
}

TypeId
ExtRlsResponse::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRlsResponse")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRlsResponse> ()
  ;
  return tid;
}

TypeId
ExtRlsResponse::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtRlsResponse::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Destination Status Code = " << m_destinationStatusCode;
  if (m_insertRelayStatus)
     os << ", Relay Status Code = " <<  m_relayStatusCode;
}

uint32_t
ExtRlsResponse::GetSerializedSize (void) const
{
  uint32_t size = 0;

  size += 1; //Dialog Token
  size += 2; //Destination Status Code
  if (m_insertRelayStatus)
    size += 2; //Relay Status Code

  return size;
}

void
ExtRlsResponse::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_destinationStatusCode);

  if (m_insertRelayStatus)
    i.WriteHtolsbU16 (m_relayStatusCode);
}

uint32_t
ExtRlsResponse::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_dialogToken = i.ReadU8 ();
  m_destinationStatusCode = i.ReadLsbtohU16 ();

  if (!i.IsEnd ())
    m_relayStatusCode = i.ReadLsbtohU16 ();

  return i.GetDistanceFrom (start);
}

void
ExtRlsResponse::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtRlsResponse::SetDestinationStatusCode (uint16_t status)
{
  m_destinationStatusCode = status;
}

void
ExtRlsResponse::SetRelayStatusCode (uint16_t status)
{
  m_insertRelayStatus = true;
  m_relayStatusCode = status;
}

uint8_t
ExtRlsResponse::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint16_t
ExtRlsResponse::GetDestinationStatusCode (void) const
{
  return m_destinationStatusCode;
}

uint16_t
ExtRlsResponse::GetRelayStatusCode (void) const
{
  return m_relayStatusCode;
}

/***************************************************
*        Relay Link Setup Announcment 8.5.20.16
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRlsAnnouncment);

ExtRlsAnnouncment::ExtRlsAnnouncment ()
  : m_status (0)
{
}

TypeId
ExtRlsAnnouncment::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRlsAnnouncment")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRlsAnnouncment> ()
  ;
  return tid;
}

TypeId
ExtRlsAnnouncment::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtRlsAnnouncment::Print (std::ostream &os) const
{
  os << "Status Code = " << m_status
     << ", Destination AID = " << m_destinationAid
     << ", Relay AID = " <<  m_relayAid
     << ", Source AID = " <<  m_sourceAid;
}

uint32_t
ExtRlsAnnouncment::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 2; //Status Code
  size += ExtRlsFrame::GetSerializedSize ();
  return size;
}

void
ExtRlsAnnouncment::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (m_status);
  i.WriteHtolsbU16 (m_destinationAid);
  i.WriteHtolsbU16 (m_relayAid);
  i.WriteHtolsbU16 (m_sourceAid);
}

uint32_t
ExtRlsAnnouncment::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_status = i.ReadLsbtohU16 ();
  m_destinationAid = i.ReadLsbtohU16 ();
  m_relayAid = i.ReadLsbtohU16 ();
  m_sourceAid = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

void
ExtRlsAnnouncment::SetStatusCode (uint16_t status)
{
  m_status = status;
}

uint16_t
ExtRlsAnnouncment::GetStatusCode (void) const
{
  return m_status;
}

/***************************************************
*        Relay Link Setup Teardown 8.5.20.17
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRlsTearDown);

ExtRlsTearDown::ExtRlsTearDown ()
  : m_reasonCode (0)
{
}

TypeId
ExtRlsTearDown::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRlsTearDown")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRlsTearDown> ()
  ;
  return tid;
}

TypeId
ExtRlsTearDown::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtRlsTearDown::Print (std::ostream &os) const
{
  os << "Destination AID = " << m_destinationAid
     << ", Relay AID = " <<  m_relayAid
     << ", Source AID = " <<  m_sourceAid
     << ", Reason Code = " << m_reasonCode;
}

uint32_t
ExtRlsTearDown::GetSerializedSize (void) const
{
  uint32_t size = 0;

  size += ExtRlsFrame::GetSerializedSize ();
  size += 2; //Reason Code

  return size;
}

void
ExtRlsTearDown::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtolsbU16 (m_destinationAid);
  i.WriteHtolsbU16 (m_relayAid);
  i.WriteHtolsbU16 (m_sourceAid);
  i.WriteHtolsbU16 (m_reasonCode);
}

uint32_t
ExtRlsTearDown::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_destinationAid = i.ReadLsbtohU16 ();
  m_relayAid = i.ReadLsbtohU16 ();
  m_sourceAid = i.ReadLsbtohU16 ();
  m_reasonCode = i.ReadLsbtohU16 ();

  return i.GetDistanceFrom (start);
}

void
ExtRlsTearDown::SetReasonCode (uint16_t reason)
{
  m_reasonCode = reason;
}

uint16_t
ExtRlsTearDown::GetReasonCode (void) const
{
  return m_reasonCode;
}

/***************************************************
*          Relay Ack Request 8.5.20.18
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRelayAckRequest);

TypeId
ExtRelayAckRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRelayAckRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRelayAckRequest> ()
  ;
  return tid;
}

TypeId
ExtRelayAckRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

/***************************************************
*          Relay Ack Response 8.5.20.19
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtRelayAckResponse);

TypeId
ExtRelayAckResponse::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtRelayAckResponse")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtRelayAckResponse> ()
  ;
  return tid;
}

TypeId
ExtRelayAckResponse::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

/*************************************************************
* Transmission Time-Point Adjustment Request frame 8.5.20.20
**************************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtTpaRequest);

ExtTpaRequest::ExtTpaRequest ()
  : m_dialogToken (0),
    m_timingOffset (0),
    m_samplingOffset (0)
{
}

TypeId
ExtTpaRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtTpaRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtTpaRequest> ()
  ;
  return tid;
}

TypeId
ExtTpaRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtTpaRequest::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Timing Offset = " << m_timingOffset
     << ", Sampling Frequency Offset = " <<  m_samplingOffset;
}

uint32_t
ExtTpaRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;

  size += 1; //Dialog Token
  size += 2; //Timing Offset
  size += 2; //Sampling Frequency Offset

  return size;
}

void
ExtTpaRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_timingOffset);
  i.WriteHtolsbU16 (m_samplingOffset);
}

uint32_t
ExtTpaRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_dialogToken = i.ReadU8 ();
  m_timingOffset = i.ReadLsbtohU16 ();
  m_samplingOffset = i.ReadLsbtohU16 ();

  return i.GetDistanceFrom (start);
}

void
ExtTpaRequest::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtTpaRequest::SetTimingOffset (uint16_t offset)
{
  m_timingOffset = offset;
}

void
ExtTpaRequest::SetSamplingFrequencyOffset (uint16_t offset)
{
  m_samplingOffset = offset;
}

uint8_t
ExtTpaRequest::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint16_t
ExtTpaRequest::GetTimingOffset (void) const
{
  return m_timingOffset;
}

uint16_t
ExtTpaRequest::GetSamplingFrequencyOffset (void) const
{
  return m_samplingOffset;
}

/*************************************************************
*            Fast Session Transfer Setup Frame
**************************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtFstSetupFrame);

ExtFstSetupFrame::ExtFstSetupFrame ()
{
}

void
ExtFstSetupFrame::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtFstSetupFrame::SetSessionTransition (SessionTransitionElement &elem)
{
  m_sessionTransition = elem;
}

void
ExtFstSetupFrame::SetMultiBand (Ptr<MultiBandElement> elem)
{
  m_multiBand = elem;
}

void
ExtFstSetupFrame::SetWakeupSchedule (Ptr<WakeupScheduleElement> elem)
{
  m_wakeupSchedule = elem;
}

void
ExtFstSetupFrame::SetAwakeWindow (Ptr<AwakeWindowElement> elem)
{
  m_awakeWindow = elem;
}

void
ExtFstSetupFrame::SetSwitchingStream (Ptr<SwitchingStreamElement> elem)
{
  m_switchingStream = elem;
}

uint8_t
ExtFstSetupFrame::GetDialogToken (void) const
{
  return m_dialogToken;
}

SessionTransitionElement
ExtFstSetupFrame::GetSessionTransition (void) const
{
  return m_sessionTransition;
}

Ptr<MultiBandElement>
ExtFstSetupFrame::GetMultiBand (void) const
{
  return m_multiBand;
}

Ptr<WakeupScheduleElement>
ExtFstSetupFrame::GetWakeupSchedule (void) const
{
  return m_wakeupSchedule;
}

Ptr<AwakeWindowElement>
ExtFstSetupFrame::GetAwakeWindow (void) const
{
  return m_awakeWindow;
}

Ptr<SwitchingStreamElement>
ExtFstSetupFrame::GetSwitchingStream (void) const
{
  return m_switchingStream;
}

/*************************************************************
*       Fast Session Transfer Request 8.5.21.2
**************************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtFstSetupRequest);

ExtFstSetupRequest::ExtFstSetupRequest ()
{
  m_dialogToken = 0;
  m_llt = 0;
}

TypeId
ExtFstSetupRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtFstSetupRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtFstSetupRequest> ()
  ;
  return tid;
}

TypeId
ExtFstSetupRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtFstSetupRequest::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", LLT = " << m_llt;
}

uint32_t
ExtFstSetupRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;

  size += 1; //Dialog Token
  size += 4; //LLT
  size += m_sessionTransition.GetSerializedSize ();
  if (m_multiBand != 0)
    size += m_multiBand->GetSerializedSize ();
  if (m_wakeupSchedule != 0)
    size += m_wakeupSchedule->GetSerializedSize ();
  if (m_awakeWindow != 0)
    size += m_awakeWindow->GetSerializedSize ();
  if (m_switchingStream != 0)
    size += m_switchingStream->GetSerializedSize ();

  return size;
}

void
ExtFstSetupRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU32 (m_llt);
  i = m_sessionTransition.Serialize (i);
  if (m_multiBand != 0)
    {
      i = m_multiBand->Serialize (i);
    }
  if (m_wakeupSchedule != 0)
    {
      i = m_wakeupSchedule->Serialize (i);
    }
  if (m_awakeWindow != 0)
    {
      i = m_awakeWindow->Serialize (i);
    }
  if (m_switchingStream != 0)
    {
      i = m_switchingStream->Serialize (i);
    }
}

uint32_t
ExtFstSetupRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  m_llt = i.ReadLsbtohU32 ();
  i = m_sessionTransition.Deserialize (i);
  if (m_multiBand != 0)
    {
      i = m_multiBand->DeserializeIfPresent (i);
    }
  if (m_wakeupSchedule != 0)
    {
      i = m_wakeupSchedule->DeserializeIfPresent (i);
    }
  if (m_awakeWindow != 0)
    {
      i = m_awakeWindow->DeserializeIfPresent (i);
    }
  if (m_switchingStream != 0)
    {
      i = m_switchingStream->DeserializeIfPresent (i);
    }

  return i.GetDistanceFrom (start);
}

void
ExtFstSetupRequest::SetLlt (uint32_t llt)
{
  m_llt = llt;
}

uint32_t
ExtFstSetupRequest::GetLlt (void) const
{
  return m_llt;
}

/*************************************************************
*          Fast Session Transfer Response 8.5.21.3
**************************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtFstSetupResponse);

ExtFstSetupResponse::ExtFstSetupResponse ()
{
  m_dialogToken = 0;
  m_statusCode = 0;
}

TypeId
ExtFstSetupResponse::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtFstSetupResponse")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtFstSetupResponse> ()
  ;
  return tid;
}

TypeId
ExtFstSetupResponse::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtFstSetupResponse::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", Status Code = " << m_statusCode;
}

uint32_t
ExtFstSetupResponse::GetSerializedSize (void) const
{
  uint32_t size = 0;

  size += 1; //Dialog Token
  size += 2; //Status Code
  size += m_sessionTransition.GetSerializedSize ();
  if (m_multiBand != 0)
    size += m_multiBand->GetSerializedSize ();
  if (m_wakeupSchedule != 0)
    size += m_wakeupSchedule->GetSerializedSize ();
  if (m_awakeWindow != 0)
    size += m_awakeWindow->GetSerializedSize ();
  if (m_switchingStream != 0)
    size += m_switchingStream->GetSerializedSize ();

  return size;
}

void
ExtFstSetupResponse::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU16 (m_statusCode);
  i = m_sessionTransition.Serialize (i);
  if (m_multiBand != 0)
    i = m_multiBand->Serialize (i);
  if (m_wakeupSchedule != 0)
    i = m_wakeupSchedule->Serialize (i);
  if (m_awakeWindow != 0)
    i = m_awakeWindow->Serialize (i);
  if (m_switchingStream != 0)
    i = m_switchingStream->Serialize (i);
}

uint32_t
ExtFstSetupResponse::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_dialogToken = i.ReadU8 ();
  m_statusCode = i.ReadLsbtohU16 ();
  i = m_sessionTransition.Deserialize (i);
  if (m_multiBand != 0)
    {
      i = m_multiBand->DeserializeIfPresent (i);
    }
  if (m_wakeupSchedule != 0)
    {
      i = m_wakeupSchedule->DeserializeIfPresent (i);
    }
  if (m_awakeWindow != 0)
    {
      i = m_awakeWindow->DeserializeIfPresent (i);
    }
  if (m_switchingStream != 0)
    {
      i = m_switchingStream->DeserializeIfPresent (i);
    }

  return i.GetDistanceFrom (start);
}

void
ExtFstSetupResponse::SetStatusCode (uint16_t status)
{
  m_statusCode = status;
}

uint16_t
ExtFstSetupResponse::GetStatusCode (void) const
{
  return m_statusCode;
}

/***************************************************
*     Fast Session Transfer Tear Down 8.5.21.4
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtFstTearDown);

ExtFstTearDown::ExtFstTearDown ()
  : m_fstsID (0)
{
}

TypeId
ExtFstTearDown::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtFstTearDown")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtFstTearDown> ()
  ;
  return tid;
}

TypeId
ExtFstTearDown::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtFstTearDown::Print (std::ostream &os) const
{
  os << "FSTS ID = " << m_fstsID;
}

uint32_t
ExtFstTearDown::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 4; //FSTS ID
  return size;
}

void
ExtFstTearDown::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU32 (m_fstsID);
}

uint32_t
ExtFstTearDown::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_fstsID = i.ReadLsbtohU32 ();
  return i.GetDistanceFrom (start);
}

void
ExtFstTearDown::SetFstsID (uint32_t id)
{
  m_fstsID = id;
}

uint32_t
ExtFstTearDown::GetFstsID (void) const
{
  return m_fstsID;
}

/***************************************************
*     Fast Session Transfer Ack Request 8.5.21.5
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtFstAckRequest);

ExtFstAckRequest::ExtFstAckRequest ()
  : m_dialogToken (0),
    m_fstsID (0)
{
}

TypeId
ExtFstAckRequest::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtFstAckRequest")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtFstAckRequest> ()
  ;
  return tid;
}

TypeId
ExtFstAckRequest::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtFstAckRequest::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken
     << ", FSTS ID = " << m_fstsID;
}

uint32_t
ExtFstAckRequest::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog Token
  size += 4; //FSTS ID
  return size;
}

void
ExtFstAckRequest::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i.WriteHtolsbU32 (m_fstsID);
}

uint32_t
ExtFstAckRequest::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_dialogToken = i.ReadU8 ();
  m_fstsID = i.ReadLsbtohU32 ();
  return i.GetDistanceFrom (start);
}

void
ExtFstAckRequest::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtFstAckRequest::SetFstsID (uint32_t id)
{
  m_fstsID = id;
}

uint8_t
ExtFstAckRequest::GetDialogToken (void) const
{
  return m_dialogToken;
}

uint32_t
ExtFstAckRequest::GetFstsID (void) const
{
  return m_fstsID;
}

/***************************************************
*     Fast Session Transfer Ack Response 8.5.21.6
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtFstAckResponse);

TypeId
ExtFstAckResponse::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtFstAckResponse")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtFstAckResponse> ()
  ;
  return tid;
}

TypeId
ExtFstAckResponse::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

/***************************************************
*             Announce Frame (8.5.22.2)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtAnnounceFrame);

ExtAnnounceFrame::ExtAnnounceFrame ()
  : m_timestamp (0),
    m_beaconInterval (0)
{
}

TypeId
ExtAnnounceFrame::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtAnnounceFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtAnnounceFrame> ()
  ;
  return tid;
}

TypeId
ExtAnnounceFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtAnnounceFrame::Print (std::ostream &os) const
{
  os << "Timestamp = " << m_timestamp << "|"
     << "BeaconInterval = " << m_beaconInterval;
  PrintInformationElements (os);
}

uint32_t
ExtAnnounceFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Timestamp
  size += 2; //Beacon Interval
  size += GetInformationElementsSerializedSize ();
  return size;
}

void
ExtAnnounceFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_timestamp);
  i.WriteHtolsbU16 (m_beaconInterval);
  i = SerializeInformationElements (i);
}

uint32_t
ExtAnnounceFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_timestamp = i.ReadU8 ();
  m_beaconInterval = i.ReadLsbtohU16 ();
  i = DeserializeInformationElements (i);
  return i.GetDistanceFrom (start);
}

void
ExtAnnounceFrame::SetTimestamp (uint8_t timestamp)
{
  m_timestamp = timestamp;
}

void
ExtAnnounceFrame::SetBeaconInterval (uint16_t interval)
{
  m_beaconInterval = interval;
}

uint8_t
ExtAnnounceFrame::GetTimestamp (void) const
{
  return m_timestamp;
}

uint16_t
ExtAnnounceFrame::GetBeaconInterval (void) const
{
  return m_beaconInterval;
}

/***************************************************
*               BRP Frame (8.5.22.3)
****************************************************/

NS_OBJECT_ENSURE_REGISTERED (ExtBrpFrame);

ExtBrpFrame::ExtBrpFrame ()
{
}

TypeId
ExtBrpFrame::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ExtBrpFrame")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ExtBrpFrame> ()
  ;
  return tid;
}

TypeId
ExtBrpFrame::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
ExtBrpFrame::Print (std::ostream &os) const
{
  os << "Dialog Token = " << m_dialogToken;
}

uint32_t
ExtBrpFrame::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 1; //Dialog Token
  size += m_brpRequestField.GetSerializedSize ();
  size += m_element.GetSerializedSize ();
  for (ChannelMeasurementFeedbackElementList::const_iterator iter = m_list.begin (); iter != m_list.end (); iter++)
    {
      size += (*iter)->GetSerializedSize ();
    }
  return size;
}

void
ExtBrpFrame::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_dialogToken);
  i = m_brpRequestField.Serialize (i);
  i = m_element.Serialize (i);
  for (ChannelMeasurementFeedbackElementList::const_iterator iter = m_list.begin (); iter != m_list.end (); iter++)
    {
      i = (*iter)->Serialize (i);
    }
}

uint32_t
ExtBrpFrame::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
//  Buffer::Iterator m;
//  uint8_t id, length;

  m_dialogToken = i.ReadU8 ();
  i = m_brpRequestField.Deserialize (i);
  i = m_element.Deserialize (i);

  /* Deserialize Infomration Elements */
  if (!i.IsEnd ())
    {
//      WifiInformationElement *element;
//      do
//        {
//          m = i;
//          i = DeserializeElementID (i, id, length);
//          m_list.push_back (element);
//        }
//      while (i.GetDistanceFrom (m) != 0);
    }

  return i.GetDistanceFrom (start);
}

void
ExtBrpFrame::SetDialogToken (uint8_t token)
{
  m_dialogToken = token;
}

void
ExtBrpFrame::SetBrpRequestField (BRP_Request_Field &field)
{
  m_brpRequestField = field;
}

void
ExtBrpFrame::SetBeamRefinementElement (BeamRefinementElement &element)
{
  m_element = element;
}

void
ExtBrpFrame::AddChannelMeasurementFeedback (ChannelMeasurementFeedbackElement *element)
{
  m_list.push_back (element);
}

uint8_t
ExtBrpFrame::GetDialogToken (void) const
{
  return m_dialogToken;
}

BRP_Request_Field
ExtBrpFrame::GetBrpRequestField (void) const
{
  return m_brpRequestField;
}

BeamRefinementElement
ExtBrpFrame::GetBeamRefinementElement (void) const
{
  return m_element;
}

ChannelMeasurementFeedbackElementList
ExtBrpFrame::GetChannelMeasurementFeedbackList (void) const
{
  return m_list;
}

} // namespace ns3
