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

#include "regular-wifi-mac-ad-extension.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RegularWifiMacExtensionAd");
NS_OBJECT_ENSURE_REGISTERED (RegularWifiMacExtensionAd);

TypeId
RegularWifiMacExtensionAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RegularWifiMacAdExtension")
    .SetParent<WifiMacExtensionAd> ()
    .SetGroupName ("WifiAd")
    .AddAttribute ("DmgSupported",
                   "This Boolean attribute is set to enable 802.11ad support at this STA",
                    BooleanValue (false),
                    MakeBooleanAccessor (&RegularWifiMacExtensionAd::SetDmgSupported,
                                         &RegularWifiMacExtensionAd::GetDmgSupported),
                    MakeBooleanChecker ())
    /* Fast Session Transfer Support */
    .AddAttribute ("LLT", "The value of link loss timeout in microseconds",
                    UintegerValue (0),
                    MakeUintegerAccessor (&RegularWifiMacExtensionAd::m_llt),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FstTimeout", "The timeout value of FST session in TUs.",
                    UintegerValue (10),
                    MakeUintegerAccessor (&RegularWifiMacExtensionAd::m_fstTimeout),
                    MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("SupportMultiBand",
                   "Support multi-band operation for fast session transfer.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&RegularWifiMacExtensionAd::m_supportMultiBand),
                    MakeBooleanChecker ())
    ;
  return tid;
}

RegularWifiMacExtensionAd::RegularWifiMacExtensionAd(const Ptr<WifiMac> &mac)
  : WifiMacExtensionAd (mac)
{
  m_dmgSupported = 0;
  m_fstId = 0;
}

bool
RegularWifiMacExtensionAd::GetDmgSupported () const
{
  return m_dmgSupported;
}

void
RegularWifiMacExtensionAd::SetDmgSupported (bool enable)
{
  NS_LOG_FUNCTION (this);

  m_dmgSupported = enable;

  if (enable)
    {
      DynamicCast<RegularWifiMac> (m_mac)->SetQosSupported (true);
      DynamicCast<RegularWifiMac> (m_mac)->EnableAggregation ();
    }
  else
    {
      DynamicCast<RegularWifiMac> (m_mac)->DisableAggregation ();
    }
}

/**
 * Functions for Fast Session Transfer.
 */
void
RegularWifiMacExtensionAd::SetupFSTSession (Mac48Address staAddress)
{
  NS_LOG_FUNCTION (this << staAddress);

  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (staAddress);
  hdr.SetAddr2 (m_mac->GetAddress ());
  hdr.SetAddr3 (m_mac->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstSetupRequest requestHdr;

  /* Generate new FST Session ID */
  m_fstId++;

  SessionTransitionElement sessionTransition;
  Band newBand, oldBand;
  newBand.Band_ID = Band_4_9GHz;
  newBand.Setup = 1;
  newBand.Operation = 1;
  sessionTransition.SetNewBand (newBand);
  oldBand.Band_ID = Band_60GHz;
  oldBand.Setup = 1;
  oldBand.Operation = 1;
  sessionTransition.SetOldBand (oldBand);
  sessionTransition.SetFstsID (m_fstId);
  sessionTransition.SetSessionControl (SessionType_InfrastructureBSS, false);

  requestHdr.SetSessionTransition (sessionTransition);
  requestHdr.SetLlt (m_llt);
  requestHdr.SetMultiBand (GetMultiBandElement ());
  requestHdr.SetDialogToken (10);

  /* We are the initiator of the FST session */
  FstSession fstSession;
  fstSession.ID = m_fstId;
  fstSession.CurrentState = FST_INITIAL_STATE;
  fstSession.IsInitiator = true;
  fstSession.NewBandId = Band_4_9GHz;
  fstSession.LLT = m_llt;
  m_fstSessionMap[staAddress] = fstSession;

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_SETUP_REQUEST;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  DynamicCast<RegularWifiMac> (m_mac)->m_dca->Queue (packet, hdr);
}

void
RegularWifiMacExtensionAd::SendFstSetupResponse (Mac48Address to, uint8_t token, uint16_t status
                                                 , SessionTransitionElement sessionTransition)
{
  NS_UNUSED(sessionTransition);
  NS_LOG_FUNCTION (this << to << token << status);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_mac->GetAddress ());
  hdr.SetAddr3 (m_mac->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstSetupResponse responseHdr;
  responseHdr.SetDialogToken (token);
  responseHdr.SetStatusCode (status);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_SETUP_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  DynamicCast<RegularWifiMac> (m_mac)->m_dca->Queue (packet, hdr);
}

void
RegularWifiMacExtensionAd::SendFstAckRequest (Mac48Address to, uint8_t dialog, uint32_t fstsID)
{
  NS_LOG_FUNCTION (this << to << dialog);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_mac->GetAddress ());
  hdr.SetAddr3 (m_mac->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstAckResponse requestHdr;
  requestHdr.SetDialogToken (dialog);
  requestHdr.SetFstsID (fstsID);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_ACK_REQUEST;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  DynamicCast<RegularWifiMac> (m_mac)->m_dca->Queue (packet, hdr);
}

void
RegularWifiMacExtensionAd::SendFstAckResponse (Mac48Address to, uint8_t dialog, uint32_t fstsID)
{
  NS_LOG_FUNCTION (this << to << dialog);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_mac->GetAddress ());
  hdr.SetAddr3 (m_mac->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstAckResponse responseHdr;
  responseHdr.SetDialogToken (dialog);
  responseHdr.SetFstsID (fstsID);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_ACK_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  DynamicCast<RegularWifiMac> (m_mac)->m_dca->Queue (packet, hdr);
}

void
RegularWifiMacExtensionAd::SendFstTearDownFrame (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_mac->GetAddress ());
  hdr.SetAddr3 (m_mac->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstTearDown frame;
  frame.SetFstsID (0);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_TEAR_DOWN;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (frame);
  packet->AddHeader (actionHdr);

  DynamicCast<RegularWifiMac> (m_mac)->m_dca->Queue (packet, hdr);
}

void
RegularWifiMacExtensionAd::NotifyBandChanged (enum WifiPhyStandard, Mac48Address address, bool isInitiator)
{
  NS_LOG_FUNCTION (this << address << isInitiator);
  /* If we are the initiator, we send FST Ack Request in the new band */
  if (isInitiator)
    {
      /* We transfer FST ACK Request in the new frequency band */
      FstSession *fstSession = &m_fstSessionMap[address];
      SendFstAckRequest (address, 0, fstSession->ID);
    }
}

void
RegularWifiMacExtensionAd::ChangeBand (Mac48Address peerStation, BandID bandId, bool isInitiator)
{
  NS_LOG_FUNCTION (this << peerStation << bandId << isInitiator);
  if (bandId == Band_60GHz)
    {
      m_bandChangedCallback (WIFI_PHY_STANDARD_80211ad, peerStation, isInitiator);
    }
  else if (bandId == Band_4_9GHz)
    {
      m_bandChangedCallback (WIFI_PHY_STANDARD_80211n_5GHZ, peerStation, isInitiator);
    }
  else if (bandId == Band_2_4GHz)
    {
      m_bandChangedCallback (WIFI_PHY_STANDARD_80211n_2_4GHZ, peerStation, isInitiator);
    }
}

void
RegularWifiMacExtensionAd::RegisterBandChangedCallback (BandChangedCallback callback)
{
  m_bandChangedCallback = callback;
}

void
RegularWifiMacExtensionAd::TxOk (Ptr<const Packet> currentPacket, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (hdr.IsAction ())
    {
      WifiActionHeader actionHdr;
      Ptr<Packet> packet = currentPacket->Copy ();
      packet->RemoveHeader (actionHdr);

      if (actionHdr.GetCategory () == WifiActionHeader::FST)
        {
          switch (actionHdr.GetAction ().fstAction)
            {
            case WifiActionHeader::FST_SETUP_RESPONSE:
              {
                NS_LOG_LOGIC ("FST Responder: Received ACK for FST Response, so transit to FST_SETUP_COMPLETION_STATE");
                FstSession *fstSession = &m_fstSessionMap[hdr.GetAddr1 ()];
                fstSession->CurrentState = FST_SETUP_COMPLETION_STATE;
                /* We are the responder of the FST session and we got ACK for FST Setup Response */
                if (fstSession->LLT == 0)
                  {
                    NS_LOG_LOGIC ("FST Responder: LLT=0, so transit to FST_TRANSITION_DONE_STATE");
                    fstSession->CurrentState = FST_TRANSITION_DONE_STATE;
                    ChangeBand (hdr.GetAddr1 (), fstSession->NewBandId, false);
                  }
                else
                  {
                    NS_LOG_LOGIC ("FST Responder: LLT>0, so start Link Loss Countdown");
                    /* Initiate a Link Loss Timer */
                    fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                              &RegularWifiMacExtensionAd::ChangeBand, this,
                                                                              hdr.GetAddr1 (), fstSession->NewBandId, false);
                  }
                return;
              }
            case WifiActionHeader::FST_ACK_RESPONSE:
              {
                /* We are the Responder of the FST session and we got ACK for FST ACK Response */
                NS_LOG_LOGIC ("FST Responder: Transmitted FST ACK Response successfully, so transit to FST_TRANSITION_CONFIRMED_STATE");
                FstSession *fstSession = &m_fstSessionMap[hdr.GetAddr1 ()];
                fstSession->CurrentState = FST_TRANSITION_CONFIRMED_STATE;
                return;
              }
            default:
              break;
            }
        }
    }
}

void RegularWifiMacExtensionAd::DoReceive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  if (hdr->IsMgt () && hdr->IsAction ())
    {
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        /* Fast Session Transfer */
        case WifiActionHeader::FST:
          switch (actionHdr.GetAction ().fstAction)
            {
            case WifiActionHeader::FST_SETUP_REQUEST:
              {
                ExtFstSetupRequest requestHdr;
                packet->RemoveHeader (requestHdr);
                /* We are the responder of the FST, create new entry for FST Session */
                FstSession fstSession;
                fstSession.ID = requestHdr.GetSessionTransition ().GetFstsID ();
                fstSession.CurrentState = FST_INITIAL_STATE;
                fstSession.IsInitiator = false;
                fstSession.NewBandId = static_cast<BandID> (requestHdr.GetSessionTransition ().GetNewBand ().Band_ID);
                fstSession.LLT = requestHdr.GetLlt ();
                m_fstSessionMap[hdr->GetAddr2 ()] = fstSession;
                NS_LOG_LOGIC ("FST Responder: Received FST Setup Request with LLT=" << m_llt);
                /* Send FST Response to the Initiator */
                SendFstSetupResponse (hdr->GetAddr2 (), requestHdr.GetDialogToken (), 0, requestHdr.GetSessionTransition ());
                return;
              }
            case WifiActionHeader::FST_SETUP_RESPONSE:
              {
                ExtFstSetupResponse responseHdr;
                packet->RemoveHeader (responseHdr);
                /* We are the initiator of the FST */
                if (responseHdr.GetStatusCode () == 0)
                  {
                    /* Move the Initiator to the Setup Completion State */
                    NS_LOG_LOGIC ("FST Initiator: Received FST Setup Response with Status=0, so transit to FST_SETUP_COMPLETION_STATE");
                    FstSession *fstSession = &m_fstSessionMap[hdr->GetAddr2 ()];
                    fstSession->CurrentState = FST_SETUP_COMPLETION_STATE;
                    if (fstSession->LLT == 0)
                      {
                        NS_LOG_LOGIC ("FST Initiator: Received FST Setup Response with LLT=0, so transit to FST_TRANSITION_DONE_STATE");
                        fstSession->CurrentState = FST_TRANSITION_DONE_STATE;
                        /* Optionally transmit Broadcast FST Setup Response */

                        /* Check the new band */
                        ChangeBand (hdr->GetAddr2 (), fstSession->NewBandId, true);
                      }
                    else
                      {
                        NS_LOG_LOGIC ("FST Initiator: Received FST Setup Response with LLT>0, so Initiate Link Loss Countdown");
                        /* Initiate a Link Loss Timer */
                        fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                                  &RegularWifiMacExtensionAd::ChangeBand, this,
                                                                                  hdr->GetAddr2 (), fstSession->NewBandId, true);
                      }
                  }
                else
                  {
                    NS_LOG_DEBUG ("FST Failed with " << hdr->GetAddr2 ());
                  }
                return;
              }
            case WifiActionHeader::FST_TEAR_DOWN:
              {
                ExtFstTearDown teardownHdr;
                packet->RemoveHeader (teardownHdr);
                FstSession *fstSession = &m_fstSessionMap[hdr->GetAddr2 ()];
                fstSession->CurrentState = FST_INITIAL_STATE;
                NS_LOG_DEBUG ("FST session with ID= " << teardownHdr.GetFstsID ()<< "is terminated");
                return;
              }
            case WifiActionHeader::FST_ACK_REQUEST:
              {
                ExtFstAckRequest requestHdr;
                packet->RemoveHeader (requestHdr);
                SendFstAckResponse (hdr->GetAddr2 (), requestHdr.GetDialogToken (), requestHdr.GetFstsID ());
                NS_LOG_LOGIC ("FST Responder: Received FST ACK Request for FSTS ID=" << requestHdr.GetFstsID ()
                              << " so transmit FST ACK Response");
                return;
              }
            case WifiActionHeader::FST_ACK_RESPONSE:
              {
                ExtFstAckResponse responseHdr;
                packet->RemoveHeader (responseHdr);
                /* We are the initiator, Get FST Session */
                FstSession *fstSession = &m_fstSessionMap[hdr->GetAddr2 ()];
                fstSession->CurrentState = FST_TRANSITION_CONFIRMED_STATE;
                NS_LOG_LOGIC ("FST Initiator: Received FST ACK Response for FSTS ID=" << responseHdr.GetFstsID ()
                              << " so transit to FST_TRANSITION_CONFIRMED_STATE");
                return;
              }
            }
        }
    }
}

void
RegularWifiMacExtensionAd::MacTxOk (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  FstSessionMap::iterator it = m_fstSessionMap.find (address);
  if (it != m_fstSessionMap.end ())
    {
      FstSession *fstSession = static_cast<FstSession*> (&it->second);
      if ((fstSession->CurrentState == FST_SETUP_COMPLETION_STATE) && fstSession->LinkLossCountDownEvent.IsRunning ())
        {
          NS_LOG_INFO ("Transmitted MPDU Successfully, so reset Link Count Down Timer");
          fstSession->LinkLossCountDownEvent.Cancel ();
          fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                    &RegularWifiMacExtensionAd::ChangeBand, this,
                                                                    address, fstSession->NewBandId, fstSession->IsInitiator);
        }
    }
}

void
RegularWifiMacExtensionAd::MacRxOk (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  FstSessionMap::iterator it = m_fstSessionMap.find (address);
  if (it != m_fstSessionMap.end ())
    {
      FstSession *fstSession = static_cast<FstSession*> (&it->second);
      if ((fstSession->CurrentState == FST_SETUP_COMPLETION_STATE) && fstSession->LinkLossCountDownEvent.IsRunning ())
        {
          NS_LOG_INFO ("Received MPDU Successfully, so reset Link Count Down Timer");
          fstSession->LinkLossCountDownEvent.Cancel ();
          fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                    &RegularWifiMacExtensionAd::ChangeBand, this,
                                                                    address, fstSession->NewBandId, fstSession->IsInitiator);
        }
    }
}

void
RegularWifiMacExtensionAd::SetWifiRemoteStationManager(const Ptr<WifiRemoteStationManager> stationManager)
{
  /* Connect trace source for FST */
  stationManager->RegisterTxOkCallback (MakeCallback (&RegularWifiMac::MacTxOk, this));
  stationManager->RegisterRxOkCallback (MakeCallback (&RegularWifiMac::MacRxOk, this));
  stationManager->SetDmgSupported (GetDmgSupported ());
}

void
RegularWifiMacExtensionAd::FinishConfigureStandard (WifiPhyStandard standard)
{
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211ad:
      SetDmgSupported (true);
      DynamicCast<RegularWifiMac> (m_mac)->ConfigureContentionWindow(15, 1023);
      break;
    default:
      return;
    }
}

} // namespace ns3
