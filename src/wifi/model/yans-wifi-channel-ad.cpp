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
#include "yans-wifi-channel-ad.h"
#include "wifi-ad/model/directional-antenna.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiChannelAd");

NS_OBJECT_ENSURE_REGISTERED (YansWifiChannelAd);

TypeId
YansWifiChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::YansWifiChannelAd")
    .SetParent<YansWifiChannel> ()
    .SetGroupName ("WifiAd")
    .AddConstructor<YansWifiChannelAd> ()
  ;
  return tid;
}

YansWifiChannelAd::YansWifiChannelAd() :
  YansWifiChannel (),
  m_blockage (nullptr),
  m_packetDropper (nullptr),
  m_experimentalMode (false)
{
  NS_LOG_FUNCTION (this);
}

YansWifiChannelAd::~YansWifiChannelAd ()
{
  NS_LOG_FUNCTION (this);
}


void
YansWifiChannelAd::AddBlockage (double (*blockage)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  m_blockage = blockage;
  m_srcWifiPhy = srcWifiPhy;
  m_dstWifiPhy = dstWifiPhy;
}

void
YansWifiChannelAd::RemoveBlockage (void)
{
  m_blockage = nullptr;
  m_srcWifiPhy = nullptr;
  m_dstWifiPhy = nullptr;
}

void
YansWifiChannelAd::AddPacketDropper (bool (*dropper)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  m_packetDropper = dropper;
  m_srcWifiPhy = srcWifiPhy;
  m_dstWifiPhy = dstWifiPhy;
}

void
YansWifiChannelAd::RemovePacketDropper (void)
{
  m_packetDropper = nullptr;
  m_srcWifiPhy = nullptr;
  m_dstWifiPhy = nullptr;
}

void
YansWifiChannelAd::LoadReceivedSignalStrengthFile (std::string fileName, Time updateFreuqnecy)
{
  NS_LOG_FUNCTION (this << "Loading Received signal strength file" << fileName);
  std::ifstream file;
  file.open (fileName.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (file.good (), " file not found");
  std::string line;
  double strength;
  while (std::getline (file, line))
    {
      std::stringstream value (line);
      value >> strength;
      m_receivedSignalStrength.push_back (strength);
    }
  m_currentSignalStrengthIndex = 0;
  m_experimentalMode = true;
  m_updateFrequency = updateFreuqnecy;
  /* Schedule Update event */
  Simulator::Schedule (m_updateFrequency, &YansWifiChannelAd::UpdateSignalStrengthValue, this);
}

void
YansWifiChannelAd::UpdateSignalStrengthValue (void)
{
  NS_LOG_FUNCTION (this);
  m_currentSignalStrengthIndex++;
  if (m_currentSignalStrengthIndex == m_receivedSignalStrength.size ())
    {
      m_currentSignalStrengthIndex = 0;
    }
  Simulator::Schedule (m_updateFrequency, &YansWifiChannelAd::UpdateSignalStrengthValue, this);
}

void
YansWifiChannelAd::Send(Ptr<YansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, Time duration)
{
  NS_LOG_FUNCTION (this << sender << packet << txPowerDbm << duration.GetSeconds ());
  Ptr<MobilityModel> senderMobility = sender->GetMobility ();
  NS_ASSERT (senderMobility != nullptr);
  uint32_t j = 0; /* Phy ID */
  Vector sender_pos = senderMobility->GetPosition ();
  Ptr<DirectionalAntenna> senderAnt = sender->GetDirectionalAntenna ();
  double rxPowerDbm;
  Time delay; /* Propagation delay of the signal */
  Ptr<MobilityModel> receiverMobility;

  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          //For now don't account for inter channel interference nor channel bonding
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          /* Packet Dropper */
          if ((m_packetDropper != 0) && ((m_srcWifiPhy == sender) && (m_dstWifiPhy == (*i))))
            {
              if (m_packetDropper ())
                {
                  continue;
                }
            }

          receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          double azimuthTx = CalculateAzimuthAngle (sender_pos, receiverMobility->GetPosition ());
          double azimuthRx = CalculateAzimuthAngle (receiverMobility->GetPosition (), sender_pos);

          delay = m_delay->GetDelay (senderMobility, receiverMobility);

          if (senderAnt != 0)
            {
              NS_LOG_DEBUG ("POWER: azimuthTx=" << azimuthTx
                            << ", azimuthRx=" << azimuthRx
                            << ", txPowerDbm=" << txPowerDbm
                            << ", RxPower=" << m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility)
                            << ", Gtx=" << senderAnt->GetTxGainDbi (azimuthTx)
                            << ", Grx=" << (*i)->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx));

              if (m_experimentalMode)
                {
                  rxPowerDbm = m_receivedSignalStrength[m_currentSignalStrengthIndex];
                }
              else
                {
                  rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) +
                               senderAnt->GetTxGainDbi (azimuthTx) +                            // Sender's antenna gain.
                               (*i)->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx);        // Receiver's antenna gain.
                }

              /* External Attenuator */
              if ((m_blockage != 0) &&
                  (((m_srcWifiPhy == sender) && (m_dstWifiPhy == (*i))) ||
                   ((m_srcWifiPhy == (*i)) && (m_dstWifiPhy == sender))))
                {
                  NS_LOG_DEBUG ("Blockage is inserted");
                  rxPowerDbm += m_blockage ();
                }
            }
          else
            {
              rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) ;
            }

          NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm, " <<
                        "distance=" << senderMobility->GetDistanceFrom (receiverMobility) << "m, delay=" << delay);

          Ptr<Packet> copy = packet->Copy ();   /* Copy of the packet to be received. */
          Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
          uint32_t dstNode;	/* Destination node (Receiver) */
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
            }

          /* We are sending PSDU Packet */
          NS_LOG_DEBUG ("Receiving Node ID=" << dstNode);

	  Simulator::ScheduleWithContext (dstNode,
					  delay, &YansWifiChannel::Receive, this,
					  (*i), copy, rxPowerDbm, duration);
	}
    }
}

void
YansWifiChannelAd::SendTrn (Ptr<YansWifiPhy> sender, double txPowerDbm, WifiTxVector txVector, uint8_t fieldsRemaining) const
{
  NS_LOG_FUNCTION (this << sender << txPowerDbm << txVector << uint (fieldsRemaining));
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT (senderMobility != nullptr);
  Ptr<MobilityModel> receiverMobility;
  uint32_t j = 0; /* Phy ID */
  Time delay; /* Propagation delay of the signal */
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender != (*i))
        {
          // For now don't account for inter channel interference.
          if ((*i)->GetChannelNumber () != sender->GetChannelNumber ())
            {
              continue;
            }

          receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
          delay = m_delay->GetDelay (senderMobility, receiverMobility);

          NS_LOG_DEBUG ("propagation: distance=" << senderMobility->GetDistanceFrom (receiverMobility)
                        << "m, delay=" << delay);

          Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
          uint32_t dstNode;	/* Destination node (Receiver) */
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetNode ()->GetId ();
            }

          Simulator::ScheduleWithContext (dstNode, delay, &YansWifiChannel::ReceiveTrn, this, j,
                                          sender, txVector, txPowerDbm, fieldsRemaining);
        }
    }
}

void
YansWifiChannelAd::ReceiveTrn (uint32_t i, Ptr<YansWifiPhy> sender, WifiTxVector txVector,
                               double txPowerDbm, uint8_t fieldsRemaining) const
{
  NS_LOG_FUNCTION (this << i << sender << txVector << txPowerDbm<< uint (fieldsRemaining));
  /* Calculate SNR upon the receiption of the TRN Field */
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  Ptr<MobilityModel> receiverMobility = m_phyList[i]->GetMobility ()->GetObject<MobilityModel> ();
  NS_ASSERT ((senderMobility != 0) && (receiverMobility != 0));
  Ptr<DirectionalAntenna> senderAnt = sender->GetDirectionalAntenna ();
  double azimuthTx = CalculateAzimuthAngle (senderMobility->GetPosition (), receiverMobility->GetPosition ());
  double azimuthRx = CalculateAzimuthAngle (receiverMobility->GetPosition (), senderMobility->GetPosition ());
  double rxPowerDbm;

  NS_LOG_DEBUG ("POWER: azimuthTx=" << azimuthTx
                << ", azimuthRx=" << azimuthRx
                << ", RxPower=" << m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility)
                << ", Gtx=" << senderAnt->GetTxGainDbi (azimuthTx)
                << ", Grx=" << m_phyList[i]->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx));

  rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility) +
               senderAnt->GetTxGainDbi (azimuthTx) +                                      // Sender's antenna gain.
               m_phyList[i]->GetDirectionalAntenna ()->GetRxGainDbi (azimuthRx);          // Receiver's antenna gain.

  /* External Attenuator */
  if ((m_blockage != 0) && (m_srcWifiPhy == sender) && (m_dstWifiPhy == m_phyList[i]))
    {
      rxPowerDbm += m_blockage ();
    }

  NS_LOG_DEBUG ("propagation: txPower=" << txPowerDbm << "dbm, rxPower=" << rxPowerDbm << "dbm");

  /* Report the received SNR to the higher layers */
  m_phyList[i]->StartReceiveTrnField (txVector, rxPowerDbm, fieldsRemaining);
}

} // namespace ns3
