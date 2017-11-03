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
#include "wifi-remote-station-manager-ad.h"
#include "wifi/model/wifi-phy.h"
#include "wifi/model/wifi-mac.h"
#include "wifi-tx-vector-ad.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiRemoteStationManager);

TypeId
WifiRemoteStationManagerAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiRemoteStationManagerAd")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("WifiAd")
    .AddTraceSource ("MacTxOK",
                     "The transmission of an MPDU  packet by the MAC layer has successed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManagerAd::m_macTxOk),
                     "ns3::Mac48Address::TracedCallback")
  ;
  return tid;
}

WifiRemoteStationManagerAd::WifiRemoteStationManagerAd() :
  WifiRemoteStationManager (),
  m_dmgSupported (false)
{
}

WifiRemoteStationManagerAd::~WifiRemoteStationManagerAd ()
{
}

void
WifiRemoteStationManagerAd::SetDmgSupported (bool enable)
{
  m_dmgSupported = enable;
}

bool
WifiRemoteStationManagerAd::HasDmgSupported (void) const
{
  return m_dmgSupported;
}

WifiTxVector
WifiRemoteStationManagerAd::GetDataTxVector (Mac48Address address, const WifiMacHeader *header, Ptr<const Packet> packet)
{
  /* Beam Tracking is Requested */
  if (header->IsBeamTrackingRequested ())
    {
      WifiTxVector v = DoGetDataTxVector (Lookup (address, header));
      WifiTxVectorAd vectorAd (v);
      vectorAd.RequestBeamTracking ();
      vectorAd.SetPacketType (header->GetPacketType ());
      vectorAd.SetTrainngFieldLength (header->GetTrainngFieldLength ());
      return v;
    }
  return WifiRemoteStationManager::GetDataTxVector(address, header, packet);
}

WifiTxVector
WifiRemoteStationManagerAd::GetDmgTxVector (Mac48Address address, const WifiMacHeader *header, Ptr<const Packet> packet)
{
  WifiTxVectorAd v;

  /* BRP Frame */
  if (header->IsActionNoAck ())
    {
      v.SetMode (WifiMode ("DMG_MCS1")); /* The BRP Frame shall be transmitted at MCS1 */
      v.SetPacketType (header->GetPacketType ());
      v.SetTrainngFieldLength (header->GetTrainngFieldLength ());
    }
  /* Beamforming Training (SLS) */
  else if (header->IsDMGBeacon () || header->IsSSW () || header->IsSSW_FBCK () || header->IsSSW_ACK ())
    {
      v.SetMode (WifiMode ("DMG_MCS0"));
      v.SetTrainngFieldLength (0);
//      v.SetPreambleType (WIFI_PREAMBLE_DMG_CTRL);
    }
  /* Dynamic Polling */
  else if (header->IsPollFrame () || header->IsGrantFrame () || header->IsSprFrame ())
    {
      v.SetMode (WifiMode ("DMG_MCS1"));
      v.SetTrainngFieldLength (0);
    }

//  v.SetPreambleType (GetPreambleForTransmission (mode, address));
  v.SetPreambleType (WIFI_PREAMBLE_LONG);
  v.SetTxPowerLevel (m_defaultTxPowerLevel);
  v.SetNss (1);
  v.SetNess (0);
  v.SetStbc (false);

  return v;
}

void
WifiRemoteStationManagerAd::ReportRtsOk (Mac48Address address, const WifiMacHeader *header,
                                         double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  m_macTxOk (address);
  m_txCallbackOk (address);
  WifiRemoteStationManager::ReportRtsOk(address, header, ctsSnr, ctsMode, rtsSnr);
}

void
WifiRemoteStationManagerAd::ReportDataOk (Mac48Address address, const WifiMacHeader *header,
                                          double ackSnr, WifiMode ackMode, double dataSnr)
{
  m_macTxOk (address);
  m_txCallbackOk (address);
  WifiRemoteStationManager::ReportDataOk(address, header, ackSnr, ackMode, dataSnr);
}

void
WifiRemoteStationManagerAd::ReportRxOk (Mac48Address address, const WifiMacHeader *header,
                                        double rxSnr, WifiMode txMode)
{
  WifiRemoteStationManager::ReportRxOk(address, header, rxSnr, txMode);
  m_rxSnr = rxSnr;
  if (!address.IsGroup ())
    {
      m_rxCallbackOk (address);
    }
}

WifiTxVector
WifiRemoteStationManagerAd::GetDmgControlTxVector (void)
{
  WifiTxVector v;
  v.SetMode (m_wifiPhy->GetMode (0)); // DMG Control Modulation Class (MCS0)
  v.SetTxPowerLevel (m_defaultTxPowerLevel);
  return v;
}

WifiTxVector
WifiRemoteStationManagerAd::GetDmgLowestScVector (void)
{
  WifiTxVector v;
  v.SetMode (m_wifiPhy->GetMode (1)); // DMG SC Modulation Class (MCS1)
  v.SetTxPowerLevel (m_defaultTxPowerLevel);
  v.SetPreambleType (WIFI_PREAMBLE_LONG);
  return v;
}


void
WifiRemoteStationManagerAd::AddStationDmgCapabilities (Mac48Address from, Ptr<DmgCapabilities> dmgCapabilities)
{
  //Used by all stations to record DMG capabilities of remote stations
  WifiRemoteStationState *state;
  state = LookupState (from);
  state->m_qosSupported = true;
  state->m_dmgSupported = true;
}
void
WifiRemoteStationManagerAd::RegisterTxOkCallback (Callback<void, Mac48Address> callback)
{
  m_txCallbackOk = callback;
}

void
WifiRemoteStationManagerAd::RegisterRxOkCallback (Callback<void, Mac48Address> callback)
{
  m_rxCallbackOk = callback;
}

WifiRemoteStationManager::StationStates*
WifiRemoteStationManagerAd::GetStationStates (void)
{
  return &m_states;
}

uint32_t
WifiRemoteStationManagerAd::GetNAssociatedStation (void) const
{
  return m_states.size ();
}

double
WifiRemoteStationManagerAd::GetRxSnr (void) const
{
  return m_rxSnr;
}

WifiMode
WifiRemoteStationManagerAd::GetControlAnswerMode (Mac48Address address, WifiMode reqMode)
{
  if (HasDmgSupported ())
    {
      /**
       * Rules for selecting a control response rate from IEEE 802.11ad-2012,
       * Section 9.7.5a Multirate support for DMG STAs:
       */
      WifiMode thismode;
      /* We start from SC PHY Rates, This is for transmitting an ACK frame or a BA frame */
      for (uint32_t idx = 0; idx < m_wifiPhy->GetNModes (); idx++)
        {
          thismode = m_wifiPhy->GetMode (idx);
          if (thismode.IsMandatory () && (thismode.GetDataRate () <= reqMode.GetDataRate ()))
            {
              return thismode;
            }
          else
            {
              // TODO: Why a for if there is a break there? Other modes than 0 are not checked.
              break;
            }
        }
    }
  else
    {
      return WifiRemoteStationManager::GetControlAnswerMode(address, reqMode);
    }
}

} // namespace ns3
