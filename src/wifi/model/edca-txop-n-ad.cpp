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
#include "edca-txop-n-ad.h"
#include "dcf-state.h"
#include "random-stream.h"
#include "ns3/log.h"
#include "ns3/ptr.h"

namespace ns3 {

TypeId
EdcaTxopNAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdcaTxopNAd")
    .SetParent<EdcaTxopN> ()
    .SetGroupName ("WifiAd")
    .AddConstructor<EdcaTxopNAd> ()
  ;
  return tid;
}

EdcaTxopNAd::EdcaTxopNAd() :
  EdcaTxopN(),
  m_missedACK (false),
  m_firstTransmission (false)
{
}

uint32_t
EdcaTxopNAd::GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid) const
{
  return m_baManager->GetNBufferedPackets (address, tid);
}

uint32_t
EdcaTxopNAd::GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const
{
  return m_baManager->GetNRetryNeededPackets (recipient, tid);
}

void
EdcaTxopNAd::CopyBlockAckAgreements (Mac48Address recipient, Ptr<EdcaTxopN> target)
{
  Ptr<EdcaTxopNAd> tgt = DynamicCast<EdcaTxopNAd> (target);
  if (tgt == nullptr)
    NS_FATAL_ERROR ("Target is not capable of 802.11ad operations");

  m_baManager->CopyAgreements (recipient, tgt->m_baManager);
}

void
EdcaTxopNAd::ResetState (void)
{
  NS_LOG_FUNCTION (this);
  m_dcf->ResetCw ();
  m_cwTrace = m_dcf->GetCw ();
  m_backoffTrace = m_rng->GetNext (0, m_dcf->GetCw ());
  m_dcf->ResetState (m_backoffTrace);
}

bool
EdcaTxopNAd::NeedsAccess (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue->HasPackets () || m_currentPacket != 0 || m_baManager->HasPackets ();
}

void
EdcaTxopNAd::UpdateRemainingTime (void)
{
  m_remainingDuration = m_cbapDuration - (Simulator::Now () - m_transmissionStarted);
  if (m_remainingDuration <= Seconds (0))
    {
//          m_accessAllowed = false;
      return;
    }
}

void
EdcaTxopNAd::EndCurrentContentionPeriod (void)
{
  NS_LOG_FUNCTION (this);
  /* Store parameters related to this CBAP period which include MSDU/A-MSDU */
  m_storedPackets[m_allocationID] = std::make_pair (m_currentPacket, m_currentHdr);
  m_currentPacket = nullptr;
}

void
EdcaTxopNAd::NotifyAccessGranted()
{
    m_firstTransmission = false;

    /* Update CBAP duration */
    if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
      {
        m_remainingDuration = m_cbapDuration - (Simulator::Now () - m_transmissionStarted);
        if (m_remainingDuration <= Seconds (0))
          {
            m_accessAllowed = false;
            return;
          }

  //      if (!m_low->RestoredSuspendedTransmission ())
  //        {
  //          m_low->ResumeTransmission (m_remainingDuration, this);
  //          return;
  //        }
      }

    if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
      {
        m_currentParams.SetAsBoundedTransmission ();
        m_currentParams.SetMaximumTransmissionDuration (m_remainingDuration);
      }

    EdcaTxopN::NotifyAccessGranted ();
}

void
EdcaTxopNAd::RestartAccessIfNeeded()
{
  /* Rewrite to check things in common with edcatxopn */
  if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
    {
      if ((m_currentPacket != 0
           || !m_queue->IsEmpty () || m_baManager->HasPackets ())
          && !m_dcf->IsAccessRequested ()
          && m_manager->IsAccessAllowed ()
          && !m_low->IsTransmissionSuspended ())
        {
          Ptr<const Packet> packet;
          WifiMacHeader hdr;
          if (m_currentPacket != 0)
            {
              packet = m_currentPacket;
              hdr = m_currentHdr;
            }
          else if (m_baManager->HasPackets ())
            {
              packet = m_baManager->PeekNextPacket (hdr);
            }
          else
            {
              Ptr<const WifiMacQueueItem> item = m_queue->PeekFirstAvailable (m_qosBlockedDestinations);
              if (item)
                {
                  packet = item->GetPacket ();
                  hdr = item->GetHeader ();
                  m_currentPacketTimestamp = item->GetTimeStamp ();
                }
            }
          if (packet != 0)
            {
              m_isAccessRequestedForRts = m_stationManager->NeedRts (hdr.GetAddr1 (), &hdr, packet, m_low->GetDataTxVector (packet, &hdr));
            }
          else
            {
              m_isAccessRequestedForRts = false;
            }
          m_manager->RequestAccess (m_dcf);
        }
    }
  else
    {
      EdcaTxopNAd::RestartAccessIfNeeded();
    }
}

void
EdcaTxopNAd::StartAccessIfNeeded()
{
  if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
    {
      if (((m_firstTransmission && (m_currentPacket != 0)) || m_currentPacket == 0)
         && (m_queue->HasPackets () || m_baManager->HasPackets ())
         && !m_dcf->IsAccessRequested ()
         && m_manager->IsAccessAllowed ()
         && !m_low->IsTransmissionSuspended ())
        {
          Ptr<const Packet> packet;
          WifiMacHeader hdr;
          if (m_currentPacket != 0)
            {
              packet = m_currentPacket;
              hdr = m_currentHdr;
            }
          else if (m_baManager->HasPackets ())
            {
              packet = m_baManager->PeekNextPacket (hdr);
            }
          else
            {
              Ptr<const WifiMacQueueItem> item = m_queue->PeekFirstAvailable (m_qosBlockedDestinations);
              if (item)
                {
                  packet = item->GetPacket ();
                  hdr = item->GetHeader ();
                  m_currentPacketTimestamp = item->GetTimeStamp ();
                }
            }
          if (packet != 0)
            {
              m_isAccessRequestedForRts = m_stationManager->NeedRts (hdr.GetAddr1 (), &hdr, packet, m_low->GetDataTxVector (packet, &hdr));
            }
          else
            {
              m_isAccessRequestedForRts = false;
            }
          m_manager->RequestAccess (m_dcf);
        }
    }
  else
    {
      EdcaTxopN::StartAccessIfNeeded();
    }
}
void
EdcaTxopNAd::InitiateTransmission (AllocationID allocationID, Time cbapDuration)
{
  NS_LOG_FUNCTION (this << cbapDuration);
  m_allocationID = allocationID;
  m_cbapDuration = cbapDuration;
  m_firstTransmission = true;
  m_transmissionStarted = Simulator::Now ();

  /* Reset DCF Manager State */
  ResetState ();

  /* Check if we have stored packet for this CBAP period (MSDU/A-MSDU) */
  StoredPacketsCI it = m_storedPackets.find (allocationID);
  if (it != m_storedPackets.end ())
    {
      PacketInformation info = it->second;
      m_currentPacket = info.first;
      m_currentHdr = info.second;
    }

  /* Do the contention access by DCF Manager */
  if ((!m_queue->IsEmpty () || m_baManager->HasPackets () || m_currentPacket != 0)
      && !m_dcf->IsAccessRequested ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}

Mac48Address
EdcaTxopNAd::MapSrcAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  if (m_typeOfStation == DMG_STA || m_typeOfStation == DMG_ADHOC)
    {
      return hdr.GetAddr2 ();
    }
  else
    {
      return EdcaTxopN::MapSrcAddressForAggregation(hdr);
    }
}

Mac48Address
EdcaTxopN::MapDestAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  if (m_typeOfStation == DMG_STA || m_typeOfStation == DMG_AP || m_typeOfStation == DMG_ADHOC)
    {
      return hdr.GetAddr1 ();
    }
  else
    {
      return EdcaTxopN::MapDestAddressForAggregation(hdr);
    }
}

void
EdcaTxopNAd::GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient,
                          double rxSnr, WifiMode txMode, double dataSnr)
{
  m_missedACK = false;
  EdcaTxopN::GotBlockAck(blockAck, recipient, rxSnr, txMode, dataSnr);
}

void
EdcaTxopNAd::SendBlockAckRequest (const Bar &bar)
{
  NS_LOG_FUNCTION (this << &bar);
  if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
    {
      m_currentParams.SetAsBoundedTransmission ();
      m_currentParams.SetMaximumTransmissionDuration (m_remainingDuration);
    }

  EdcaTxopN::SendBlockAckRequest(bar);
}

void
EdcaTxopNAd::SendAddBaRequest (Mac48Address dest, uint8_t tid, uint16_t startSeq,
                             uint16_t timeout, bool immediateBAck)
{
  if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
    {
      m_currentParams.SetAsBoundedTransmission ();
      m_currentParams.SetMaximumTransmissionDuration (m_remainingDuration);
    }
  EdcaTxopN::SendAddBaRequest(dest, tid, startSeq, timeout, immediateBAck);
}

} //namespace ns3
