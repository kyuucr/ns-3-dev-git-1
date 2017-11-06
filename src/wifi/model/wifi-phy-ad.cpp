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
#include "wifi-phy-ad.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "wifi/model/wifi-phy-tag.h"
#include "wifi/model/wifi-phy-state-helper.h"
#include "abstract-antenna.h"
#include "directional-antenna.h"

namespace ns3 {

TypeId
WifiPhyAd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyAd")
    .SetParent<WifiPhy> ()
    .SetGroupName ("WifiAd")
    .AddAttribute ("SupportOfdmPhy", "Whether the DMG STA supports OFDM PHY layer.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&WifiPhyAd::m_supportOFDM),
                   MakeBooleanChecker ())
    .AddAttribute ("SupportLpScPhy", "Whether the DMG STA supports LP-SC PHY layer.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&WifiPhyAd::m_supportLpSc),
                   MakeBooleanChecker ())
  ;
  return tid;
}


WifiPhyAd::WifiPhyAd()
  : WifiPhy (),
    m_antenna (nullptr),
    m_totalBits (0),
    m_rdsActivated (false),
    m_lastTxDuration (NanoSeconds (0.0))
{

}

WifiPhyAd::~WifiPhyAd()
{

}

void
WifiPhyAd::ConfigureDefaultsForStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);

  if (standard == WIFI_PHY_STANDARD_80211ad)
    {
      SetChannelWidth (2160);
      SetFrequency (58320);
      // Channel number should be aligned by SetFrequency () to 36
      NS_ASSERT (GetChannelNumber () == 1);
    }
  else
    {
      WifiPhy::ConfigureDefaultsForStandard (standard);
    }
}

void
WifiPhyAd::ConfigureStandard (WifiPhyStandard standard)
{
  if (standard == WIFI_PHY_STANDARD_80211a)
    {
      Configure80211ad ();
    }
  else
    {
      WifiPhy::ConfigureStandard(standard);
    }
}

WifiMode
WifiPhyAd::GetPlcpHeaderMode (WifiTxVector txVector) const
{
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
      return WifiPhyAd::GetDMG_MCS0 ();

    case WIFI_MOD_CLASS_DMG_SC:
      return WifiPhyAd::GetDMG_MCS1 ();

    case WIFI_MOD_CLASS_DMG_OFDM:
      return WifiPhyAd::GetDMG_MCS13 ();
    default:
      return WifiPhy::GetPlcpHeaderMode (txVector);
    }
}

Time
WifiPhyAd::GetPlcpHeaderDuration (WifiTxVector txVector) const
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  if (preamble == WIFI_PREAMBLE_NONE)
    {
      return WifiPhy::GetPlcpHeaderDuration (txVector);
    }

  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
      /* From Annex L (L.5.2.5) */
      return NanoSeconds (4654);
    case WIFI_MOD_CLASS_DMG_SC:
    case WIFI_MOD_CLASS_DMG_LP_SC:
      /* From Table 21-4 in 802.11ad spec 21.3.4 */
      return NanoSeconds (582);
    case WIFI_MOD_CLASS_DMG_OFDM:
      /* From Table 21-4 in 802.11ad spec 21.3.4 */
      return NanoSeconds (242);
    default:
      return WifiPhy::GetPlcpHeaderDuration (txVector);
    }
}


Time
WifiPhyAd::GetPlcpPreambleDuration (WifiTxVector txVector) const
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  if (preamble == WIFI_PREAMBLE_NONE)
    {
      return WifiPhyAd::GetPlcpPreambleDuration(txVector);
    }
  switch (txVector.GetMode ().GetModulationClass ())
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
      // CTRL Preamble = (6400 + 1152) Samples * Tc (Chip Time for SC), Tc = Tccp = 0.57ns.
      // CTRL Preamble = 4.291 micro seconds.
      return NanoSeconds (4291);

    case WIFI_MOD_CLASS_DMG_SC:
    case WIFI_MOD_CLASS_DMG_LP_SC:
      // SC Preamble = 3328 Samples * Tc (Chip Time for SC), Tc = 0.57ns.
      // SC Preamble = 1.89 micro seconds.
      return NanoSeconds (1891);

    case WIFI_MOD_CLASS_DMG_OFDM:
      // OFDM Preamble = 4992 Samples * Ts (Chip Time for OFDM), Tc = 0.38ns.
      // OFDM Preamble = 1.89 micro seconds.
      return NanoSeconds (1891);

    default:
      return WifiPhyAd::GetPlcpPreambleDuration(txVector);
    }
}

void
WifiPhyAd::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, MpduType mpdutype)
{
  bool sendTrnFields = false;
  WifiPreamble preamble = txVector.GetPreambleType ();
  Time txDuration = CalculateTxDuration (packet->GetSize (), txVector, GetFrequency (), mpdutype, 1);

  if (((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) ||
       (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE)) && (txVector.GetTrainngFieldLength () > 0))
    {
      sendTrnFields = true;
    }

  WifiPhyAd::SendPacket(packet, txVector, mpdutype);

  /* Send TRN Fields if beam refinement or tracking is required */
  if (sendTrnFields)
    {
      /* Prepare transmission of the first TRN Packet */
      Simulator::Schedule (txDuration, &WifiPhyAd::SendTrnField, this,
                           txVector, txVector.GetTrainngFieldLength ());
    }
  /* Record duration of the current transmission */
  m_lastTxDuration = txDuration;
  /* New Trace for Tx End */
  Simulator::Schedule (txDuration, &WifiPhy::NotifyTxEnd, this, packet);
}

void
WifiPhyAd::StartReceivePreambleAndHeader (Ptr<Packet> packet, double rxPowerW, Time rxDuration)
{
  // TODO: This function is a monstre and should be properly splitted
  // to reuse things from WifiPhy

  //This function should be later split to check separately whether plcp preamble and plcp header can be successfully received.
  //Note: plcp preamble reception is not yet modeled.
  NS_LOG_FUNCTION (this << packet << WToDbm (rxPowerW) << rxDuration);
  WifiPhyTag tag;
  AmpduTag ampduTag;

  bool found = packet->RemovePacketTag (tag);
  if (!found)
    {
      NS_FATAL_ERROR ("Received Wi-Fi Signal with no WifiPhyTag");
      return;
    }

  WifiTxVector txVector = tag.GetWifiTxVector ();
  Time totalDuration = rxDuration + txVector.GetTrainngFieldLength () * TRNUnit;
  m_rxDuration = totalDuration; // Duraion of the last frame
  Time endRx = Simulator::Now () + totalDuration;

  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT
      && (txVector.GetNss () != (1 + (txVector.GetMode ().GetMcsValue () / 8))))
    {
      NS_FATAL_ERROR ("MCS value does not match NSS value: MCS = " << (uint16_t)txVector.GetMode ().GetMcsValue () << ", NSS = " << (uint16_t)txVector.GetNss ());
    }

  if (txVector.GetNss () > GetMaxSupportedRxSpatialStreams ())
    {
      NS_FATAL_ERROR ("Reception ends in failure because of an unsupported number of spatial streams");
    }

  WifiPreamble preamble = txVector.GetPreambleType ();
  MpduType mpdutype = tag.GetMpduType ();
  Time preambleAndHeaderDuration = CalculatePlcpPreambleAndHeaderDuration (txVector);

  Ptr<InterferenceHelper::Event> event;
  event = m_interference.Add (packet->GetSize (),
                              txVector,
                              totalDuration,
                              rxPowerW);

  switch (m_state->GetState ())
    {
    case WifiPhy::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' tramissions started before the end of
       * the switching.
       */
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the completion of the
          //channel switching.
          goto maybeCcaBusy;
        }
      break;
    case WifiPhy::RX:
      NS_LOG_DEBUG ("drop packet " << packet << " because already in Rx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the reception of the
          //currently-received packet.
          goto maybeCcaBusy;
        }
      break;
    case WifiPhy::TX:
      NS_LOG_DEBUG ("drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the transmission of the
          //currently-transmitted packet.
          goto maybeCcaBusy;
        }
      break;
    case WifiPhy::CCA_BUSY:
    case WifiPhy::IDLE:
      if (rxPowerW > GetEdThresholdW ()) //checked here, no need to check in the payload reception (current implementation assumes constant rx power over the packet duration)
        {
          if (m_rdsActivated)
            {
              NS_LOG_DEBUG ("Receiving as RDS in FD-AF Mode");
              /**
               * We are working in Full Duplex-Amplify and Forward. So we receive the packet without decoding it
               * or checking its header. Then we amplify it and redirect it to the the destination.
               * We take a simple approach to model full duplex communication by swapping current steering sector.
               * Another approach would by adding new PHY layer which adds more complexity to the solution.
               */

              if ((mpdutype == NORMAL_MPDU) || (mpdutype == LAST_MPDU_IN_AGGREGATE))
                {
                  if ((m_rdsSector == m_srcSector) && (m_rdsAntenna == m_srcAntenna))
                    {
                      m_rdsSector = m_dstSector;
                      m_rdsAntenna = m_dstAntenna;
                    }
                  else
                    {
                      m_rdsSector = m_srcSector;
                      m_rdsAntenna = m_srcAntenna;
                    }

                  m_directionalAntenna->SetCurrentTxSectorID (m_rdsSector);
                  m_directionalAntenna->SetCurrentTxAntennaID (m_rdsAntenna);
                }

              /* We simply transmit the frame on the channel without passing it to the upper layers (We amplify the power) */
              StartTx (packet, txVector, rxDuration);
            }
          else
            {
              if (preamble == WIFI_PREAMBLE_NONE && (m_mpdusNum == 0 || m_plcpSuccess == false))
                {
                  m_plcpSuccess = false;
                  m_mpdusNum = 0;
                  NS_LOG_DEBUG ("drop packet because no PLCP preamble/header has been received");
                  NotifyRxDrop (packet);
                  goto maybeCcaBusy;
                }
              else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum == 0)
                {
                  //received the first MPDU in an A-MPDU
                  m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
                  m_rxMpduReferenceNumber++;
                }
              else if (preamble == WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
                {
                  //received the other MPDUs that are part of the A-MPDU
                  if (ampduTag.GetRemainingNbOfMpdus () < (m_mpdusNum - 1))
                    {
                      NS_LOG_DEBUG ("Missing MPDU from the A-MPDU " << m_mpdusNum - ampduTag.GetRemainingNbOfMpdus ());
                      m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
                    }
                  else
                    {
                      m_mpdusNum--;
                    }
                }
              else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
                {
                  NS_LOG_DEBUG ("New A-MPDU started while " << m_mpdusNum << " MPDUs from previous are lost");
                  m_mpdusNum = ampduTag.GetRemainingNbOfMpdus ();
                }
              else if (preamble != WIFI_PREAMBLE_NONE && m_mpdusNum > 0 )
                {
                  NS_LOG_DEBUG ("Didn't receive the last MPDUs from an A-MPDU " << m_mpdusNum);
                  m_mpdusNum = 0;
                }

              NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
              //sync to signal
              m_state->SwitchToRx (totalDuration);
              NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
              NotifyRxBegin (packet);
              m_interference.NotifyRxStart ();

              if (preamble != WIFI_PREAMBLE_NONE)
                {
                  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
                  m_endPlcpRxEvent = Simulator::Schedule (preambleAndHeaderDuration, &WifiPhy::StartReceivePacket, this,
                                                          packet, txVector, mpdutype, event);
                }

              NS_ASSERT (m_endRxEvent.IsExpired ());
              if (txVector.GetTrainngFieldLength () == 0)
                {
                  m_endRxEvent = Simulator::Schedule (rxDuration, &WifiPhy::EndPsduReceive, this,
                                                      packet, preamble, mpdutype, event);
                }
              else
                {
                  m_endRxEvent = Simulator::Schedule (rxDuration, &WifiPhy::EndPsduOnlyReceive, this,
                                                      packet, txVector.GetPacketType (), preamble, mpdutype, event);
                }
            }
        }
      else
        {
          NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
                        WToDbm (rxPowerW) << "<" << WToDbm (GetEdThresholdW ()) << ")");
          NotifyRxDrop (packet);
          m_plcpSuccess = false;
          goto maybeCcaBusy;
        }
      break;
    case WifiPhy::SLEEP:
      NS_LOG_DEBUG ("drop packet because in sleep mode");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      break;
    }

  return;

maybeCcaBusy:
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (DbmToW (GetCcaMode1Threshold ()));
  if (!delayUntilCcaEnd.IsZero ())
    {
      NS_LOG_DEBUG ("In CCA Busy State for " << delayUntilCcaEnd);
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
  else
    {
      NS_LOG_DEBUG ("Not in CCA Busy State");
    }
}

Time
WifiPhyAd::CalculateTxDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype, uint8_t incFlag)
{
  Time txDuration;
  WifiPreamble preamble = txVector.GetPreambleType ();

  /* Check if the MPDU is single or last aggregate MPDU */
  if (((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) ||
       (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE)) && (txVector.GetTrainngFieldLength () > 0))
    {
      NS_LOG_DEBUG ("Send TRN Fields:" << txVector.GetTrainngFieldLength ());
      txDuration = txVector.GetTrainngFieldLength () * TRNUnit;
    }

  return txDuration + WifiPhy::CalculateTxDuration(size, txVector, frequency, mpdutype, incFlag);
}

Time
WifiPhyAd::GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype, uint8_t incFlag)
{
  WifiMode payloadMode = txVector.GetMode ();
  WifiPreamble preamble = txVector.GetPreambleType ();
  if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_CTRL)
    {
      if (txVector.GetTrainngFieldLength () == 0)
        {
          uint32_t Ncw;                       /* Number of LDPC codewords. */
          uint32_t Ldpcw;                     /* Number of bits in the second and any subsequent codeword except the last. */
          uint32_t Ldplcw;                    /* Number of bits in the last codeword. */
          uint32_t DencodedSymmbols;          /* Number of differentailly encoded payload symbols. */
          uint32_t Chips;                     /* Number of chips (After spreading using Ga32 Golay Sequence). */
          uint32_t Nbits = (size - 8) * 8;    /* Number of bits in the payload part. */

          Ncw = 1 + (uint32_t) ceil ((double (size) - 6) * 8/168);
          Ldpcw = (uint32_t) ceil ((double (size) - 6) * 8/(Ncw - 1));
          Ldplcw = (size - 6) * 8 - (Ncw - 2) * Ldpcw;
          DencodedSymmbols = (672 - (504 - Ldpcw)) * (Ncw - 2) + (672 - (504 - Ldplcw));
          Chips = DencodedSymmbols * 32;

          /* Make sure the result is in nanoseconds. */
          double ret = double (Chips)/1.76;
          NS_LOG_DEBUG("bits " << Nbits << " Diff encoded Symmbols " << DencodedSymmbols << " rate " << payloadMode.GetDataRate() << " Payload Time " << ret << " ns");

          return NanoSeconds (ceil (ret));
        }
      else
        {
          uint32_t Ncw;                       /* Number of LDPC codewords. */
          Ncw = 1 + (uint32_t) ceil ((double (size) - 6) * 8/168);
          return NanoSeconds (ceil (double ((88 + (size - 6) * 8 + Ncw * 168) * 0.57 * 32)));
        }
    }
  else if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_LP_SC)
    {
//        uint32_t Nbits = (size * 8);  /* Number of bits in the payload part. */
//        uint32_t Nrsc;                /* The total number of Reed Solomon codewords */
//        uint32_t Nrses;               /* The total number of Reed Solomon encoded symbols */
//        Nrsc = (uint32_t) ceil(Nbits/208);
//        Nrses = Nbits + Nrsc * 16;

//        uint32_t Nsbc;                 /* Short Block code Size */
//        if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_13_28)
//          Nsbc = 16;
//        else if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_13_21)
//          Nsbc = 12;
//        else if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_52_63)
//          Nsbc = 9;
//        else if (payloadMode.GetCodeRate() == WIFI_CODE_RATE_13_14)
//          Nsbc = 8;
//        else
//          NS_FATAL_ERROR("unsupported code rate");

//        uint32_t Ncbps;               /* Ncbps = Number of coded bits per symbol. Check Table 21-21 for different constellations. */
//        if (payloadMode.GetConstellationSize() == 2)
//          Ncbps = 336;
//        else if (payloadMode.GetConstellationSize() == 4)
//          Ncbps = 2 * 336;
//          NS_FATAL_ERROR("unsupported constellation size");

//        uint32_t Neb;                 /* Total number of encoded bits */
//        uint32_t Nblks;               /* Total number of 512 blocks containing 392 data symbols */
//        Neb = Nsbc * Nrses;
//        Nblks = (uint32_t) ceil(neb/());
      return NanoSeconds (0);
    }
  else if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_SC)
    {
      /* 21.3.4 Timeing Related Parameters, Table 21-4 TData = (Nblks * 512 + 64) * Tc. */
      /* 21.6.3.2.3.3 (4), Compute Nblks = The number of symbol blocks. */

      uint32_t Ncbpb; // Ncbpb = Number of coded bits per symbol block. Check Table 21-20 for different constellations.
      if (payloadMode.GetConstellationSize () == 2)
        Ncbpb = 448;
      else if (payloadMode.GetConstellationSize () == 4)
        Ncbpb = 2 * 448;
      else if (payloadMode.GetConstellationSize () == 16)
        Ncbpb = 4 * 448;
      else
        NS_FATAL_ERROR("unsupported constellation size");

      uint32_t Nbits = (size * 8); /* Nbits = Number of bits in the payload part. */
      uint32_t Ncbits;             /* Ncbits = Number of coded bits in the payload part. */

      if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_4)
        Ncbits = Nbits * 4;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_2)
        Ncbits = Nbits * 2;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_13_16)
        Ncbits = (uint32_t) ceil (double (Nbits) * 16.0 / 13);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_3_4)
        Ncbits = (uint32_t) ceil (double (Nbits) * 4.0 / 3);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_5_8)
        Ncbits = (uint32_t) ceil (double (Nbits) * 8.0 / 5);
      else
        NS_FATAL_ERROR("unsupported code rate");

      /* We have Lcw = 672 which is the LDPC codeword length. */
      uint32_t Ncw = (uint32_t) ceil (double (Ncbits) / 672.0);         /* Ncw = The number of LDPC codewords.  */
      uint32_t Nblks = (uint32_t) ceil (double (Ncw) * 672.0 / Ncbpb);  /* Nblks = The number of symbol blocks. */

      /* Make sure the result is in nanoseconds. */
      uint32_t tData = lrint (ceil ((double (Nblks) * 512 + 64) / 1.76)); /* The duration of the data part */
      NS_LOG_DEBUG ("bits " << Nbits << " cbits " << Ncbits
                    << " Ncw " << Ncw
                    << " Nblks " << Nblks
                    << " rate " << payloadMode.GetDataRate() << " Payload Time " << tData << " ns");

      if (txVector.GetTrainngFieldLength () != 0)
        {
          if (tData < OFDMSCMin)
            tData = OFDMSCMin;
        }
      return NanoSeconds (tData);
    }
  else if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_DMG_OFDM)
    {
      /* 21.3.4 Timeing Related Parameters, Table 21-4 TData = Nsym * Tsys(OFDM) */
      /* 21.5.3.2.3.3 (5), Compute Nsym = Number of OFDM Symbols */

      uint32_t Ncbps; // Ncbpb = Number of coded bits per symbol. Check Table 21-20 for different constellations.
      if (payloadMode.GetConstellationSize () == 2)
        Ncbps = 336;
      else if (payloadMode.GetConstellationSize () == 4)
        Ncbps = 2 * 336;
      else if (payloadMode.GetConstellationSize () == 16)
        Ncbps = 4 * 336;
      else if (payloadMode.GetConstellationSize () == 64)
        Ncbps = 6 * 336;
      else
        NS_FATAL_ERROR("unsupported constellation size");

      uint32_t Nbits = (size * 8); /* Nbits = Number of bits in the payload part. */
      uint32_t Ncbits;             /* Ncbits = Number of coded bits in the payload part. */

      if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_4)
        Ncbits = Nbits * 4;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_1_2)
        Ncbits = Nbits * 2;
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_13_16)
        Ncbits = (uint32_t) ceil (double (Nbits) * 16.0 / 13);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_3_4)
        Ncbits = (uint32_t) ceil (double (Nbits) * 4.0 / 3);
      else if (payloadMode.GetCodeRate () == WIFI_CODE_RATE_5_8)
        Ncbits = (uint32_t) ceil (double (Nbits) * 8.0 / 5);
      else
        NS_FATAL_ERROR ("unsupported code rate");

      uint32_t Ncw = (uint32_t) ceil (double (Ncbits) / 672.0);         /* Ncw = The number of LDPC codewords.  */
      uint32_t Nsym = (uint32_t) ceil (double (Ncw * 672.0) / Ncbps);   /* Nsym = Number of OFDM Symbols. */

      /* Make sure the result is in nanoseconds */
      uint32_t tData;       /* The duration of the data part */
      tData = Nsym * 242;   /* Tsys(OFDM) = 242ns */
      NS_LOG_DEBUG ("bits " << Nbits << " cbits " << Ncbits << " rate " << payloadMode.GetDataRate() << " Payload Time " << tData << " ns");

      if (txVector.GetTrainngFieldLength () != 0)
        {
          if (tData < OFDMBRPMin)
            tData = OFDMBRPMin;
        }
      return NanoSeconds (tData);
    }

  return WifiPhy::GetPayloadDuration(size, txVector, frequency, mpdutype, incFlag);
}

void
WifiPhyAd::Configure80211ad (void)
{
  NS_LOG_FUNCTION (this);

  /* CTRL-PHY */
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS0 ());

  /* SC-PHY */
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS1 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS2 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS3 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS4 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS5 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS6 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS7 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS8 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS9 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS10 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS11 ());
  m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS12 ());

  /* OFDM-PHY */
  if (m_supportOFDM)
    {
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS13 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS14 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS15 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS16 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS17 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS18 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS19 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS20 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS21 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS22 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS23 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS24 ());
    }

  /* LP-SC PHY */
  if (m_supportLpSc)
    {
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS25 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS26 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS27 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS28 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS29 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS30 ());
      m_deviceRateSet.push_back (WifiPhyAd::GetDMG_MCS31 ());
    }
}

uint64_t
WifiPhyAd::GetTotalTransmittedBits () const
{
  return m_totalBits;
}

Time
WifiPhyAd::GetLastRxDuration (void) const
{
  return m_rxDuration;
}

void
WifiPhyAd::SetAntenna (Ptr<AbstractAntenna> antenna)
{
  m_antenna = antenna;
}

Ptr<AbstractAntenna>
WifiPhyAd::GetAntenna (void) const
{
  return m_antenna;
}

void
WifiPhyAd::SetDirectionalAntenna (Ptr<DirectionalAntenna> antenna)
{
  m_directionalAntenna = antenna;
}

Ptr<DirectionalAntenna>
WifiPhyAd::GetDirectionalAntenna (void) const
{
  return m_directionalAntenna;
}

void
WifiPhyAd::EndPsduReceive (Ptr<Packet> packet, WifiPreamble preamble, MpduType mpdutype, Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpPayloadSnrPer (event);
  m_interference.NotifyRxEnd ();

  if (m_plcpSuccess == true)
    {
      NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate (event->GetTxVector ())) <<
                    ", snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
      double rnd = m_random->GetValue ();
      if (rnd > snrPer.per)
        {
          NotifyRxEnd (packet);
          SignalNoiseDbm signalNoise;
          signalNoise.signal = RatioToDb (event->GetRxPowerW ()) + 30;
          signalNoise.noise = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
          MpduInfo aMpdu;
          aMpdu.type = mpdutype;
          aMpdu.mpduRefNumber = m_rxMpduReferenceNumber;
          NotifyMonitorSniffRx (packet, GetFrequency (), event->GetTxVector (), aMpdu, signalNoise);
          m_state->SwitchFromRxEndOk (packet, snrPer.snr, event->GetTxVector ());
        }
      else
        {
          /* failure. */
          NS_LOG_DEBUG ("drop packet because the probability to receive it = " << rnd << " is lower than " << snrPer.per);
          NotifyRxDrop (packet);
          m_state->SwitchFromRxEndError (packet, snrPer.snr);
        }
    }
  else
    {
      m_state->SwitchFromRxEndError (packet, snrPer.snr);
    }

  if (preamble == WIFI_PREAMBLE_NONE && mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      m_plcpSuccess = false;
    }
}

void
WifiPhyAd::EndPsduOnlyReceive (Ptr<Packet> packet, PacketType packetType, WifiPreamble preamble, MpduType mpdutype, Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);
  NS_ASSERT (IsStateRx ());

  bool isEndOfFrame = ((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) || (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE));
  struct InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpPayloadSnrPer (event);

  if (m_plcpSuccess == true)
    {
      NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate ()) <<
                    ", snr(dB)=" << RatioToDb(snrPer.snr) << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
      double rnd = m_random->GetValue ();
      m_psduSuccess = (rnd > snrPer.per);
      if (m_psduSuccess)
        {
          NotifyRxEnd (packet);
          SignalNoiseDbm signalNoise;
          signalNoise.signal = RatioToDb (event->GetRxPowerW ()) + 30;
          signalNoise.noise = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
          MpduInfo aMpdu;
          aMpdu.type = mpdutype;
          aMpdu.mpduRefNumber = m_rxMpduReferenceNumber;
          NotifyMonitorSniffRx (packet, GetFrequency (), event->GetTxVector (), aMpdu, signalNoise);
          m_state->ReportPsduEndOk (packet, snrPer.snr, event->GetTxVector ());
        }
      else
        {
          /* failure. */
          NS_LOG_DEBUG ("drop packet because the probability to receive it = " << rnd << " is lower than " << snrPer.per);
          NotifyRxDrop (packet);
          m_state->ReportPsduEndError (packet, snrPer.snr);
        }
    }
  else
    {
      m_state->ReportPsduEndError (packet, snrPer.snr);
    }

  if (preamble == WIFI_PREAMBLE_NONE && mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      m_plcpSuccess = false;
    }

  if (isEndOfFrame && (packetType == TRN_R))
    {
      /* If the received frame has TRN-R Fields, we should sweep antenna configuration at the beginning of each field */
      m_directionalAntenna->SetInDirectionalReceivingMode ();
      m_directionalAntenna->SetCurrentRxSectorID (1);
      m_directionalAntenna->SetCurrentRxAntennaID (1);
    }
}

void
WifiPhyAd::PacketTxEnd (Ptr<Packet> packet)
{
  m_lastTxDuration = NanoSeconds (0);
  NotifyTxEnd (packet);
}

Time
WifiPhyAd::GetLastTxDuration (void) const
{
  return m_lastTxDuration;
}

void
WifiPhyAd::SendTrnField (WifiTxVector txVector, uint8_t fieldsRemaining)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << uint (fieldsRemaining));
  if (txVector.GetPacketType () == TRN_T)
    {
      /* Change Sector ID at the begining of each TRN-T field */
      m_directionalAntenna->SetCurrentTxSectorID (fieldsRemaining);
    }

  fieldsRemaining--;
  if (fieldsRemaining != 0)
    {
      Simulator::Schedule (TRNUnit, &WifiPhyAd::SendTrnField, this, txVector, fieldsRemaining);
    }

  StartTrnTx (txVector, fieldsRemaining);
}

void
WifiPhyAd::StartReceiveTrnField (WifiTxVector txVector, double rxPowerDbm, uint8_t fieldsRemaining)
{
  NS_LOG_FUNCTION (this << txVector.GetMode () << rxPowerDbm << fieldsRemaining);
  double rxPowerW = DbmToW (rxPowerDbm);
  if (m_plcpSuccess && m_state->IsStateRx ())
    {
      /* Add Interference event for TRN field */
      Ptr<InterferenceHelper::Event> event;
      event = m_interference.Add (txVector,
                                  TRNUnit,
                                  rxPowerW);

      /* Schedule an event for the complete reception of this TRN Field */
      Simulator::Schedule (TRNUnit, &WifiPhyAd::EndReceiveTrnField, this,
                           m_directionalAntenna->GetCurrentRxSectorID (), m_directionalAntenna->GetCurrentRxAntennaID (),
                           txVector, fieldsRemaining, event);

      if (txVector.GetPacketType () == TRN_R)
        {
          /* Change Rx Sector for the next TRN Field */
          m_directionalAntenna->SetCurrentRxSectorID (m_directionalAntenna->GetNextRxSectorID ());
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop TRN Field because signal power too Small (" << rxPowerW << "<" << GetEdThresholdW ());
    }
}

void
WifiPhyAd::EndReceiveTrnField (uint8_t sectorId, uint8_t antennaId,
                                 WifiTxVector txVector, uint8_t fieldsRemaining,
                                 Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << sectorId << antennaId << txVector.GetMode () << fieldsRemaining << event);

  /* Calculate SNR and report it to the upper layer */
  double snr;
  snr = m_interference.CalculatePlcpTrnSnr (event);
  m_reportSnrCallback (sectorId, antennaId, fieldsRemaining, snr, (txVector.GetPacketType () == TRN_T));

  /* Check if this is the last TRN field in the current transmission */
  if (fieldsRemaining == 0)
    {
      EndReceiveTrnFields ();
    }
}

void
WifiPhyAd::EndReceiveTrnFields (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (IsStateRx ());
  m_interference.NotifyRxEnd ();

  if (m_plcpSuccess && m_psduSuccess)
    {
      m_state->SwitchFromRxEndOk ();
    }
  else
    {
      m_state->SwitchFromRxEndError ();
    }
}

void
WifiPhyAd::RegisterReportSnrCallback (ReportSnrCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_reportSnrCallback = callback;
}

void
WifiPhyAd::ActivateRdsOpereation (uint8_t srcSector, uint8_t srcAntenna,
                                    uint8_t dstSector, uint8_t dstAntenna)
{
  NS_LOG_FUNCTION (this << srcSector << srcAntenna << dstSector << dstAntenna);
  m_rdsActivated = true;
  m_rdsSector = m_srcSector = srcSector;
  m_rdsAntenna = m_srcAntenna = srcAntenna;
  m_dstSector = dstSector;
  m_dstAntenna = dstAntenna;
}

void
WifiPhyAd::ResumeRdsOperation (void)
{
  NS_LOG_FUNCTION (this);
  m_rdsActivated = true;
  m_rdsSector = m_srcSector;
  m_rdsAntenna = m_srcAntenna;
}

void
WifiPhyAd::SuspendRdsOperation (void)
{
  NS_LOG_FUNCTION (this);
  m_rdsActivated = false;
}

/* Channel Measurement Functions for 802.11ad-2012 */

void
WifiPhyAd::StartMeasurement (uint16_t measurementDuration, uint8_t blocks)
{
  m_measurementBlocks = blocks;
  m_measurementUnit = floor (measurementDuration/blocks);
  Simulator::Schedule (MicroSeconds (measurementDuration), &WifiPhyAd::EndMeasurement, this);
}

void
WifiPhyAd::MeasurementUnitEnded (void)
{
  /* Add measurement to the list of measurements */
  m_measurementList.push_back (m_measurementBlocks);
  m_measurementBlocks--;
  if (m_measurementBlocks > 0)
    {
      /* Schedule new measurement Unit */
      Simulator::Schedule (MicroSeconds (m_measurementUnit), &WifiPhyAd::EndMeasurement, this);
    }
}

void
WifiPhyAd::EndMeasurement (void)
{
  m_reportMeasurementCallback (m_measurementList);
}

void
WifiPhyAd::RegisterMeasurementResultsReady (ReportMeasurementCallback callback)
{
  m_reportMeasurementCallback = callback;
}

WifiPhyAd::State
WifiPhyAd::GetPhyState (void) const
{
  return m_state->GetState ();
}

/*
 * 802.11ad PHY Layer Rates (Clause 21 Rates)
 */

/* DMG Control PHY */
WifiMode
WifiPhyAd::GetDMG_MCS0 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS0", 0,
                                     WIFI_MOD_CLASS_DMG_CTRL,
                                     true,
                                     2160000000, 27500000,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

/* DMG SC PHY */
WifiMode
WifiPhyAd::GetDMG_MCS1 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS1", 1,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true,
                                     2160000000, 385000000,
                                     WIFI_CODE_RATE_1_4, /* 2 repetition */
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS2 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS2", 2,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true,
                                     2160000000, 770000000,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS3 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS3", 3,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true,
                                     2160000000, 962500000,
                                     WIFI_CODE_RATE_5_8,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS4 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS4", 4,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     true, /* VHT SC MCS1-4 mandatory*/
                                     2160000000, 1155000000,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS5 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS5", 5,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 1251250000,
                                     WIFI_CODE_RATE_13_16,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS6 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS6", 6,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 1540000000,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS7 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS7", 7,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 1925000000,
                                     WIFI_CODE_RATE_5_8,
                                     4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS8 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS8", 8,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 2310000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS9 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS9", 9,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 2502500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS10 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS10", 10,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 3080000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS11 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS11", 11,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 3850000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     16);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS12 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS12", 12,
                                     WIFI_MOD_CLASS_DMG_SC,
                                     false,
                                     2160000000, 4620000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

/**** OFDM MCSs BELOW ****/
WifiMode
WifiPhyAd::GetDMG_MCS13 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS13", 13,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     true,
                                     2160000000, 693000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS14 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS14", 14,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 866250000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS15 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS15", 15,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 1386000000ULL,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS16 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS16", 16,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 1732500000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS17 ()
{
  static WifiMode mode =
  WifiModeFactory::CreateWifiMode ("DMG_MCS17", 17,
                                   WIFI_MOD_CLASS_DMG_OFDM,
                                   false,
                                   2160000000, 2079000000ULL,
                                   WIFI_CODE_RATE_3_4,
                                   4);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS18 ()
{
  static WifiMode mode =
  WifiModeFactory::CreateWifiMode ("DMG_MCS18", 18,
                                   WIFI_MOD_CLASS_DMG_OFDM,
                                   false,
                                   2160000000, 2772000000ULL,
                                   WIFI_CODE_RATE_1_2,
                                   16);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS19 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS19", 19,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 3465000000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     16);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS20 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS20", 20,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 4158000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS21 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS21", 21,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 4504500000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     16);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS22 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS22", 22,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 5197500000ULL,
                                     WIFI_CODE_RATE_5_8,
                                     64);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS23 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS23", 23,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 6237000000ULL,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS24 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS24", 24,
                                     WIFI_MOD_CLASS_DMG_OFDM,
                                     false,
                                     2160000000, 6756750000ULL,
                                     WIFI_CODE_RATE_13_16,
                                     64);
  return mode;
}

/**** Low Power SC MCSs ****/
WifiMode
WifiPhyAd::GetDMG_MCS25 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS25", 25,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 626000000,
                                     WIFI_CODE_RATE_13_28,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS26 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS26", 26,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 834000000,
                                     WIFI_CODE_RATE_13_21,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS27 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS27", 27,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 1112000000ULL,
                                     WIFI_CODE_RATE_52_63,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS28 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS28", 28,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 1251000000ULL,
                                     WIFI_CODE_RATE_13_28,
                                     2);
  return mode;
}

WifiMode
WifiPhyAd::GetDMG_MCS29 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS29", 29,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 1668000000ULL,
                                     WIFI_CODE_RATE_13_21,
                                     4);
  return mode;
}


WifiMode
WifiPhyAd::GetDMG_MCS30 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS30", 30,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 2224000000ULL,
                                     WIFI_CODE_RATE_52_63,
                                     4);
  return mode;
}


WifiMode
WifiPhyAd::GetDMG_MCS31 ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DMG_MCS31", 31,
                                     WIFI_MOD_CLASS_DMG_LP_SC,
                                     false,
                                     2160000000, 2503000000ULL,
                                     WIFI_CODE_RATE_13_14,
                                     4);
  return mode;
}

} // namespace ns3
