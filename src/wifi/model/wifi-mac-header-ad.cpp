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
#include "wifi-mac-header-ad.h"
#include "ns3/address-utils.h"

namespace ns3 {
/*
 * Enumeration for DMG Control Frame Extension.
 */
enum
{
  SUBTYPE_CTL_EXTENSION_POLL = 2,
  SUBTYPE_CTL_EXTENSION_SPR = 3,
  SUBTYPE_CTL_EXTENSION_GRANT = 4,
  SUBTYPE_CTL_EXTENSION_DMG_CTS = 5,
  SUBTYPE_CTL_EXTENSION_DMG_DTS = 6,
  SUBTYPE_CTL_EXTENSION_GRANT_ACK = 7,
  SUBTYPE_CTL_EXTENSION_SSW = 8,
  SUBTYPE_CTL_EXTENSION_SSW_FBCK = 9,
  SUBTYPE_CTL_EXTENSION_SSW_ACK = 10
};

WifiMacHeaderAd::WifiMacHeaderAd()
{

}

void
WifiMacHeaderAd::SetDMGBeacon (void)
{
  m_ctrlType = TYPE_Extension;
  m_ctrlSubtype = 0;
}

void
WifiMacHeaderAd::SetActionNoAck (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0E;
}

bool
WifiMacHeaderAd::IsExtension (void) const
{
  return (m_ctrlType == TYPE_Extension);
}

bool
WifiMacHeaderAd::IsDmgCts (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_CTS);
}

bool
WifiMacHeaderAd::IsDmgDts (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_DTS);
}

bool
WifiMacHeaderAd::IsActionNoAck () const
{
  return (GetType () == WIFI_MAC_MGT_ACTION_NO_ACK);
}

bool
WifiMacHeaderAd::IsDMGBeacon (void) const
{
  return (GetType () == WIFI_MAC_EXTENSION_DMG_BEACON);
}

bool
WifiMacHeaderAd::IsSSW (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SSW);
}

bool
WifiMacHeaderAd::IsSSW_FBCK (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SSW_FBCK);
}

bool
WifiMacHeaderAd::IsSSW_ACK (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SSW_ACK);
}

bool
WifiMacHeaderAd::IsPollFrame (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_POLL);
}

bool
WifiMacHeaderAd::IsSprFrame (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_SPR);
}

bool
WifiMacHeaderAd::IsGrantFrame (void) const
{
  return (GetType () == WIFI_MAC_CTL_DMG_GRANT);
}

void
WifiMacHeaderAd::SetAsDmgPpdu (void)
{
  m_dmgPpdu = true;
}

void
WifiMacHeaderAd::SetQosAmsduType (uint8_t type)
{
  m_qosAmsduType = type;
}

void WifiMacHeaderAd::SetQosRdGrant (bool value)
{
  m_qosRdg = value;
}

void
WifiMacHeaderAd::SetQosAcConstraint (bool value)
{
  m_qosAcConstraint = value;
}

bool
WifiMacHeaderAd::GetQosAmsduType (void) const
{
  NS_ASSERT (m_dmgPpdu && IsQosData ());
  return m_qosAmsduType;
}

bool
WifiMacHeaderAd::IsQosRdGrant (void) const
{
  NS_ASSERT (m_dmgPpdu && IsQosData ());
  return (m_qosRdg == 1);
}

bool
WifiMacHeaderAd::GetQosAcConstraint (void) const
{
  NS_ASSERT (m_dmgPpdu && IsQosData ());
  return m_qosAcConstraint;
}

void
WifiMacHeaderAd::SetPacketType (PacketType type)
{
  m_brpPacketType = type;
}

PacketType
WifiMacHeaderAd::GetPacketType (void) const
{
  return m_brpPacketType;
}

void
WifiMacHeaderAd::SetTrainngFieldLength (uint8_t length)
{
  m_trainingFieldLength = length;
}

uint8_t
WifiMacHeaderAd::GetTrainngFieldLength (void) const
{
  return m_trainingFieldLength;
}

void
WifiMacHeaderAd::RequestBeamRefinement (void)
{
  m_beamRefinementRequired = true;
}

void
WifiMacHeaderAd::DisableBeamRefinement (void)
{
  m_beamRefinementRequired = false;
}

bool
WifiMacHeaderAd::IsBeamRefinementRequested (void) const
{
  return (m_beamRefinementRequired == true);
}

void
WifiMacHeaderAd::RequestBeamTracking (void)
{
  m_beamTrackingRequired = true;
}

void
WifiMacHeaderAd::DisableBeamTracking (void)
{
  m_beamTrackingRequired = false;
}

bool
WifiMacHeaderAd::IsBeamTrackingRequested (void) const
{
  return (m_beamTrackingRequired == true);
}

void
WifiMacHeaderAd::SetType (WifiMacType type)
{
  switch (type)
    {
    case WIFI_MAC_CTL_DMG_POLL:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_POLL;
      break;
    case WIFI_MAC_CTL_DMG_SPR:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SPR;
      break;
    case WIFI_MAC_CTL_DMG_GRANT:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_GRANT;
      break;
    case WIFI_MAC_CTL_DMG_CTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_DMG_CTS;
      break;
    case WIFI_MAC_CTL_DMG_DTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_DMG_DTS;
      break;
    case WIFI_MAC_CTL_DMG_SSW:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SSW;
      break;
    case WIFI_MAC_CTL_DMG_SSW_FBCK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SSW_FBCK;
      break;
    case WIFI_MAC_CTL_DMG_SSW_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_SSW_ACK;
      break;
    case WIFI_MAC_CTL_DMG_GRANT_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_EXTENSION;
      m_ctrlFrameExtension = SUBTYPE_CTL_EXTENSION_GRANT_ACK;
      break;
    case WIFI_MAC_EXTENSION_DMG_BEACON:
      m_ctrlType = TYPE_Extension;
      m_ctrlSubtype = 0;
      break;
    default:
      WifiMacHeader::SetType(type);
    }
}
WifiMacType
WifiMacHeaderAd::GetType (void) const
{
  if (m_ctrlType == TYPE_CTL && m_ctrlSubtype == SUBTYPE_CTL_EXTENSION)
    {
      switch (m_ctrlFrameExtension)
        {
        case SUBTYPE_CTL_EXTENSION_POLL:
          return WIFI_MAC_CTL_DMG_POLL;
        case SUBTYPE_CTL_EXTENSION_SPR:
          return WIFI_MAC_CTL_DMG_SPR;
        case SUBTYPE_CTL_EXTENSION_GRANT:
          return WIFI_MAC_CTL_DMG_GRANT;
        case SUBTYPE_CTL_EXTENSION_DMG_CTS:
          return WIFI_MAC_CTL_DMG_CTS;
        case SUBTYPE_CTL_EXTENSION_DMG_DTS:
          return WIFI_MAC_CTL_DMG_DTS;
        case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
          return WIFI_MAC_CTL_DMG_GRANT_ACK;
        case SUBTYPE_CTL_EXTENSION_SSW:
          return WIFI_MAC_CTL_DMG_SSW;
        case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
          return WIFI_MAC_CTL_DMG_SSW_FBCK;
        case SUBTYPE_CTL_EXTENSION_SSW_ACK:
          return WIFI_MAC_CTL_DMG_SSW_ACK;
        }
    }
  else if (m_ctrlType == TYPE_Extension && m_ctrlSubtype = 0)
    {
      return WIFI_MAC_EXTENSION_DMG_BEACON;
    }
  return WifiMacHeader::GetType();
}

uint16_t
WifiMacHeaderAd::GetFrameControl (void) const
{
  uint16_t val = 0;
  val |= (m_ctrlType << 2) & (0x3 << 2);
  val |= (m_ctrlSubtype << 4) & (0xf << 4);
  val |= (m_ctrlMoreData << 13) & (0x1 << 13);
  val |= (m_ctrlWep << 14) & (0x1 << 14);
  val |= (m_ctrlOrder << 15) & (0x1 << 15);

  if ((m_ctrlType == 1) && (m_ctrlSubtype == 6))
    {
      /* Frame Control for DMG */
      val |= (m_ctrlFrameExtension << 8) & (0xf << 8);
    }
  else
    {
      /* Frame Control for Non-DMG */
      val |= (m_ctrlToDs << 8) & (0x1 << 8);
      val |= (m_ctrlFromDs << 9) & (0x1 << 9);
      val |= (m_ctrlMoreFrag << 10) & (0x1 << 10);
      val |= (m_ctrlRetry << 11) & (0x1 << 11);
    }

  return val;
}

uint16_t
WifiMacHeaderAd::GetQosControl (void) const
{
  uint16_t val = 0;
  val |= m_qosTid;
  val |= m_qosEosp << 4;
  val |= m_qosAckPolicy << 5;
  val |= m_amsduPresent << 7;
  if (m_dmgPpdu)
    {
      val |= m_qosAmsduType << 8;
      val |= m_qosRdg << 9;
      val |= m_qosAcConstraint << 15;
    }
  else
    {
      val |= m_qosStuff << 8;
    }
  return val;
}

void
WifiMacHeaderAd::SetFrameControl (uint16_t ctrl)
{
  m_ctrlType = (ctrl >> 2) & 0x03;
  m_ctrlSubtype = (ctrl >> 4) & 0x0f;

  if ((m_ctrlType == 1) && (m_ctrlSubtype == 6))
    {
      /* Frame Control for DMG */
      m_ctrlFrameExtension = (ctrl >> 8) & 0x0F;
      m_ctrlMoreData = (ctrl >> 13) & 0x01;
      m_ctrlWep = (ctrl >> 14) & 0x01;
      m_ctrlOrder = (ctrl >> 15) & 0x01;
    }
  else
    {
      /* Frame Control for Non-DMG */
      m_ctrlToDs = (ctrl >> 8) & 0x01;
      m_ctrlFromDs = (ctrl >> 9) & 0x01;
      m_ctrlMoreFrag = (ctrl >> 10) & 0x01;
      m_ctrlRetry = (ctrl >> 11) & 0x01;
      m_ctrlMoreData = (ctrl >> 13) & 0x01;
      m_ctrlWep = (ctrl >> 14) & 0x01;
      m_ctrlOrder = (ctrl >> 15) & 0x01;
    }
}

void
WifiMacHeaderAd::SetQosControl (uint16_t qos)
{
  m_qosTid = qos & 0x000f;
  m_qosEosp = (qos >> 4) & 0x0001;
  m_qosAckPolicy = (qos >> 5) & 0x0003;
  m_amsduPresent = (qos >> 7) & 0x0001;
  if (m_dmgPpdu)
    {
      m_qosAmsduType = (qos >> 8) & 0x1;
      m_qosRdg = (qos >> 9) & 0x1;
      m_qosAcConstraint = (qos >> 15) & 0x1;
    }
  else
    {
      m_qosStuff = (qos >> 8) & 0x00ff;
    }
}

uint32_t
WifiMacHeaderAd::GetSize (void) const
{
  if (m_ctrlType == TYPE_CTL && m_ctrlSubtype == SUBTYPE_CTL_EXTENSION)
    {
      switch (m_ctrlFrameExtension)
        {
        case SUBTYPE_CTL_EXTENSION_POLL:
        case SUBTYPE_CTL_EXTENSION_SPR:
        case SUBTYPE_CTL_EXTENSION_GRANT:
        case SUBTYPE_CTL_EXTENSION_DMG_CTS:
        case SUBTYPE_CTL_EXTENSION_SSW:
        case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
        case SUBTYPE_CTL_EXTENSION_SSW_ACK:
        case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
          return 2 + 2 + 6 + 6;
        case SUBTYPE_CTL_EXTENSION_DMG_DTS:
          return 2 + 2 + 6;
        }
    }
  else if (m_ctrlType == TYPE_Extension)
    {
     return 2 + 2 + 6;
    }
  return WifiMacHeader::GetSize();
}

void
WifiMacHeaderAd::Serialize (Buffer::Iterator i) const
{
  i.WriteHtolsbU16 (GetFrameControl ());
  i.WriteHtolsbU16 (m_duration);
  WriteTo (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      WriteTo (i, m_addr2);
      WriteTo (i, m_addr3);
      i.WriteHtolsbU16 (GetSequenceControl ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_EXTENSION:
          switch (m_ctrlFrameExtension)
            {
            case SUBTYPE_CTL_EXTENSION_POLL:
            case SUBTYPE_CTL_EXTENSION_SPR:
            case SUBTYPE_CTL_EXTENSION_GRANT:
            case SUBTYPE_CTL_EXTENSION_DMG_CTS:
            case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
            case SUBTYPE_CTL_EXTENSION_SSW:
            case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
            case SUBTYPE_CTL_EXTENSION_SSW_ACK:
              WriteTo(i, m_addr2); // TA Address Field.
              break;
            case SUBTYPE_CTL_EXTENSION_DMG_DTS:
              break;
            }
          break;
        default:
          //NOTREACHED
          NS_ASSERT (false);
          break;
        }
      break;
    case TYPE_DATA:
      {
        WriteTo (i, m_addr2);
        WriteTo (i, m_addr3);
        i.WriteHtolsbU16 (GetSequenceControl ());
        if (m_ctrlToDs && m_ctrlFromDs)
          {
            WriteTo (i, m_addr4);
          }
        if (m_ctrlSubtype & 0x08)
          {
            i.WriteHtolsbU16 (GetQosControl ());
          }
      } break;
    case TYPE_Extension:
      break;
    default:
      //NOTREACHED
      NS_ASSERT (false);
      break;
    }
}

uint32_t
WifiMacHeaderAd::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t frame_control = i.ReadLsbtohU16 ();
  SetFrameControl (frame_control);
  m_duration = i.ReadLsbtohU16 ();
  ReadFrom (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_EXTENSION:
          switch (m_ctrlFrameExtension)
            {
            case SUBTYPE_CTL_EXTENSION_POLL:
            case SUBTYPE_CTL_EXTENSION_SPR:
            case SUBTYPE_CTL_EXTENSION_GRANT:
            case SUBTYPE_CTL_EXTENSION_DMG_CTS:
            case SUBTYPE_CTL_EXTENSION_GRANT_ACK:
            case SUBTYPE_CTL_EXTENSION_SSW:
            case SUBTYPE_CTL_EXTENSION_SSW_FBCK:
            case SUBTYPE_CTL_EXTENSION_SSW_ACK:
              ReadFrom(i, m_addr2); // TA Address Field.
              break;
            case SUBTYPE_CTL_EXTENSION_DMG_DTS:
              break;
            }
          break;
        }
      break;
    case TYPE_DATA:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          ReadFrom (i, m_addr4);
        }
      if (m_ctrlSubtype & 0x08)
        {
          SetQosControl (i.ReadLsbtohU16 ());
        }
      break;
    case TYPE_Extension:
      break;
    }
  return i.GetDistanceFrom (start);
}

} // namespace ns3
