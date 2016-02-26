/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Natale Patriciello <natale.patriciello@gmail.com>
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
 */

#include "../internet/test/tcp-general-test.h"
#include "ns3/point-to-point-module.h"
#include "ns3/antenna-module.h"

#include "ns3/propagation-module.h"
#include "ns3/mobility-module.h"
#include "ns3/test.h"
#include "ns3/internet-module.h"
#include "ns3/node-container.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"
#include "ns3/tcp-l4-protocol.h"
#include "../internet/model/ipv4-end-point.h"
#include "../internet/model/ipv6-end-point.h"
#include "ns3/lte-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpBufferSizeTestSuite");

class TcpSocketHighBuf : public TcpSocketMsgBase
{
public:
  static TypeId GetTypeId ();
  TcpSocketHighBuf () : TcpSocketMsgBase () { }
  TcpSocketHighBuf (const TcpSocketHighBuf &other) : TcpSocketMsgBase (other) { }

protected:
  virtual void ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader);
  virtual Ptr<TcpSocketBase> Fork (void) { return CopyObject<TcpSocketHighBuf> (this); }
};

TypeId
TcpSocketHighBuf::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketHighBuf")
    .SetParent<TcpSocketMsgBase> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpSocketHighBuf> ()
  ;
  return tid;
}

void
TcpSocketHighBuf::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  NS_LOG_DEBUG ("Data segment, seq=" << tcpHeader.GetSequenceNumber () <<
                " pkt size=" << p->GetSize () );

  // Put into Rx buffer
  SequenceNumber32 expectedSeq = m_rxBuffer->NextRxSequence ();
  if (!m_rxBuffer->Add (p, tcpHeader))
    { // Insert failed: No data or RX buffer full
      SendEmptyPacket (TcpHeader::ACK);
      return;
    }
  // Now send a new ACK packet acknowledging all received and delivered data
  if (m_rxBuffer->Size () > m_rxBuffer->Available () || m_rxBuffer->NextRxSequence () > expectedSeq + p->GetSize ())
    { // A gap exists in the buffer, or we filled a gap: Always ACK
      SendEmptyPacket (TcpHeader::ACK);
    }
  else
    { // In-sequence packet: ACK if delayed ack count allows
      if (++m_delAckCount >= m_delAckMaxCount)
        {
          m_delAckEvent.Cancel ();
          m_delAckCount = 0;
          SendEmptyPacket (TcpHeader::ACK);
        }
      else if (m_delAckEvent.IsExpired ())
        {
          m_delAckEvent = Simulator::Schedule (m_delAckTimeout,
                                               &TcpSocketHighBuf::DelAckTimeout, this);
          NS_LOG_LOGIC (this << " scheduled delayed ACK at " <<
                        (Simulator::Now () + Simulator::GetDelayLeft (m_delAckEvent)).GetSeconds ());
        }
    }
  // Notify app to receive if necessary
  if (expectedSeq < m_rxBuffer->NextRxSequence ())
    { // NextRxSeq advanced, we have something to send to the app
      if (!m_shutdownRecv && m_rxBuffer->Available () > 4000000)
        {
          NotifyDataRecv ();
        }
      else
        {
          NS_LOG_DEBUG ("Queueing " << m_rxBuffer->Available () << " bytes");
        }
      // Handle exceptions
      if (m_closeNotified)
        {
          NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
        }
      // If we received FIN before and now completed all "holes" in rx buffer,
      // invoke peer close procedure
      if (m_rxBuffer->Finished () && (tcpHeader.GetFlags () & TcpHeader::FIN) == 0)
        {
          DoPeerClose ();
        }
    }
}

class TcpBufferSizeTest : public TcpGeneralTest
{
public:
  TcpBufferSizeTest (const std::string &desc);

protected:
  virtual Ptr<TcpSocketMsgBase> CreateReceiverSocket (Ptr<Node> node);
  virtual Ptr<TcpSocketMsgBase> CreateSenderSocket (Ptr<Node> node);
  void ConfigureEnvironment ();
  void ConfigureProperties ();
  virtual Ipv4Address MakeEnvironment (Ptr<Node> sender, Ptr<Node> receiver);
};

TcpBufferSizeTest::TcpBufferSizeTest (const std::string &desc)
  : TcpGeneralTest (desc)
{
}

void
TcpBufferSizeTest::ConfigureEnvironment ()
{
  NS_LOG_FUNCTION (this);
  TcpGeneralTest::ConfigureEnvironment ();
  SetPropagationDelay (MilliSeconds (50));
  SetAppPktSize (1340);
  SetAppPktCount (60000);
  SetConnectTime (Seconds (4.0));
}

void
TcpBufferSizeTest::ConfigureProperties ()
{
  TcpGeneralTest::ConfigureProperties ();
  SetRcvBufSize (RECEIVER, 6000000);
  SetSegmentSize (SENDER, 1340);
  SetSegmentSize (RECEIVER, 1340);
}

Ptr<TcpSocketMsgBase>
TcpBufferSizeTest::CreateReceiverSocket (Ptr<Node> node)
{
  return CreateSocket (node, TcpSocketHighBuf::GetTypeId (), m_congControlTypeId);
}

Ptr<TcpSocketMsgBase>
TcpBufferSizeTest::CreateSenderSocket (Ptr<Node> node)
{
  return CreateSocket (node, TcpSocketHighBuf::GetTypeId (), m_congControlTypeId);
}

static void
SetNodeProperties (Ptr<Node> node, double x, double y, double z, const std::string &name)
{
  Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel> ();
  if (loc == 0)
    {
      loc = CreateObject<ConstantPositionMobilityModel> ();
      node->AggregateObject (loc);
    }
  loc->SetPosition (Vector3D (x, y, z));  // 5, 2
  Names::Add (name, node);
}

static Ipv4InterfaceContainer
BuildIpv4LAN (NetDeviceContainer &devs, const std::string &baseAddr = "10.0.0.0")
{
  static Ipv4AddressHelper address;
  address.SetBase (baseAddr.c_str (), "255.255.0.0");
  address.NewNetwork ();

  return address.Assign (devs);
}

Ipv4Address
TcpBufferSizeTest::MakeEnvironment (Ptr<Node> sender, Ptr<Node> receiver)
{
  static Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  static Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (1575));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (19575));
  lteHelper->SetUeAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (1575));
  lteHelper->SetPathlossModelType ("ns3::OkumuraHataPropagationLossModel");
  lteHelper->SetPathlossModelAttribute ("Environment", EnumValue (UrbanEnvironment));
  lteHelper->SetPathlossModelAttribute ("CitySize", EnumValue (MediumCity));
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  SetNodeProperties (pgw, 50, 20, 2, "pgw");

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Add (receiver);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  //add a position to the Remote Host node
  SetNodeProperties (remoteHost, 70, 20, 2, "remote");

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("500Mb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (5)));

  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting;

  remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->SetDefaultRoute (internetIpIfaces.GetAddress (0), 1);

  // Create the ENBs and set position
  NodeContainer enbNodes;
  enbNodes.Create (1);
  for (uint32_t i = 0; i < 1; ++i)
    {
      std::stringstream n;
      n << "ENB" << i+1;
      SetNodeProperties (enbNodes.Get (i), 10, 10, 2, n.str ());
    }

  // Create the UEs and set position
  NodeContainer ueNodes;
  ueNodes.Add (sender);
  for (uint32_t i = 0; i < 1; ++i)
    {
      std::stringstream n;
      n << "UE" << i+1;
      SetNodeProperties (ueNodes.Get (i), 9, 10, 2, n.str ());
    }

  NetDeviceContainer enbDevices = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevices = lteHelper->InstallUeDevice (ueNodes);

  NetDeviceContainer::Iterator it;
  for (it = enbDevices.Begin (); it != enbDevices.End (); ++it)
    {
      Ptr<LteEnbNetDevice> dev = DynamicCast<LteEnbNetDevice> (*it);
      NS_ASSERT (dev != 0);

      dev->GetPhy ()->SetTxPower (23.0);
      dev->GetRrc ()->SetSrsPeriodicity (80);
    }

  InternetStackHelper stack;
  stack.Install (ueNodes);

  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (ueDevices);

  for (it = ueDevices.Begin (); it != ueDevices.End (); ++it)
    {
      Ptr<LteUeNetDevice> dev = DynamicCast<LteUeNetDevice> (*it);

      dev->GetPhy ()->SetTxPower (23.0);
    }

  lteHelper->AttachToClosestEnb (ueDevices, enbDevices);


  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      Ptr<Node> ue = ueNodes.Get (i);
      Ptr<Ipv4StaticRouting> ueStaticRouting;
      ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());

      // 0 is Loopback, 1 is LteUeNetDevice
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  BuildIpv4LAN (ueDevices, "192.168.0.0");

  {
    Ptr<Ipv4StaticRouting> ueStaticRouting;
    ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

    // 0 is Loopback, 1 is LteUeNetDevice
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  return internetIpIfaces.GetAddress (1);
}


//-----------------------------------------------------------------------------

static class TcpBufferSizeTestSuite : public TestSuite
{
public:
  TcpBufferSizeTestSuite () : TestSuite ("tcp-buffer-size-test", UNIT)
  {
    AddTestCase (new TcpBufferSizeTest ("Some data exchanged"),
                 TestCase::QUICK);
  }
} g_tcpBufferSizeTestSuite;

} // namespace ns3
