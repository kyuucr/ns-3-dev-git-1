/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jose Nuñez <jose.nunez@cttc.es>
 *
 */

#include <ns3/mesh-epc-helper.h>
#include <ns3/log.h>
#include <ns3/inet-socket-address.h>
#include <ns3/mac48-address.h>
#include <ns3/eps-bearer.h>
#include <ns3/ipv4-address.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/packet-socket-helper.h>
#include <ns3/packet-socket-address.h>
#include <ns3/epc-enb-application.h>
#include <ns3/epc-sgw-pgw-application.h>

#include <ns3/point-to-point-net-device.h>
#include <ns3/error-model.h>
#include <ns3/random-variable-stream.h>
#include <ns3/core-module.h>

#include <ns3/lte-enb-rrc.h>
#include <ns3/epc-x2.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/epc-mme.h>
#include <ns3/epc-ue-nas.h>
#include <ns3/drop-tail-queue.h>

#include <algorithm>    // std::count
//Backpressure routing protocol
#include "ns3/backpressure.h"
#include "ns3/mobility-module.h"
#include "ns3/node-list.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ppp-header.h"
#include "ns3/epc-gtpu-header.h"
#include "ns3/udp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MeshEpcHelper");

NS_OBJECT_ENSURE_REGISTERED (MeshEpcHelper);

static void
Fail (const std::string &name, Ptr<const Packet> p)
{
  NS_FATAL_ERROR (name << " SPOTTED A DROP!!");
}

/*
static std::string
PrintPacket (Ptr<Packet> p)
{
  Ipv4Header ipv4;
  TcpHeader tcp;
  PppHeader ppp;
  UdpHeader udp;
  GtpuHeader gtpu;
  uint32_t size;

  size = p->RemoveHeader (ppp);
  NS_ASSERT (size > 0);

  size = p->RemoveHeader (ipv4);
  NS_ASSERT (size > 0);


  std::stringstream ret;


  if (ipv4.GetProtocol () == UDP_PROT_NUMBER)
    {
      size = p->RemoveHeader (udp);

      if (udp.GetSourcePort () == 698)
        {
          return "";
        }

      size = p->RemoveHeader (gtpu);
      if (size > 0)
        {
          size = p->RemoveHeader (ipv4);
          NS_ASSERT (size > 0);
          size = p->RemoveHeader (tcp);
          NS_ASSERT (size > 0);
          ret << " From: " << ipv4.GetSource () << " to: " << ipv4.GetDestination () <<
            " seq: " << tcp.GetSequenceNumber () << " ack: " << tcp.GetAckNumber () <<
            " size: " << p->GetSize () << " flags: " << TcpHeader::FlagsToString (tcp.GetFlags ());
        }
    }
  else if (ipv4.GetProtocol () == TCP_PROT_NUMBER)
    {
      size = p->RemoveHeader (tcp);
      NS_ASSERT (size > 0);
      ret << " From: " << ipv4.GetSource () << " to: " << ipv4.GetDestination () <<
        " seq: " << tcp.GetSequenceNumber () << " ack: " << tcp.GetAckNumber () <<
        " size: " << p->GetSize () << " flags: " << TcpHeader::FlagsToString (tcp.GetFlags ());
    }

  return ret.str ();
}

static void
TraceTx (const std::string &name, Ptr<const Packet> p)
{
  std::string s = PrintPacket (p->Copy ());

  if (!s.empty ())
    NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " [TX] " <<
                  name << s);
}

static void
TraceRx (const std::string &name, Ptr<const Packet> p)
{
  std::string s = PrintPacket (p->Copy ());

  if (!s.empty ())
    NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " [RX] " <<
                  name << s);
}
*/
void
MeshEpcHelper::TraceAndDebug (Ptr<NetDevice> first, Ptr<NetDevice> second)
{
  //first->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TraceTx, Names::FindName (first->GetNode ())));
  //first->TraceConnectWithoutContext ("MacRx", MakeBoundCallback (&TraceRx, Names::FindName (first->GetNode ())));
  first->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&Fail, Names::FindName (first->GetNode ())));

  //second->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TraceTx, Names::FindName (second->GetNode ())));
  //second->TraceConnectWithoutContext ("MacRx", MakeBoundCallback (&TraceRx, Names::FindName (second->GetNode ())));
  second->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&Fail, Names::FindName (second->GetNode ())));
}

MeshEpcHelper::MeshEpcHelper ()
  : m_gtpuUdpPort (2152)    // fixed by the standard
{
  NS_LOG_FUNCTION (this);

  // since we use point-to-point links for all S1-U links,
  // we use a /30 subnet which can hold exactly two addresses
  // (remember that net broadcast and null address are not valid)
  m_s1uIpv4AddressHelper.SetBase ("10.0.0.0", "255.255.255.252");

  m_x2Ipv4AddressHelper.SetBase ("12.0.0.0", "255.255.255.252");

  // we use a /8 net for all UEs
  m_ueAddressHelper.SetBase ("7.0.0.0", "255.0.0.0");

  // create SgwPgwNode
  m_sgwPgw = CreateObject<Node> ();
  InternetStackHelper internet;
  internet.Install (m_sgwPgw);

  // create S1-U socket
  Ptr<Socket> sgwPgwS1uSocket = Socket::CreateSocket (m_sgwPgw, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = sgwPgwS1uSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_gtpuUdpPort));
  NS_ASSERT (retval == 0);

  // create TUN device implementing tunneling of user data over GTP-U/UDP/IP
  m_tunDevice = CreateObject<VirtualNetDevice> ();
  // allow jumbo packets
  m_tunDevice->SetAttribute ("Mtu", UintegerValue (30000));

  // yes we need this
  m_tunDevice->SetAddress (Mac48Address::Allocate ());

  m_sgwPgw->AddDevice (m_tunDevice);
  NetDeviceContainer tunDeviceContainer;
  tunDeviceContainer.Add (m_tunDevice);

  // the TUN device is on the same subnet as the UEs, so when a packet
  // addressed to an UE arrives at the intenet to the WAN interface of
  // the PGW it will be forwarded to the TUN device.
  Ipv4InterfaceContainer tunDeviceIpv4IfContainer = m_ueAddressHelper.Assign (tunDeviceContainer);

  // create EpcSgwPgwApplication
  m_sgwPgwApp = CreateObject<EpcSgwPgwApplication> (m_tunDevice, sgwPgwS1uSocket);
  m_sgwPgw->AddApplication (m_sgwPgwApp);

  // connect SgwPgwApplication and virtual net device for tunneling
  m_tunDevice->SetSendCallback (MakeCallback (&EpcSgwPgwApplication::RecvFromTunDevice, m_sgwPgwApp));

  // Create MME and connect with SGW via S11 interface
  m_mme = CreateObject<EpcMme> ();
  m_mme->SetS11SapSgw (m_sgwPgwApp->GetS11SapSgw ());
  m_sgwPgwApp->SetS11SapMme (m_mme->GetS11SapMme ());
}

MeshEpcHelper::~MeshEpcHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MeshEpcHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MeshEpcHelper")
    .SetParent<EpcHelper> ()
    .SetGroupName ("Lte")
    .AddConstructor<MeshEpcHelper> ()
    .AddAttribute ("S1uLinkDataRate",
                   "The data rate to be used for the next S1-U link to be created",
                   DataRateValue (DataRate ("500Mb/s")),
                   MakeDataRateAccessor (&MeshEpcHelper::m_s1uLinkDataRateTerrestrial),
                   MakeDataRateChecker ())
    .AddAttribute ("S1uLinkDataRateSatellite",
                   "The data rate to be used for the next S1-U satellite link to be created",
                   DataRateValue (DataRate ("300Mb/s")),
                   MakeDataRateAccessor (&MeshEpcHelper::m_s1uLinkDataRateSatellite),
                   MakeDataRateChecker ())
    .AddAttribute ("S1uLinkDelay",
                   "The delay to be used for the next S1-U link to be created",
                   TimeValue (MilliSeconds (4)),
                   MakeTimeAccessor (&MeshEpcHelper::m_s1uLinkDelayTerrestrial),
                   MakeTimeChecker ())
    .AddAttribute ("S1uLinkDelaySatellite",
                   "The delay to be used for the next S1-U link to be created",
                   TimeValue (Seconds (0.25)),
                   MakeTimeAccessor (&MeshEpcHelper::m_s1uLinkDelaySatellite),
                   MakeTimeChecker ())
    .AddAttribute ("S1uLinkMtu",
                   "The MTU of the next S1-U link to be created. Note that, because of the additional GTP/UDP/IP tunneling overhead, you need a MTU larger than the end-to-end MTU that you want to support.",
                   UintegerValue (30000),
                   MakeUintegerAccessor (&MeshEpcHelper::m_s1uLinkMtuTerrestrial),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("S1uLinkMtuSatellite",
                   "The MTU of the next S1-U link to be created. Note that, because of the additional GTP/UDP/IP tunneling overhead, you need a MTU larger than the end-to-end MTU that you want to support.",
                   UintegerValue (30000),
                   MakeUintegerAccessor (&MeshEpcHelper::m_s1uLinkMtuSatellite),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("X2LinkDataRate",
                   "The data rate to be used for the next X2 link to be created",
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&MeshEpcHelper::m_x2LinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("X2LinkDelay",
                   "The delay to be used for the next X2 link to be created",
                   TimeValue (MilliSeconds (5)),
                   MakeTimeAccessor (&MeshEpcHelper::m_x2LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("X2LinkMtu",
                   "The MTU of the next X2 link to be created. Note that, because of some big X2 messages, you need a big MTU.",
                   UintegerValue (30000),
                   MakeUintegerAccessor (&MeshEpcHelper::m_x2LinkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
MeshEpcHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_tunDevice->SetSendCallback (MakeNullCallback<bool, Ptr<Packet>, const Address&, const Address&, uint16_t> ());
  m_tunDevice = 0;
  m_sgwPgwApp = 0;
  m_sgwPgw->Dispose ();
}

void
MeshEpcHelper::AddHybridMeshBackhaul (NodeContainer enbs, std::vector<std::vector<int> > terrestrialConMatrix,
                                      std::vector<bool> terrestrialEpc, std::vector<bool> terrestrialSat,
                                      Ipv4ListRoutingHelper routingList, uint32_t meshQ)
{
  NS_LOG_FUNCTION (this);

  // set terrestrial point to point link characteristics
  PointToPointHelper p2phterr;
  p2phterr.SetDeviceAttribute ("DataRate", DataRateValue (m_s1uLinkDataRateTerrestrial));
  p2phterr.SetDeviceAttribute ("Mtu", UintegerValue (m_s1uLinkMtuTerrestrial));
  p2phterr.SetChannelAttribute ("Delay", TimeValue (m_s1uLinkDelayTerrestrial));

  // set satellite point to point link characteristics
  PointToPointHelper p2phsat;
  p2phsat.SetDeviceAttribute ("DataRate", DataRateValue (m_s1uLinkDataRateSatellite));
  p2phsat.SetDeviceAttribute ("Mtu", UintegerValue (m_s1uLinkMtuSatellite));
  p2phsat.SetChannelAttribute ("Delay", TimeValue (m_s1uLinkDelaySatellite));
  p2phsat.SetQueue("ns3::DropTailQueue",
                   "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES),
                   "MaxBytes", UintegerValue ((m_s1uLinkDataRateSatellite.GetBitRate() / 8)*
                                              2*m_s1uLinkDelaySatellite.GetSeconds ()));

  // Set EPC - node point to ponit link characteristics
  PointToPointHelper p2phepc;
  p2phepc.SetDeviceAttribute ("DataRate", DataRateValue (DataRate("1Gbps")));
  p2phepc.SetDeviceAttribute ("Mtu", UintegerValue (m_s1uLinkMtuTerrestrial));
  p2phepc.SetChannelAttribute ("Delay", TimeValue (Time("0.5ms")));
  p2phepc.SetQueue("ns3::DropTailQueue",
                   "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES),
                   "MaxBytes", UintegerValue ((DataRate("1Gbps").GetBitRate() / 8)*
                                              2*Time("0.5ms").GetSeconds ()));

  int32_t numberEnodeBs = enbs.GetN ();
  int SatGw = std::count (terrestrialSat.begin (), terrestrialSat.end (), 1);

  if (SatGw !=0)
    {   //if there is satellite gw, install the internet stack helper
      numberEnodeBs = numberEnodeBs-1;
      Ptr<Node> enb = enbs.Get (numberEnodeBs);
      InternetStackHelper internet;
      // add Ipv4 stack to the extra enb
      internet.SetRoutingHelper (routingList);
      internet.Install (enb);
    }

  Ptr<Ipv4> epc_ipv4 = m_sgwPgw->GetObject<Ipv4> ();

  for (uint16_t i=0; i<numberEnodeBs; i++)
    {
      //we exclude the "extraEnodeB" because we have previously installed the InternetStackHelper on it
      Ptr<Node> enb = enbs.Get (i);
      InternetStackHelper internet;
      // add Ipv4 stack to enbs
      internet.SetRoutingHelper (routingList);
      internet.Install (enb);
      Ptr<Ipv4> enb_ipv4 = enb->GetObject<Ipv4> ();

      if (terrestrialEpc[enb->GetId ()-2] == 1)   //first enb has id equal to 2
        {
          // create a point to point link between the new eNB and the SGW with
          // the corresponding new NetDevices on each side
          NetDeviceContainer enbSgwDevices = p2phepc.Install (enb, m_sgwPgw);

          Ptr<NetDevice> enbDev = enbSgwDevices.Get (0);
          Ptr<NetDevice> sgwDev = enbSgwDevices.Get (1);

          m_s1uIpv4AddressHelper.NewNetwork ();
          m_s1uIpv4AddressHelper.Assign (enbSgwDevices);

          NS_LOG_INFO (Names::FindName (enb) << " id " << enb->GetId () <<
                       " connected to EPC, interface " << enb_ipv4->GetNInterfaces ()-1);
          NS_LOG_INFO (Names::FindName (m_sgwPgw) << " id " << m_sgwPgw->GetId () <<
                       " connected to " << Names::FindName (enb) <<
                       ", interface " << epc_ipv4->GetNInterfaces ()-1);

          //in case of backpressure routing, we record this info
          Ptr<PointToPointNetDevice> pTop = enbDev->GetObject<PointToPointNetDevice>();
          Ptr<backpressure::RoutingProtocol> bpN;
          int interface= 0;
          if (pTop!=NULL)
            {
              interface= pTop->GetIfIndex ();
            }
          bpN = enb->GetObject<backpressure::RoutingProtocol> ();
          if (bpN !=NULL)
            {
              bpN->SetInfoInterfaces (interface, terrestrialEpc[enb->GetId ()-2]);
              TraceAndDebug (enbDev, sgwDev);
            }
          pTop = sgwDev->GetObject<PointToPointNetDevice>();
          interface = pTop->GetIfIndex ();
          bpN = m_sgwPgw->GetObject<backpressure::RoutingProtocol> ();
          if (bpN !=NULL)
            {
              bpN->SetInfoInterfaces (interface, terrestrialEpc[enb->GetId ()-2]);
            }
        }

      //connecting the terrestrial nodes with satellite capabilities to the satellite gateway
      if (terrestrialSat[enb->GetId ()-2] == 1)   //first enb has id equal to 2
        {
          Ptr<Ipv4> sat_ipv4 = enbs.Get (numberEnodeBs)->GetObject<Ipv4>();
          NetDeviceContainer enbSatGwDevices = p2phsat.Install (enb, enbs.Get (numberEnodeBs));

          Ptr<NetDevice> enbDev = enbSatGwDevices.Get (0);
          Ptr<NetDevice> satgwDev = enbSatGwDevices.Get (1);
          m_s1uIpv4AddressHelper.NewNetwork ();
          m_s1uIpv4AddressHelper.Assign (enbSatGwDevices);


          NS_LOG_INFO (Names::FindName (enb) << " id " << enb->GetId () <<
                       " connected to SAT, interface " << enb_ipv4->GetNInterfaces ()-1);
          NS_LOG_INFO (Names::FindName (enbs.Get (numberEnodeBs)) << " id " <<
                       enbs.Get (numberEnodeBs)->GetId () << " connected to " << Names::FindName (enb) <<
                       ", interface " << sat_ipv4->GetNInterfaces ()-1);

          Ptr<PointToPointNetDevice> pTop = enbDev->GetObject<PointToPointNetDevice>();
          Ptr<backpressure::RoutingProtocol> bpN;
          int interface = 0;
          if (pTop!=NULL)
            {
              interface= pTop->GetIfIndex ();
            }
          bpN = enb->GetObject<backpressure::RoutingProtocol> ();
          if (bpN !=NULL)
            {
              bpN->SetInfoInterfaces (interface, terrestrialSat[enb->GetId ()-2]);
              bpN->SetSatInterface(interface);
            }
          pTop = satgwDev->GetObject<PointToPointNetDevice>();
          interface = pTop->GetIfIndex ();
          bpN = enbs.Get (numberEnodeBs)->GetObject<backpressure::RoutingProtocol> ();
          if (bpN !=NULL)
            {
              bpN->SetInfoInterfaces (interface, terrestrialSat[enb->GetId ()-2]);
              bpN->SetSatInterface(interface);
            }
        }
    }

  p2phterr.SetChannelAttribute ("Delay", TimeValue (m_s1uLinkDelayTerrestrial));   //10km
  //connecting the satellite gateway to the EPC ( the satellite gw is also a enobeB)
  if (SatGw!=0)
    {   //this is the satellite gw connection to the EPC
      NodeContainer enbSgwNodes;
      enbSgwNodes.Add (m_sgwPgw);
      Ptr<Node> enb = enbs.Get (numberEnodeBs);
      Ptr<Ipv4> enb_ipv4 = enb->GetObject<Ipv4> ();
      enbSgwNodes.Add (enb);
      DataRate rate (1000000*1000);  //1Gbps connection with nodes connecting to the epc
      p2phterr.SetDeviceAttribute ("DataRate", DataRateValue (rate));
      NetDeviceContainer enbSgwDevices = p2phterr.Install (enb, m_sgwPgw);

      Ptr<NetDevice> enbDev = enbSgwDevices.Get (0);
      Ptr<NetDevice> sgwDev = enbSgwDevices.Get (1);
      m_s1uIpv4AddressHelper.NewNetwork ();
      Ipv4InterfaceContainer enbSgwIpIfaces = m_s1uIpv4AddressHelper.Assign (enbSgwDevices);

      NS_LOG_INFO (Names::FindName (enb) << " id " << enb->GetId () <<
                   " connected to EPC, interface " << enb_ipv4->GetNInterfaces ()-1);
      NS_LOG_INFO (Names::FindName (m_sgwPgw) << " connected to " << Names::FindName (enb) <<
                   ", interface " << epc_ipv4->GetNInterfaces ()-1);

      Ptr<PointToPointNetDevice> pTop = enbDev->GetObject<PointToPointNetDevice>();
      Ptr<backpressure::RoutingProtocol> bpN;
      int interface = 0;
      if (pTop!=NULL)
        {
          interface= pTop->GetIfIndex ();
        }
      bpN = enb->GetObject<backpressure::RoutingProtocol> ();
      if (bpN !=NULL)
        {
          bpN->SetInfoInterfaces (interface, 1);
          bpN->SetSatInterface(interface);
        }
      pTop = sgwDev->GetObject<PointToPointNetDevice>();
      interface = pTop->GetIfIndex ();
      bpN = m_sgwPgw->GetObject<backpressure::RoutingProtocol> ();
      if (bpN !=NULL)
        {
          bpN->SetInfoInterfaces (interface, 1);
          bpN->SetSatInterface(interface);
        }
    }

  NS_LOG_INFO ("Create Links Between Terrestrial Mesh Nodes.");

  uint32_t linkCount = 0;
  p2phterr.SetDeviceAttribute ("DataRate", DataRateValue (m_s1uLinkDataRateTerrestrial));

  if (meshQ == 0)
    { // No meshQ, stick to BDP
      p2phterr.SetQueue("ns3::DropTailQueue",
                        "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES),
                        "MaxBytes", UintegerValue ((m_s1uLinkDataRateTerrestrial.GetBitRate() / 8)*
                                                  2*m_s1uLinkDelayTerrestrial.GetSeconds()));
    }
  else
    { // Manually passed meshQ (in packets)
      p2phterr.SetQueue("ns3::DropTailQueue",
                        "Mode", EnumValue (DropTailQueue::QUEUE_MODE_PACKETS),
                        "MaxPackets", UintegerValue (meshQ));
    }

  for (size_t i = 0; i < terrestrialConMatrix.size (); i++)
    {
      for (size_t j = 0; j < terrestrialConMatrix[i].size (); j++)
        {
          if (terrestrialConMatrix[i][j] != 0)
            {
              NodeContainer n_links = NodeContainer (enbs.Get (i), enbs.Get (j));

              NetDeviceContainer n_devs = p2phterr.Install (n_links);

              m_s1uIpv4AddressHelper.NewNetwork ();
              m_s1uIpv4AddressHelper.Assign (n_devs);

              NS_LOG_INFO (Names::FindName (enbs.Get (i)) << " id " << enbs.Get (i)->GetId ()
                                                          << " connected to " <<
                           Names::FindName (enbs.Get (j)) << ", interface " <<
                           enbs.Get (i)->GetObject<Ipv4>()->GetNInterfaces ()-1);
              NS_LOG_INFO (Names::FindName (enbs.Get (j)) << " id " << enbs.Get (j)->GetId ()
                                                          << " connected to " <<
                           Names::FindName (enbs.Get (i)) << ", interface " <<
                           enbs.Get (j)->GetObject<Ipv4>()->GetNInterfaces ()-1);

              linkCount++;
              uint32_t interface;
              Ptr<PointToPointNetDevice> pTop = n_devs.Get (0)->GetObject<PointToPointNetDevice>();
              Ptr<backpressure::RoutingProtocol> bpN;
              if (pTop!=NULL)
                {
                  interface= pTop->GetIfIndex ();
                  if (terrestrialConMatrix[i][j] == 2)
                    {
                      enbs.Get (i)->GetObject<Ipv4>()->SetDown (interface);
                    }
                  bpN = enbs.Get (i)->GetObject<backpressure::RoutingProtocol> ();
                  if (bpN !=NULL)
                    {
                      bpN->SetInfoInterfaces (interface, terrestrialConMatrix[i][j]);

                      TraceAndDebug (n_devs.Get (0), n_devs.Get (1));
                    }
                }
              pTop = n_devs.Get (1)->GetObject<PointToPointNetDevice>();
              if (pTop!=NULL)
                {
                  interface= pTop->GetIfIndex ();
                  if (terrestrialConMatrix[i][j] == 2)
                    {
                      enbs.Get (j)->GetObject<Ipv4>()->SetDown (interface);
                    }
                  bpN = enbs.Get (j)->GetObject<backpressure::RoutingProtocol> ();
                  if (bpN !=NULL)
                    {
                      bpN->SetInfoInterfaces (interface, terrestrialConMatrix[i][j]);
                    }
                }
            }
          else
            {
              // No connection
            }
        }
    }
}

void
MeshEpcHelper::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // add an IPv4 stack to the previously created eNB
  //InternetStackHelper internet;
  //internet.Install (enb);
  //NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB after node creation: " << enb->GetObject<Ipv4> ()->GetNInterfaces ());

  // create a point to point link between the new eNB and the SGW with
  // the corresponding new NetDevices on each side
  /*NodeContainer enbSgwNodes;
enbSgwNodes.Add (m_sgwPgw);
enbSgwNodes.Add (enb);
PointToPointHelper p2ph;
p2ph.SetDeviceAttribute ("DataRate", DataRateValue (m_s1uLinkDataRateTerrestrial));
p2ph.SetDeviceAttribute ("Mtu", UintegerValue (m_s1uLinkMtuTerrestrial));
p2ph.SetChannelAttribute ("Delay", TimeValue (m_s1uLinkDelayTerrestrial));
NetDeviceContainer enbSgwDevices = p2ph.Install (enb, m_sgwPgw);
NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB after installing p2p dev: " << enb->GetObject<Ipv4> ()->GetNInterfaces ());
Ptr<NetDevice> enbDev = enbSgwDevices.Get (0);
Ptr<NetDevice> sgwDev = enbSgwDevices.Get (1);
m_s1uIpv4AddressHelper.NewNetwork ();
Ipv4InterfaceContainer enbSgwIpIfaces = m_s1uIpv4AddressHelper.Assign (enbSgwDevices);
NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB after assigning Ipv4 addr to S1 dev: " << enb->GetObject<Ipv4> ()->GetNInterfaces ());*/

  //Ipv4Address enbAddress = enbSgwIpIfaces.GetAddress (0);
  //Ipv4Address sgwAddress = enbSgwIpIfaces.GetAddress (1);
  Ipv4Address enbAddress = enb->GetObject<Ipv4>()->GetAddress (1,0).GetLocal ();      //take first backhaul interface address
  NS_LOG_INFO (" enbAddress " << enbAddress << " enb "<< enb->GetId () );
  Ipv4Address sgwAddress;
  //sgwAddress = m_sgwPgw->GetObject<Ipv4>()->GetAddress (2+cellId,0).GetLocal(); //take second SGW address
  sgwAddress = m_sgwPgw->GetObject<Ipv4>()->GetAddress (3,0).GetLocal ();  //take second SGW address
  NS_LOG_INFO (" sgwAddress " << sgwAddress);

  // create S1-U socket for the ENB
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (enb, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = enbS1uSocket->Bind (InetSocketAddress (enbAddress, m_gtpuUdpPort));
  NS_ASSERT (retval == 0);


  // give PacketSocket powers to the eNB
  //PacketSocketHelper packetSocket;
  //packetSocket.Install (enb);

  // create LTE socket for the ENB
  Ptr<Socket> enbLteSocket = Socket::CreateSocket (enb, TypeId::LookupByName ("ns3::PacketSocketFactory"));
  PacketSocketAddress enbLteSocketBindAddress;
  enbLteSocketBindAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  retval = enbLteSocket->Bind (enbLteSocketBindAddress);
  NS_ASSERT (retval == 0);
  PacketSocketAddress enbLteSocketConnectAddress;
  enbLteSocketConnectAddress.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  retval = enbLteSocket->Connect (enbLteSocketConnectAddress);
  NS_ASSERT (retval == 0);


  NS_LOG_INFO ("create EpcEnbApplication");
  Ptr<EpcEnbApplication> enbApp = CreateObject<EpcEnbApplication> (enbLteSocket, enbS1uSocket, enbAddress, sgwAddress, cellId);
  enb->AddApplication (enbApp);
  NS_ASSERT (enb->GetNApplications () == 1);
  NS_ASSERT_MSG (enb->GetApplication (0)->GetObject<EpcEnbApplication> () != 0, "cannot retrieve EpcEnbApplication");
  NS_LOG_LOGIC ("enb: " << enb << ", enb->GetApplication (0): " << enb->GetApplication (0));


  NS_LOG_INFO ("Create EpcX2 entity");
  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

  NS_LOG_INFO ("connect S1-AP interface");
  m_mme->AddEnb (cellId, enbAddress, enbApp->GetS1apSapEnb ());
  m_sgwPgwApp->AddEnb (cellId, enbAddress, sgwAddress);
  enbApp->SetS1apSapMme (m_mme->GetS1apSapMme ());
}


void
MeshEpcHelper::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // Create a point to point link between the two eNBs with
  // the corresponding new NetDevices on each side
  NodeContainer enbNodes;
  enbNodes.Add (enb1);
  enbNodes.Add (enb2);
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (m_x2LinkDataRate));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (m_x2LinkMtu));
  p2ph.SetChannelAttribute ("Delay", TimeValue (m_x2LinkDelay));
  NetDeviceContainer enbDevices = p2ph.Install (enb1, enb2);
  NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB #1 after installing p2p dev: " << enb1->GetObject<Ipv4> ()->GetNInterfaces ());
  NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB #2 after installing p2p dev: " << enb2->GetObject<Ipv4> ()->GetNInterfaces ());
  Ptr<NetDevice> enb1Dev = enbDevices.Get (0);
  Ptr<NetDevice> enb2Dev = enbDevices.Get (1);

  m_x2Ipv4AddressHelper.NewNetwork ();
  Ipv4InterfaceContainer enbIpIfaces = m_x2Ipv4AddressHelper.Assign (enbDevices);
  NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB #1 after assigning Ipv4 addr to X2 dev: " << enb1->GetObject<Ipv4> ()->GetNInterfaces ());
  NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB #2 after assigning Ipv4 addr to X2 dev: " << enb2->GetObject<Ipv4> ()->GetNInterfaces ());

  Ipv4Address enb1X2Address = enbIpIfaces.GetAddress (0);
  Ipv4Address enb2X2Address = enbIpIfaces.GetAddress (1);

  // Add X2 interface to both eNBs' X2 entities
  Ptr<EpcX2> enb1X2 = enb1->GetObject<EpcX2> ();
  Ptr<LteEnbNetDevice> enb1LteDev = enb1->GetDevice (0)->GetObject<LteEnbNetDevice> ();
  uint16_t enb1CellId = enb1LteDev->GetCellId ();
  NS_LOG_LOGIC ("LteEnbNetDevice #1 = " << enb1LteDev << " - CellId = " << enb1CellId);

  Ptr<EpcX2> enb2X2 = enb2->GetObject<EpcX2> ();
  Ptr<LteEnbNetDevice> enb2LteDev = enb2->GetDevice (0)->GetObject<LteEnbNetDevice> ();
  uint16_t enb2CellId = enb2LteDev->GetCellId ();
  NS_LOG_LOGIC ("LteEnbNetDevice #2 = " << enb2LteDev << " - CellId = " << enb2CellId);

  enb1X2->AddX2Interface (enb1CellId, enb1X2Address, enb2CellId, enb2X2Address);
  enb2X2->AddX2Interface (enb2CellId, enb2X2Address, enb1CellId, enb1X2Address);

  enb1LteDev->GetRrc ()->AddX2Neighbour (enb2LteDev->GetCellId ());
  enb2LteDev->GetRrc ()->AddX2Neighbour (enb1LteDev->GetCellId ());
}


void
MeshEpcHelper::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice );

  m_mme->AddUe (imsi);
  m_sgwPgwApp->AddUe (imsi);


}

uint8_t
MeshEpcHelper::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  // we now retrieve the IPv4 address of the UE and notify it to the SGW;
  // we couldn't do it before since address assignment is triggered by
  // the user simulation program, rather than done by the EPC
  Ptr<Node> ueNode = ueDevice->GetNode ();
  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed before EPS bearers can be activated");
  int32_t interface =  ueIpv4->GetInterfaceForDevice (ueDevice);
  NS_ASSERT (interface >= 0);
  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);
  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
  NS_LOG_LOGIC (" UE IP address: " << ueAddr);  m_sgwPgwApp->SetUeAddress (imsi, ueAddr);

  uint8_t bearerId = m_mme->AddBearer (imsi, tft, bearer);
  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  if (ueLteDevice)
    {
      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
    }
  return bearerId;
}


Ptr<Node>
MeshEpcHelper::GetPgwNode ()
{
  return m_sgwPgw;
}


Ipv4InterfaceContainer
MeshEpcHelper::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  return m_ueAddressHelper.Assign (ueDevices);
}



Ipv4Address
MeshEpcHelper::GetUeDefaultGatewayAddress ()
{
  // return the address of the tun device
  return m_sgwPgw->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
}


} // namespace ns3
