/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

//
// This is an illustration of how one could use virtualization techniques to
// allow running applications on virtual machines talking over simulated
// networks.
//
// The actual steps required to configure the virtual machines can be rather
// involved, so we don't go into that here.  Please have a look at one of
// our HOWTOs on the nsnam wiki for more details about how to get the
// system confgured.  For an example, have a look at "HOWTO Use Linux
// Containers to set up virtual networks" which uses this code as an
// example.
//
// The configuration you are after is explained in great detail in the
// HOWTO, but looks like the following:
//
//  +----------+
//  | virtual  |
//  |  Linux   |
//  |   UML    |
//  |          |
//  |   eth0   |
//  +----------+
//       |
//  +----------+
//  |  Linux   |
//  |  Bridge  |                   n1
//  +----------+               +--------+
//       |                     |  ns-3  |
//  +------------+             |  Sink  |
//  | "tap-left" |             +--------+
//  +------------+             |  ns-3  |
//       |           n0        |   TCP  |
//       |       +--------+    +--------+
//       +-------|  tap   |    |  ns-3  |
//               | bridge |    |   IP   |
//               +--------+    +--------+
//               | P2PND  |    | P2PND  |
//               +--------+    +--------+
//                   |             |
//                   |             |
//                   |             |
//                   ===============
//                   P2P Virtual Link
//
#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapP2PVirtualReceiver");

void
PrintRoutingTable (Ptr<Node> node)
{
  Ipv4StaticRoutingHelper helper;
  Ptr<Ipv4> stack = node->GetObject<Ipv4>();
  Ptr<Ipv4StaticRouting> staticrouting = helper.GetStaticRouting (stack);
  uint32_t numroutes=staticrouting->GetNRoutes ();
  Ipv4RoutingTableEntry entry;
  std::cout << "Routing table for device: " << Names::FindName (node) << "\n";
  std::cout << "Destination\tMask\t\tGateway\t\tIface\n";

  for (uint32_t i =0; i<numroutes; i++)
    {
      entry =staticrouting->GetRoute (i);
      std::cout << entry.GetDestNetwork ()  << "\t"
                << entry.GetDestNetworkMask () << "\t"
                << entry.GetGateway () << "\t\t"
                << entry.GetInterface () << "\n";
    }

  return;
}

int
main (int argc, char *argv[])
{
  double error_p = 0.0;
  std::string bandwidth = "2Mbps";
  std::string delay = "0.01ms";
  bool pcap = false;
  bool sack = true;
  uint16_t port = 5001;

  CommandLine cmd;
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("bandwidth", "Bottleneck bandwidth", bandwidth);
  cmd.AddValue ("delay", "Bottleneck delay", delay);
  cmd.AddValue ("pcap", "Enable or disable PCAP tracing", pcap);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("port", "Listening port", port);
  cmd.Parse (argc, argv);

  //
  // Setting the receiver buffer to 4MB, and other options for the receiver
  //
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

  //
  // We are interacting with the outside, real, world.  This means we have to
  // interact in real-time and therefore means we have to use the real-time
  // simulator and take the time to calculate checksums.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // Create two ghost nodes.  The first will represent the virtual machine host
  // on the left side of the network; and the second will represent the ns3
  // node on the right.
  //
  NodeContainer nodes;
  nodes.Create (2);

  //
  // Set up the error model for the virtual link
  //
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  //
  // Use a PointToPointHelper to get a virtual P2P channel created, and the needed net
  // devices installed on both of the nodes.
  //
  CsmaHelper virtualLink;
  virtualLink.SetChannelAttribute ("DataRate", StringValue (bandwidth));
  virtualLink.SetChannelAttribute ("Delay", StringValue (delay));
  virtualLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer devices = virtualLink.Install (nodes);

  //
  // Install the internet stack on our node.
  //
  InternetStackHelper stack;
  stack.Install (nodes.Get (1));

  Ipv4AddressHelper address;
  address.SetBase ("10.0.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices.Get (1));

  //
  // Use the TapBridgeHelper to connect to the pre-configured tap devices for
  // the left side.  We go with "UseBridge" mode since the CSMA devices support
  // promiscuous mode and can therefore make it appear that the bridge is
  // extended into ns-3.  The install method essentially bridges the specified
  // tap to the specified CSMA device.
  //
  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("UseLocal"));
  //tapBridge.SetAttribute ("IpAddress", Ipv4AddressValue ("10.0.1.2"));
  //tapBridge.SetAttribute ("Netmask", Ipv4MaskValue ("255.255.255.0"));
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-right"));
  tapBridge.Install (nodes.Get (0), devices.Get (0));

  //
  // Populate the routing tables.
  //
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //
  // Add a default route
  //
  Ipv4StaticRoutingHelper helper;
  Ptr<Ipv4> ipv4 = nodes.Get (1)->GetObject<Ipv4>();
  Ptr<Ipv4StaticRouting> staticRouting = helper.GetStaticRouting (ipv4);
  staticRouting->AddHostRouteTo("10.0.0.2", 1);
  staticRouting->SetDefaultRoute (Ipv4Address ("10.0.0.2"), 1);

  PrintRoutingTable (nodes.Get (1));

  //
  // Set up the TCP sink in the ns-3 node
  //
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (1));

  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (600.0));

  if (pcap)
    {
      virtualLink.EnablePcapAll ("P2PTapVirtualReceiver", true);
    }

  //
  // Run the simulation for ten minutes to give the user time to play around
  //
  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}
