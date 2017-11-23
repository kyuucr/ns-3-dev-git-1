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
#include "ctrl-headers-ad.h"
#include "ns3/log.h"
#include "ns3/address-utils.h"

namespace ns3 {

/*************************
 *  Poll Frame (8.3.1.11)
 *************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDmgPoll);

CtrlDmgPoll::CtrlDmgPoll ()
    : m_responseOffset (0)
{
  NS_LOG_FUNCTION (this);
}

CtrlDmgPoll::~CtrlDmgPoll ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDmgPoll::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS();
  static TypeId tid = TypeId ("ns3::CtrlDmgPoll")
    .SetParent<Header> ()
    .AddConstructor<CtrlDmgPoll> ()
    ;
  return tid;
}

TypeId
CtrlDmgPoll::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION(this);
  return GetTypeId();
}

void
CtrlDmgPoll::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION(this << &os);
  os << "Response Offset=" << m_responseOffset;
}

uint32_t
CtrlDmgPoll::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 2;  // Response Offset Field.
}

void
CtrlDmgPoll::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (m_responseOffset);
}

uint32_t
CtrlDmgPoll::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_responseOffset = i.ReadLsbtohU16 ();
  return i.GetDistanceFrom (start);
}

void
CtrlDmgPoll::SetResponseOffset (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_responseOffset = value;
}

uint16_t
CtrlDmgPoll::GetResponseOffset (void) const
{
  NS_LOG_FUNCTION (this);
  return m_responseOffset;
}

/*************************************************
 *  Service Period Request (SPR) Frame (8.3.1.12)
 *************************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SPR);

CtrlDMG_SPR::CtrlDMG_SPR ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SPR::~CtrlDMG_SPR ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SPR::GetTypeId(void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SPR")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_SPR> ()
    ;
  return tid;
}

TypeId
CtrlDMG_SPR::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDMG_SPR::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_dynamic.Print (os);
  m_bfControl.Print (os);
}

uint32_t
CtrlDMG_SPR::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 7;  // Dynamic Allocation Info Field + BF Control.
}

void
CtrlDMG_SPR::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_dynamic.Serialize (i);
  i = m_bfControl.Serialize (i);
}

uint32_t
CtrlDMG_SPR::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_dynamic.Deserialize (i);
  i = m_bfControl.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
CtrlDMG_SPR::SetDynamicAllocationInfo (DynamicAllocationInfoField field)
{
  m_dynamic = field;
}

void
CtrlDMG_SPR::SetBFControl (BF_Control_Field value)
{
  m_bfControl = value;
}

DynamicAllocationInfoField
CtrlDMG_SPR::CtrlDMG_SPR::GetDynamicAllocationInfo (void) const
{
  return m_dynamic;
}

BF_Control_Field
CtrlDMG_SPR::GetBFControl (void) const
{
  return m_bfControl;
}

/*************************
 * Grant Frame (8.3.1.13)
 *************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_Grant);

CtrlDMG_Grant::CtrlDMG_Grant ()
{
  NS_LOG_FUNCTION(this);
}

CtrlDMG_Grant::~CtrlDMG_Grant ()
{
  NS_LOG_FUNCTION(this);
}

TypeId
CtrlDMG_Grant::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_Grant")
      .SetParent<Header>()
      .AddConstructor<CtrlDMG_Grant> ()
      ;
  return tid;
}

TypeId
CtrlDMG_Grant::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

/********************************************
 * DMG Denial to Send (DTS) Frame (8.3.1.15)
 ********************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_DTS);

CtrlDMG_DTS::CtrlDMG_DTS ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_DTS::~CtrlDMG_DTS ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_DTS::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_DTS")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_DTS> ()
    ;
  return tid;
}

TypeId
CtrlDMG_DTS::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDMG_DTS::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
}

uint32_t
CtrlDMG_DTS::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 12;  // NAV-SA + NAV-DA
}

void
CtrlDMG_DTS::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  WriteTo (i, m_navSA);
  WriteTo (i, m_navDA);
}

uint32_t
CtrlDMG_DTS::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  ReadFrom (i, m_navSA);
  ReadFrom (i, m_navDA);

  return i.GetDistanceFrom (start);
}

/****************************************
 *  Sector Sweep (SSW) Frame (8.3.1.16)
 ****************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SSW);

CtrlDMG_SSW::CtrlDMG_SSW ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SSW::~CtrlDMG_SSW ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SSW::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SSW")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_SSW> ()
    ;
  return tid;
}

TypeId
CtrlDMG_SSW::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void CtrlDMG_SSW::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_ssw.Print (os);
  m_sswFeedback.Print (os);
}

uint32_t
CtrlDMG_SSW::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 6;  // SSW Field + SSW Feedback Field.
}

void
CtrlDMG_SSW::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_ssw.Serialize (i);
  i = m_sswFeedback.Serialize (i);
}

uint32_t
CtrlDMG_SSW::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_ssw.Deserialize (i);
  i = m_sswFeedback.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
CtrlDMG_SSW::SetSswField (DMG_SSW_Field &field)
{
  m_ssw = field;
}

void
CtrlDMG_SSW::SetSswFeedbackField (DMG_SSW_FBCK_Field &field)
{
  m_sswFeedback = field;
}

DMG_SSW_Field
CtrlDMG_SSW::GetSswField (void) const
{
  return m_ssw;
}

DMG_SSW_FBCK_Field
CtrlDMG_SSW::GetSswFeedbackField (void) const
{
  return m_sswFeedback;
}

/*********************************************************
 *  Sector Sweep Feedback (SSW-Feedback) Frame (8.3.1.17)
 *********************************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SSW_FBCK);

CtrlDMG_SSW_FBCK::CtrlDMG_SSW_FBCK ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SSW_FBCK::~CtrlDMG_SSW_FBCK ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SSW_FBCK::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SSW_FBCK")
    .SetParent<Header>()
    .AddConstructor<CtrlDMG_SSW_FBCK> ()
    ;
  return tid;
}

TypeId
CtrlDMG_SSW_FBCK::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlDMG_SSW_FBCK::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_sswFeedback.Print (os);
  m_brpRequest.Print (os);
  m_linkMaintenance.Print (os);
}

uint32_t
CtrlDMG_SSW_FBCK::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 8;  // SSW Feedback Field + BRP Request + Beamformed Link Maintenance.
}

void
CtrlDMG_SSW_FBCK::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i = m_sswFeedback.Serialize (i);
  i = m_brpRequest.Serialize (i);
  i = m_linkMaintenance.Serialize (i);
}

uint32_t
CtrlDMG_SSW_FBCK::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION(this << &start);
  Buffer::Iterator i = start;

  i = m_sswFeedback.Deserialize (i);
  i = m_brpRequest.Deserialize (i);
  i = m_linkMaintenance.Deserialize (i);

  return i.GetDistanceFrom (start);
}

void
CtrlDMG_SSW_FBCK::SetSswFeedbackField (DMG_SSW_FBCK_Field &field)
{
  m_sswFeedback = field;
}

void
CtrlDMG_SSW_FBCK::SetBrpRequestField (BRP_Request_Field &field)
{
  m_brpRequest = field;
}

void
CtrlDMG_SSW_FBCK::SetBfLinkMaintenanceField (BF_Link_Maintenance_Field &field)
{
  m_linkMaintenance = field;
}

DMG_SSW_FBCK_Field
CtrlDMG_SSW_FBCK::GetSswFeedbackField (void) const
{
  return m_sswFeedback;
}

BRP_Request_Field
CtrlDMG_SSW_FBCK::GetBrpRequestField (void) const
{
  return m_brpRequest;
}

BF_Link_Maintenance_Field
CtrlDMG_SSW_FBCK::GetBfLinkMaintenanceField (void) const
{
  return m_linkMaintenance;
}

/**********************************************
 * Sector Sweep ACK (SSW-ACK) Frame (8.3.1.18)
 **********************************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlDMG_SSW_ACK);

CtrlDMG_SSW_ACK::CtrlDMG_SSW_ACK ()
{
  NS_LOG_FUNCTION (this);
}

CtrlDMG_SSW_ACK::~CtrlDMG_SSW_ACK ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlDMG_SSW_ACK::GetTypeId(void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlDMG_SSW_ACK")
    .SetParent<Header> ()
    .AddConstructor<CtrlDMG_SSW_ACK> ()
  ;
  return tid;
}

TypeId
CtrlDMG_SSW_ACK::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

/*******************************
 *  Grant ACK Frame (8.3.1.19)
 *******************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlGrantAck);

CtrlGrantAck::CtrlGrantAck ()
{
  NS_LOG_FUNCTION (this);
  memset (m_reserved, 0, sizeof (uint8_t) * 5);
}

CtrlGrantAck::~CtrlGrantAck ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CtrlGrantAck::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid = TypeId ("ns3::CtrlGrantAck")
    .SetParent<Header> ()
    .AddConstructor<CtrlGrantAck> ()
    ;
  return tid;
}

TypeId
CtrlGrantAck::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
CtrlGrantAck::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  m_bfControl.Print (os);
}

uint32_t
CtrlGrantAck::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 7;  // Reserved + BF Control.
}

void
CtrlGrantAck::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i.Write (m_reserved, 5);
  i = m_bfControl.Serialize (i);
}

uint32_t
CtrlGrantAck::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i.Read (m_reserved, 5);
  i = m_bfControl.Deserialize (i);

  return i.GetDistanceFrom (start);
}

} // namespace ns3
