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

#include "dca-txop-ad.h"
#include "wifi/model/wifi-mac-queue.h"
#include "wifi/model/dcf-state.h"

#include "ns3/log.h"
namespace ns3 {

DcaTxopAd::DcaTxopAd()
  : DcaTxop ()
{

}

void
DcaTxopAd::SetTxOkCallback (TxPacketOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txPacketOkCallback = callback;
}

void
DcaTxopAd::SetTxOkNoAckCallback (TxOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkNoAckCallback = callback;
}

void
DcaTxopAd::ResetState (void)
{
  NS_LOG_FUNCTION (this);
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
}

void
DcaTxopAd::InitiateTransmission (AllocationID allocationID, Time cbapDuration)
{
  NS_LOG_FUNCTION (this << cbapDuration);
  m_allocationID = allocationID;
  m_cbapDuration = cbapDuration;
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

  StartAccessIfNeeded ();
}

void
DcaTxopAd::EndCurrentContentionPeriod (void)
{
  NS_LOG_FUNCTION (this);
  /* Store parameters related to this service period which include MSDU */
  if (m_currentPacket != 0)
    {
      m_storedPackets[m_allocationID] = std::make_pair (m_currentPacket, m_currentHdr);
      m_currentPacket = 0;
    }
}

bool
DcaTxopAd::NeedsAccess (void) const
{
  NS_LOG_FUNCTION (this);
  return !m_queue->IsEmpty () || m_currentPacket != 0;
}

void
DcaTxopAd::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty ())
      && !m_dcf->IsAccessRequested ()
      && m_manager->IsAccessAllowed ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}

void
DcaTxopAd::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && !m_queue->IsEmpty ()
      && !m_dcf->IsAccessRequested ()
      && m_manager->IsAccessAllowed ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}


void
DcaTxopAd::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);

  /* Update CBAP duration */
  if (m_stationManager->HasDmgSupported ())
    {
      m_cbapDuration = m_cbapDuration - (Simulator::Now () - m_transmissionStarted);
      if (m_cbapDuration <= Seconds (0))
        {
          NS_LOG_DEBUG ("No more time in the current CBAP Allocation");
          return;
        }
    }

  DcaTxopAd::NotifyAccessGranted();
}

void
DcaTxopAd::StartNextFragment (void)
{
  if (m_stationManager->HasDmgSupported ())
    {
      m_currentParams.SetAsBoundedTransmission ();
      m_currentParams.SetMaximumTransmissionDuration (m_cbapDuration);
    }
}

void
DcaTxopAd::EndTxNoAck (void)
{
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
  DcaTxop::EndTxNoAck();
}

bool
DcaTxopAd::ShouldRtsAndAckBeDisabled () const
{
  return (m_currentHdr.GetAddr1 ().IsGroup () || || m_currentHdr.IsActionNoAck ());
}


} // namespace ns3
