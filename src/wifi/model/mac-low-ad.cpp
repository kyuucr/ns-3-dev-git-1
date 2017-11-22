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
#include "mac-low-ad.h"
#include "wifi-mac-header-ad.h"
#include "dmg-sta-wifi-mac.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

MacLowTransmissionParametersAd::MacLowTransmissionParametersAd()
  : MacLowTransmissionParameters (),
    m_boundedTransmission (false),
    m_transmitInServicePeriod (false)
{

}

void
MacLowTransmissionParametersAd::SetAsBoundedTransmission (void)
{
  m_boundedTransmission = true;
}
bool
MacLowTransmissionParametersAd::IsTransmissionBounded (void) const
{
  return (m_boundedTransmission == true);
}
void
MacLowTransmissionParametersAd::SetMaximumTransmissionDuration (Time duration)
{
  m_transmissionDuration = duration;
}
Time
MacLowTransmissionParametersAd::GetMaximumTransmissionDuration (void) const
{
  NS_ASSERT (IsTransmissionBounded ());
  return m_transmissionDuration;
}
bool
MacLowTransmissionParametersAd::IsCbapAccessPeriod (void) const
{
  return (m_transmitInServicePeriod == false);
}
void
MacLowTransmissionParametersAd::SetTransmitInSericePeriod (void)
{
  m_transmitInServicePeriod = true;
}

TypeId
MacLowAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MacLowAd")
    .SetParent<MacLow> ()
    .SetGroupName ("WifiAd")
    .AddConstructor<MacLowAd> ()
  ;
  return tid;
}

MacLowAd::MacLowAd() : MacLow (),
  m_mac (nullptr),
  m_transmissionSuspended (false)
{

}

void
MacLowAd::SetSbifs (Time sbifs)
{
  m_sbifs = sbifs;
}

void
MacLowAd::SetMbifs (Time mbifs)
{
  m_mbifs = mbifs;
}

void
MacLowAd::SetLbifs (Time lbifs)
{
  m_lbifs = lbifs;
}

void
MacLowAd::SetBrifs (Time brifs)
{
  m_brifs = brifs;
}

Time
MacLowAd::GetSbifs (void) const
{
  return m_sbifs;
}

Time
MacLowAd::GetMbifs (void) const
{
  return m_mbifs;
}

Time
MacLowAd::GetLbifs (void) const
{
  return m_lbifs;
}

Time
MacLowAd::GetBrifs (void) const
{
  return m_brifs;
}

void
MacLowAd::ResumeTransmission (Time duration, Ptr<DcaTxop> dca)
{
  NS_LOG_FUNCTION (this << duration << dca);
  /* Restore the vaiables associated to the current allocation*/
  m_restoredSuspendedTransmission = true;
  m_currentPacket = m_currentAllocation.packet;
  m_currentHdr = m_currentAllocation.hdr;
  m_txParams = m_currentAllocation.txParams;
  m_currentTxVector = m_currentAllocation.txVector;
  m_ampdu = m_currentAllocation.isAmpdu;
  /* Restore Aggregate Queue contents */
  if (m_ampdu && m_currentHdr.IsQosData ())
    {
      uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
      m_currentAllocation.aggregateQueue->QuickTransfer (m_aggregateQueue[tid]);
    }

  m_txParams.SetMaximumTransmissionDuration (duration);
  NS_LOG_DEBUG ("IsAmpdu=" << m_currentAllocation.isAmpdu
                << ", PacketSize=" << m_currentAllocation.packet->GetSize ()
                << ", seq=0x" << std::hex << m_currentHdr.GetSequenceControl () << std::dec);
  /* Check if the remaining time is enough to resume previously suspended transmission */
  Time transactionTime = CalculateDmgTransactionDuration (m_currentPacket, m_currentHdr);
  if (transactionTime <= m_txParams.GetMaximumTransmissionDuration ())
    {
      /* This only applies for service period */
//      if (overrideDuration)
//        {
//          m_txParams.EnableOverrideDurationId (duration);
//        }
      CancelAllEvents ();
      m_currentDca = dca;
      SendDataPacket ();

      /* When this method completes, we have taken ownership of the medium. */
      NS_ASSERT_MSG (m_phy->IsStateTx (), "Current State=" << m_phy->GetPhyState ());
    }
  else
    {
      m_transmissionSuspended = true;
    }
}

void
MacLowAd::RestoreAllocationParameters (AllocationID allocationId)
{
  NS_LOG_FUNCTION (this << allocationId);
  m_transmissionSuspended = false;
  m_currentAllocationID = allocationId;
  /* Find the stored parameters for the provided allocation */
  AllocationPeriodsTableCI it = m_allocationPeriodsTable.find (m_currentAllocationID);
  if (it != m_allocationPeriodsTable.end ())
    {
      NS_LOG_DEBUG ("Restored Allocation Parameters for Allocation=" << allocationId);
      m_currentAllocation = it->second;
      m_restoredSuspendedTransmission = false;
    }
  else
    {
      m_restoredSuspendedTransmission = true;
    }
}

void
MacLowAd::StoreAllocationParameters (void)
{
  NS_LOG_FUNCTION (this);
//  CancelAllEvents ();
//  if (m_currentHdr.IsQosData ())
//    {
//      uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
//      FlushAggregateQueue (tid);
//    }
//  m_currentPacket = 0;
//  m_currentDca = 0;
//  m_ampdu = false;
  if ((m_currentPacket != 0) && (!m_currentHdr.IsCtl ()))
    {
      CancelAllEvents ();
      /* Since CurrentPacket is not empty it means we suspended transmission */
      m_currentAllocation.packet = m_currentPacket;
      m_currentAllocation.hdr = m_currentHdr;
      m_currentAllocation.txParams = m_txParams;
      m_currentAllocation.txVector = m_currentTxVector;
      m_currentAllocation.isAmpdu = m_ampdu;
      if (m_ampdu && m_currentHdr.IsQosData ())
        {
          uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
          m_currentAllocation.aggregateQueue = CreateObject<WifiMacQueue> ();
          m_aggregateQueue[tid]->QuickTransfer (m_currentAllocation.aggregateQueue);
        }
      else
        {
          m_currentAllocation.aggregateQueue = 0;
        }
      m_allocationPeriodsTable[m_currentAllocationID] = m_currentAllocation;
      NS_LOG_DEBUG ("IsAmpdu=" << m_currentAllocation.isAmpdu
                    << ", PacketSize=" << m_currentAllocation.packet->GetSize ()
                    << ", seq=0x" << std::hex << m_currentHdr.GetSequenceControl () << std::dec);
    }
  else
    {
      m_allocationPeriodsTable.erase (m_currentAllocationID);
    }
}

bool
MacLowAd::IsTransmissionSuspended (void) const
{
  return m_transmissionSuspended;
}

bool
MacLowAd::RestoredSuspendedTransmission (void) const
{
  return m_restoredSuspendedTransmission;
}

void MacLowAd::StartTransmission (Ptr<const Packet> packet,
                                const WifiMacHeader* hdr,
                                MacLowTransmissionParameters parameters,
                                Ptr<DcaTxop> dca)
{
  if (! m_txParams.MustSendRts () &&
      !((m_ctsToSelfSupported || m_stationManager->GetUseNonErpProtection ()) && NeedCtsToSelf ()))
    {
      if (m_txParams.IsTransmissionBounded ())
        {
          Time transactionTime = CalculateDmgTransactionDuration (m_currentPacket, m_currentHdr);
          NS_LOG_DEBUG ("TransactionTime=" << transactionTime <<
                        ", RemainingTime=" << m_txParams.GetMaximumTransmissionDuration () <<
                        ", CurrentTime=" << Simulator::Now ());
          if (transactionTime <= m_txParams.GetMaximumTransmissionDuration ())
            {
              SendDataPacket ();
            }
          else
            {
              /* We will not take ownership of the medium */
              NS_LOG_DEBUG ("No enough time to complete this DMG transaction");
              /* Save the state of the current transmission */
              m_transmissionSuspended = true;
              StoreAllocationParameters ();
              return;
            }
        }
      else
        {
          SendDataPacket ();
        }
    }
  else
    {
      MacLow::StartTransmission(packet, hdr, parameters, dca);
    }
}

void
MacLowAd::TransmitSingleFrame (Ptr<const Packet> packet,
                             const WifiMacHeader* hdr,
                             MacLowTransmissionParameters params,
                             Ptr<DcaTxop> dca)
{
  NS_LOG_FUNCTION (this << packet << hdr << params << dca);
  m_currentPacket = packet->Copy ();
  m_currentHdr = *hdr;
  CancelAllEvents ();
  m_currentDca = dca;
  m_txParams = params;
  m_currentTxVector = GetDmgTxVector (m_currentPacket, &m_currentHdr);
  m_ampdu = false;
  SendDataPacket ();

  /* When this method completes, we have taken ownership of the medium. */
  NS_ASSERT (m_phy->IsStateTx ());
}

void
MacLowAd::StartTransmission (Ptr<const Packet> packet,
                           const WifiMacHeader* hdr,
                           MacLowTransmissionParameters params,
                           TransmissionOkCallback callback)
{
  NS_LOG_FUNCTION (this << packet << hdr << params);
  m_currentPacket = packet->Copy ();
  m_currentHdr = *hdr;
  CancelAllEvents ();
  m_currentDca = 0;
  m_transmissionCallback = callback;
  m_txParams = params;
  m_currentTxVector = GetDmgTxVector (m_currentPacket, &m_currentHdr);
  m_ampdu = false;
  SendDataPacket ();

  /* When this method completes, we have taken ownership of the medium. */
  NS_ASSERT (m_phy->IsStateTx ());
}

Time
MacLowAd::GetDmgControlDuration (WifiTxVector txVector, uint32_t payloadSize) const
{
  NS_ASSERT (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_DMG_CTRL);
  return m_phy->CalculateTxDuration (payloadSize, txVector, m_phy->GetFrequency ());
}


Time
MacLowAd::GetDmgCtsDuration (void) const
{
  WifiTxVector ctsTxVector = GetDmgControlTxVector ();
  NS_ASSERT (ctsTxVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_DMG_CTRL);
  return m_phy->CalculateTxDuration (GetDmgCtsSize (), ctsTxVector, m_phy->GetFrequency ());
}

uint32_t
MacLowAd::GetDmgCtsSize (void) const
{
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_DMG_CTS);
  return cts.GetSize () + 4;
}

WifiTxVector
MacLowAd::GetCtsToSelfTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
{
  return m_stationManager->GetCtsToSelfTxVector (hdr, packet);
}

WifiTxVector
MacLowAd::GetDmgTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
{
  Mac48Address to = hdr->GetAddr1 ();
  return m_stationManager->GetDmgTxVector (to, hdr, packet);
}

WifiTxVector
MacLowAd::GetDmgControlTxVector (void) const
{
  return m_stationManager->GetDmgControlTxVector ();
}

Time
MacLowAd::CalculateDmgTransactionDuration (Time packetDuration)
{
  NS_LOG_FUNCTION (this << packetDuration);
  Time duration = packetDuration;
  if (m_txParams.MustWaitNormalAck ())
    {
      duration += GetAckDuration (m_currentHdr.GetAddr1 (), m_currentTxVector);
    }
  else if (m_txParams.MustWaitFastAck () || m_txParams.MustWaitSuperFastAck ())
    {
      duration += GetPifs ();
    }
  if (m_txParams.MustWaitBasicBlockAck ())
    {
      duration += GetSifs ();
      WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
      duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, BASIC_BLOCK_ACK);
    }
  else if (m_txParams.MustWaitCompressedBlockAck ())
    {
      duration += GetSifs ();
      WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
      duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
    }
  return duration;
}

Time
MacLowAd::CalculateDmgTransactionDuration (Ptr<Packet> packet, WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  Time duration = m_phy->CalculateTxDuration (GetSize (packet, &hdr),
                                              m_currentTxVector, m_phy->GetFrequency ());
  if (m_txParams.MustWaitNormalAck ())
    {
      duration += GetSifs ();
      duration += GetAckDuration (hdr.GetAddr1 (), m_currentTxVector);
    }
  else if (m_txParams.MustWaitFastAck () || m_txParams.MustWaitSuperFastAck ())
    {
      duration += GetPifs ();
    }
  else if (m_txParams.MustWaitBasicBlockAck ())
    {
      duration += GetSifs ();
      WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
      duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, BASIC_BLOCK_ACK);
    }
  else if (m_txParams.MustWaitCompressedBlockAck ())
    {
      duration += GetSifs ();
      WifiTxVector blockAckReqTxVector = GetBlockAckTxVector (m_currentHdr.GetAddr2 (), m_currentTxVector.GetMode ());
      duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), blockAckReqTxVector, COMPRESSED_BLOCK_ACK);
    }

  /* Convert to MicroSeconds since the duration in the headers are in MicroSeconds */
  return MicroSeconds (ceil ((double) duration.GetNanoSeconds () / 1000));
}

void
MacLowAd::SendDmgCtsAfterRts (Mac48Address source, Time duration, WifiTxVector rtsTxVector, double rtsSnr)
{
  NS_LOG_FUNCTION (this << source << duration << rtsTxVector.GetMode () << rtsSnr);
  /* Send a DMG CTS when we receive a RTS right after SIFS. */
  WifiTxVector ctsTxVector = GetDmgControlTxVector ();
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_DMG_CTS);
  cts.SetDsNotFrom ();
  cts.SetDsNotTo ();
  cts.SetNoMoreFragments ();
  cts.SetNoRetry ();
  cts.SetAddr1 (source);
  cts.SetAddr2 (GetAddress ());
  /* Set duration field */
  duration -= GetDmgCtsDuration ();
  duration -= GetSifs ();
  NS_ASSERT (duration >= MicroSeconds (0));
  cts.SetDuration (duration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (cts);
  WifiMacTrailer fcs;
  packet->AddTrailer (fcs);

  SnrTag tag;
  tag.Set (rtsSnr);
  packet->AddPacketTag (tag);

  ForwardDown (packet, &cts, ctsTxVector);
}

void
MacLowAd::ScheduleCts(WifiTxVector txVector, double rxSnr)
{
  m_sendCtsEvent = Simulator::Schedule (GetSifs (),
                                        &MacLowAd::SendDmgCtsAfterRts, this,
                                        m_lastReceivedHdr->GetAddr2 (),
                                        m_lastReceivedHdr->GetDuration (),
                                        txVector,
                                        rxSnr);
}

void
MacLowAd::DoReceiveOk(Ptr<Packet> packet, double rxSnr, WifiTxVector txVector, bool ampduSubframe)
{
  if ((DynamicCast<WifiMacHeaderAd> (m_lastReceivedHdr)->IsDmgCts ())
             && m_lastReceivedHdr->GetAddr1 () == m_self
             && m_ctsTimeoutEvent.IsRunning ()
             && m_currentPacket != nullptr)
      {
        ReceivedCts(packet, rxSnr, txVector, ampduSubframe);
      }
  else if (DynamicCast<WifiMacHeaderAd> (m_lastReceivedHdr)->IsDMGBeacon ())
    {
      NS_LOG_DEBUG ("Received DMG Beacon with BSSID=" << m_lastReceivedHdr->GetAddr1 ());
      m_stationManager->ReportRxOk (m_lastReceivedHdr->GetAddr1 (), PeekPointer(m_lastReceivedHdr),
                                    rxSnr, txVector.GetMode ());
      ReceivedPacket(packet);
    }
  else if (DynamicCast<WifiMacHeaderAd> (m_lastReceivedHdr)->IsActionNoAck () &&
           m_lastReceivedHdr->GetAddr1 () == m_self)
    {
      NS_LOG_DEBUG ("Received Action No ACK Frame");
      ReceivedPacket(packet);
    }
  else if ((DynamicCast<WifiMacHeaderAd> (m_lastReceivedHdr)->IsSSW () ||
           DynamicCast<WifiMacHeaderAd> (m_lastReceivedHdr)->IsSSW_FBCK () &&)
           m_lastReceivedHdr->GetAddr1 () == m_self)
    {
      NS_LOG_DEBUG ("Received Sector Sweep Frame");
      ReceivedPacket(packet);
    }
}

void
MacLowAd::NotifyNav (Ptr<const Packet> packet, Ptr<const WifiMacHeader> hdr, WifiPreamble preamble)
{
  NS_UNUSED (preamble);
  if (!hdr->IsCfpoll ()
      && hdr->GetAddr2 () != m_bssid
      && hdr->GetAddr1 () != m_self)
    {
      if (DynamicCast<WifiMacHeaderAd>(hdr)->IsGrantFrame ())
        {
          // see section 9.33.7.3 802.11ad-2012
          Ptr<Packet> newPacket = packet->Copy ();
          CtrlDMG_Grant grant;
          newPacket->RemoveHeader (grant);
          Ptr<DmgStaWifiMac> highMac = DynamicCast<DmgStaWifiMac> (m_mac);
          if (grant.GetDynamicAllocationInfo ().GetSourceAID () == highMac->GetAssociationID () ||
              grant.GetDynamicAllocationInfo ().GetDestinationAID () == highMac->GetAssociationID ())
            {
              return;
            }
        }
    }
  MacLow::NotifyNav(packet, hdr, preamble);
}

void
MacLowAd::NotifyDoNavResetNow (Time duration) const
{
  if (m_txParams.IsCbapAccessPeriod ())
    {
      MacLow::NotifyDoNavResetNow(duration);
    }
}

void
MacLow::NotifyDoNavStartNow(Time duration) const
{
  if (m_txParams.IsCbapAccessPeriod ())
    {
      MacLow::NotifyDoNavStartNow(duration);
    }
}

void
MacLowAd::NotifyAckTimeoutStartNow (Time duration) const
{
  if (m_txParams.IsCbapAccessPeriod ())
    {
      MacLow::NotifyAckTimeoutStartNow(duration);
    }
}

void
MacLowAd::NotifyAckTimeoutResetNow () const
{
  if (m_txParams.IsCbapAccessPeriod ())
    {
      MacLow::NotifyAckTimeoutResetNow();
    }
}

void
MacLowAd::NotifyCtsTimeoutStartNow (Time duration) const
{
  if (m_txParams.IsCbapAccessPeriod ())
    {
      MacLow::NotifyCtsTimeoutStartNow(duration);
    }
}

void
MacLowAd::NotifyCtsTimeoutResetNow () const
{
  if (m_txParams.IsCbapAccessPeriod ())
    {
      MacLow::NotifyCtsTimeoutResetNow();
    }
}

void ForwardDown (Ptr<const Packet> packet, const WifiMacHeader *hdr, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << packet << hdr << txVector);
  NS_LOG_DEBUG ("send " << hdr->GetTypeString () <<
                ", to=" << hdr->GetAddr1 () <<
                ", size=" << packet->GetSize () <<
                ", mode=" << txVector.GetMode  () <<
                ", preamble=" << txVector.GetPreambleType () <<
                ", duration=" << hdr->GetDuration () <<
                ", seq=0x" << std::hex << m_currentHdr.GetSequenceControl () << std::dec);

  /* Antenna steering */
  if (m_phy->GetStandard () == WIFI_PHY_STANDARD_80211ad)
    {
      Ptr<DmgWifiMac> wifiMac = DynamicCast<DmgWifiMac> (m_mac);
      /* Change antenna configuration */
      if (((wifiMac->GetCurrentAccessPeriod () == CHANNEL_ACCESS_DTI) && (wifiMac->GetCurrentAllocation () == CBAP_ALLOCATION))
          || (wifiMac->GetCurrentAccessPeriod () == CHANNEL_ACCESS_ATI))
        {
          if ((wifiMac->GetTypeOfStation () == DMG_AP) && (hdr->IsAck () || hdr->IsBlockAck ()))
            {
              wifiMac->SteerTxAntennaToward (hdr->GetAddr1 ());
            }
          else
            {
              wifiMac->SteerAntennaToward (hdr->GetAddr1 ());
            }
        }
      else if (wifiMac->GetTypeOfStation () == DMG_ADHOC)
        {
          if ((hdr->IsAck () || hdr->IsBlockAck ()))
            {
              wifiMac->SteerTxAntennaToward (hdr->GetAddr1 ());
            }
          else
            {
              wifiMac->SteerAntennaToward (hdr->GetAddr1 ());
            }
        }
    }

  if (!m_ampdu || hdr->IsAck () || hdr->IsRts () || hdr->IsBlockAck () || hdr->IsMgt ())
    {
      Time frameDuration = m_phy->CalculateTxDuration (packet->GetSize (), txVector, m_phy->GetFrequency ());
      m_phy->SendPacket (packet, txVector, frameDuration);
    }
  else
    {
      Ptr<Packet> newPacket;
      Ptr <WifiMacQueueItem> dequeuedItem;
      WifiMacHeader newHdr;
      uint8_t queueSize = m_aggregateQueue[GetTid (packet, *hdr)]->GetNPackets ();
      bool singleMpdu = false;
      bool last = false;
      MpduType mpdutype = NORMAL_MPDU;

      uint8_t tid = GetTid (packet, *hdr);
      AcIndex ac = QosUtilsMapTidToAc (tid);
      std::map<AcIndex, Ptr<EdcaTxopN> >::const_iterator edcaIt = m_edca.find (ac);

      if (queueSize == 1)
        {
          singleMpdu = true;
        }

      //Add packet tag
      AmpduTag ampdutag;
      ampdutag.SetAmpdu (true);
      Time delay = Seconds (0);
      if (queueSize > 1 || singleMpdu)
        {
          txVector.SetAggregation (true);
        }

      /* Calculate individual A-MPDU Subframe length in time */
      Time remainingAmpduDuration = NanoSeconds (0);
      std::vector<SubMpduInfo> mpduInfoList;
      WifiPreamble preamble = txVector.GetPreambleType ();
      SubMpduInfo info;

      /* Calculate data part duration only for DMG */
      txVector.SetPreambleType (WIFI_PREAMBLE_NONE);
      Time ampduDuration = m_phy->CalculateTxDuration (packet->GetSize (), txVector, m_phy->GetFrequency ());
      txVector.SetPreambleType (preamble);
//      NS_LOG_DEBUG ("A-MPDU Data Duration=" << ampduDuration << ", Size=" << packet->GetSize ());

      for (; queueSize > 0; queueSize--)
        {
          dequeuedItem = m_aggregateQueue[GetTid (packet, *hdr)]->Dequeue ();
          newHdr = dequeuedItem->GetHeader ();
          newPacket = dequeuedItem->GetPacket ()->Copy ();
          newHdr.SetDuration (hdr->GetDuration ());
          newPacket->AddHeader (newHdr);
          AddWifiMacTrailer (newPacket);
          if (queueSize == 1)
            {
              last = true;
              mpdutype = LAST_MPDU_IN_AGGREGATE;
            }

          edcaIt->second->GetMpduAggregator ()->AddHeaderAndPad (newPacket, last, singleMpdu);

          if (!singleMpdu)
            {
              mpdutype = MPDU_IN_AGGREGATE;
            }
          else
            {
              mpdutype = NORMAL_MPDU;
            }

          info.hdr = newHdr;
          info.packet = newPacket;
          info.type = mpdutype;

          /* Temporary solution to save the remaining of the A-MPDU duration  in the last MPDU */
          if (last)
            {
              txVector.SetPreambleType (preamble);
              info.mpduDuration = m_phy->CalculatePlcpPreambleAndHeaderDuration (txVector) + ampduDuration - remainingAmpduDuration;
            }
          else
            {
              info.mpduDuration = m_phy->CalculatePlcpPreambleAndHeaderDuration (txVector) +
                  NanoSeconds ((double (newPacket->GetSize ()) / double (packet->GetSize ())) * ampduDuration.GetNanoSeconds ());
            }

          //          NS_LOG_DEBUG ("MPDU_Duration=" << info.mpduDuration << ", Size=" << newPacket->GetSize ());
          mpduInfoList.push_back (info);
          remainingAmpduDuration += info.mpduDuration;
          /* Only the first MPDU has preamble */
          txVector.SetPreambleType (WIFI_PREAMBLE_NONE);
        }
//      NS_LOG_DEBUG ("A-MPDU_Duration=" << remainingAmpduDuration);

      /* Send each individual A-MPDU Subframe/Single MPDU */
      queueSize = mpduInfoList.size ();
      txVector.SetPreambleType (preamble);
      for (std::vector<SubMpduInfo>::iterator it = mpduInfoList.begin (); queueSize > 0; it++, queueSize--)
        {
          newPacket = (*it).packet;
          remainingAmpduDuration -= (*it).mpduDuration;
          ampdutag.SetRemainingNbOfMpdus (queueSize - 1);

          if (queueSize > 1)
            {
              ampdutag.SetRemainingAmpduDuration (remainingAmpduDuration);
            }
          else
            {
              ampdutag.SetRemainingAmpduDuration (NanoSeconds (0));
            }
          newPacket->AddPacketTag (ampdutag);

          NS_LOG_DEBUG ("Sending MPDU with Seq " << (*it).hdr.GetSequenceNumber () << " as part of A-MPDU");
          if (delay.IsZero ())
            {
              m_phy->SendPacket (newPacket, txVector, (*it).mpduDuration, (*it).type);
            }
          else
            {
              Simulator::Schedule (delay, &MacLow::SendMpdu, this, newPacket, txVector, (*it).mpduDuration, (*it).type);
            }
          if (queueSize > 1)
            {
              NS_ASSERT (remainingAmpduDuration > 0);
              delay = delay + (*it).mpduDuration;
            }

          txVector.SetPreambleType (WIFI_PREAMBLE_NONE);
        }
    }
}

bool
MacLowAd::ReceiveMpdu (Ptr<Packet> packet, WifiMacHeader hdr)
{
  NS_LOG_DEBUG (this << packet);
  if (m_stationManager->HasDmgSupported ())
    {
      return MacLow::DoReceiveMpdu(packet, hdr);
    }
  else
    {
      return MacLow::ReceiveMpdu (packet, hdr);
    }
}

Time
MacLowAd::GetAPPDUMaxTime() const
{
  if (m_phy->GetStandard () == WIFI_PHY_STANDARD_80211ad)
    {
      return MilliSeconds(2);
    }
  else
    {
      return MacLow::GetAPPDUMaxTime();
    }
}

} // namespace ns3
