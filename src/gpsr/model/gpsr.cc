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
 */
#include "gpsr.h"
#include "gpsr-packet.h"

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/location-service.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GpsrIpv4RoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED (GpsrIpv4RoutingProtocol);

/// UDP Port for GPSR control traffic, not defined by IANA yet
const uint32_t GpsrIpv4RoutingProtocol::GPSR_PORT = 666;

TypeId
GpsrIpv4RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GpsrIpv4RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<GpsrIpv4RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&GpsrIpv4RoutingProtocol::m_helloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("LocationServiceName", "Type of LocationService to use",
                   EnumValue (GPSR_GOD),
                   MakeEnumAccessor (&GpsrIpv4RoutingProtocol::m_locationTableType),
                   MakeEnumChecker (GPSR_GOD, "GPSR_GOD"))
  ;
  return tid;
}

GpsrIpv4RoutingProtocol::GpsrIpv4RoutingProtocol ()
  : Ipv4RoutingProtocol (),
    m_helloInterval (MilliSeconds (1000)),
    m_locationTableType (GPSR_GOD)
{
}

GpsrIpv4RoutingProtocol::~GpsrIpv4RoutingProtocol ()
{
  NS_LOG_FUNCTION (this);
  delete m_locationTable;
}

void
GpsrIpv4RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;
  Ipv4RoutingProtocol::DoDispose ();
}

void
GpsrIpv4RoutingProtocol::NotifyInterfaceUp (uint32_t interface)
{
  NS_ASSERT (m_ipv4 != 0);
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());

  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  NS_ASSERT (l3 != 0);

  if (l3->GetNAddresses (interface) > 1)
    {
      NS_FATAL_ERROR ("GPSR does not work with more than one address per each interface.");
    }

  Ipv4InterfaceAddress ifaceAddr = l3->GetAddress (interface, 0);
  if (ifaceAddr.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      NS_LOG_INFO ("Loopback interface not managed by GPSR.");
      return;
    }

  // Create a socket to listen only on this interface
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                             UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&GpsrIpv4RoutingProtocol::RecvGPSR, this));
  InetSocketAddress inetAddr (m_ipv4->GetAddress (interface, 0).GetLocal (), GPSR_PORT);
  if (socket->Bind (inetAddr))
    {
      NS_FATAL_ERROR ("Failed to bind() GPSR Routing socket");
    }
  socket->Connect (InetSocketAddress (Ipv4Address (0xffffffff), GPSR_PORT));
  socket->BindToNetDevice (l3->GetNetDevice (interface));
  socket->SetAllowBroadcast (true);

  socket->SetAttribute ("IpTtl", UintegerValue (1));
  m_socketToAddresses.insert (std::make_pair (socket, ifaceAddr));
}

void
GpsrIpv4RoutingProtocol::Initialize ()
{
  NS_LOG_FUNCTION (this);

  UniformRandomVariable x;

  Time maxJitter = m_helloInterval / 2;
  Time jitter = Seconds (x.GetValue (0.0, maxJitter.GetSeconds ()));

  m_helloTimer.Cancel ();
  m_helloTimer.SetFunction (&GpsrIpv4RoutingProtocol::HelloTimerExpire, this);
  m_helloTimer.Schedule (jitter);

  switch (m_locationTableType)
    {
    case GPSR_GOD:
      m_locationTable = new GodIpv4LocationTable ();
      break;
    default:
      NS_FATAL_ERROR ("Selected mode is not valid");
    }
}

void
GpsrIpv4RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);

  m_ipv4 = ipv4;

  Simulator::ScheduleNow (&GpsrIpv4RoutingProtocol::Initialize, this);
}

void
GpsrIpv4RoutingProtocol::HelloTimerExpire ()
{
  NS_LOG_FUNCTION (this);

  SendHello ();

  UniformRandomVariable x;

  Time maxJitter = m_helloInterval / 2;
  Time jitter = Seconds (x.GetValue (0.0, maxJitter.GetSeconds ()));

  m_helloTimer.Cancel ();
  m_helloTimer.SetFunction (&GpsrIpv4RoutingProtocol::HelloTimerExpire, this);
  m_helloTimer.Schedule (m_helloInterval + jitter);
}

void
GpsrIpv4RoutingProtocol::SendHello ()
{
  NS_LOG_FUNCTION (this);

  Ptr<MobilityModel> mobilityModel = m_ipv4->GetObject<MobilityModel> ();

  if (mobilityModel == 0)
    {
      NS_FATAL_ERROR ("Cannot use a location-based routing protocol without a"
                      " mobility model installed on the node, I'm sorry");
    }

  double positionX = mobilityModel->GetPosition ().x;
  double positionY = mobilityModel->GetPosition ().y;

  for (SocketMap::const_iterator j = m_socketToAddresses.begin (); j != m_socketToAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;

      GpsrHelloHeader helloHeader;
      helloHeader.SetOriginPosx (positionX);
      helloHeader.SetOriginPosy (positionY);

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (helloHeader);

      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      socket->SendTo (packet, 0, InetSocketAddress (destination, GPSR_PORT));
    }
}

void
GpsrIpv4RoutingProtocol::RecvGPSR (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Address sourceAddress;
  GpsrHelloHeader hdr;

  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  uint32_t readBytes = packet->RemoveHeader (hdr);

  if (readBytes == 0)
    {
      NS_LOG_DEBUG ("This is not an Hello Packet");
    }
  else
    {
      Vector position;
      position.x = hdr.GetOriginPosx ();
      position.y = hdr.GetOriginPosy ();
      InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
      Ipv4Address sender = inetSourceAddr.GetIpv4 ();
      Ipv4Address receiver = m_socketToAddresses[socket].GetLocal ();
      uint32_t interfaceIdx = m_ipv4->GetInterfaceForAddress (receiver);

      m_neighborsTable.AddEntry (sender, position);
      m_addressToOif.insert (std::make_pair (sender, interfaceIdx));
    }
}

void
GpsrIpv4RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << " interface " << interface << " address " << address);
}

void
GpsrIpv4RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << address);
}

Ptr<Socket>
GpsrIpv4RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);

  for (SocketMap::const_iterator j =  m_socketToAddresses.begin (); j != m_socketToAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        {
          return socket;
        }
    }

  return 0;
}

void
GpsrIpv4RoutingProtocol::NotifyInterfaceDown (uint32_t interface)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());

  // Close socket
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (interface, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketToAddresses.erase (socket);
  if (m_socketToAddresses.empty ())
    {
      NS_LOG_LOGIC ("No gpsr interfaces");
      m_neighborsTable.Clear ();
      m_locationTable->Clear ();
    }
}

void
GpsrIpv4RoutingProtocol::PrintRoutingTable (ns3::Ptr<ns3::OutputStreamWrapper> os) const
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("NOT IMPLEMENTED YET");
}

Ptr<Ipv4Route>
GpsrIpv4RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                                      Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));

  if (p == 0)
    {
      NS_FATAL_ERROR ("Trying to find a route for an empty packet ?!?");
    }

  Ipv4Address dst = header.GetDestination ();

  // when sending on local multicast, there have to be interface specified
  if (dst.IsLocalMulticast ())
    {
      NS_ASSERT_MSG (oif, "Try to send on link-local multicast address, and no interface index is given!");

      Ptr<Ipv4Route> rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (dst);
      rtentry->SetGateway (Ipv4Address::GetZero ());
      rtentry->SetOutputDevice (oif);
      rtentry->SetSource (m_ipv4->GetAddress (m_ipv4->GetInterfaceForDevice (oif), 0).GetLocal ());
      return rtentry;
    }

  // That packet will go outside. Find a destination for it only if we have
  // connection with other nodes.
  if (m_socketToAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_WARN ("No gpsr socket");
      return 0;
    }

  // Multicast goes here
  if (dst.IsMulticast ())
    {
      // Note:  Multicast routes for outbound packets are stored in the
      // normal unicast table.  An implication of this is that it is not
      // possible to source multicast datagrams on multiple interfaces.
      // This is a well-known property of sockets implementation on
      // many Unix variants.
      // So, we just log it and fall through the rest of the function
      NS_LOG_LOGIC ("Multicast destination");
    }

  Vector dstPos = m_locationTable->GetPosition (dst);

  if (CalculateDistance (dstPos, m_locationTable->GetInvalidPosition ()) == 0
      && m_locationTable->IsInSearch (dst))
    {
      // Old code in this case just "defer" the routing phase.
      // This is clearly _stupid_. We have been asked for a route, we don't
      // have the position of the destination. We can't do much.
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_WARN ("No dest position for gpsr");
      return 0;
    }

  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();

  if (MM == 0)
    {
      NS_FATAL_ERROR ("Cannot use GPSR without a mobility model");
    }

  Vector myPos = MM->GetPosition ();
  Address nextHop;

  if (m_neighborsTable.IsNeighbour (dst))
    {
      nextHop = dst;
    }
  else
    {
      nextHop = m_neighborsTable.BestNeighbour (dstPos, myPos);
    }

  if (!nextHop.IsInvalid ())
    {
      Ipv4Address ipv4_nextHop = Ipv4Address::ConvertFrom (nextHop);
      AddressMap::iterator it = m_addressToOif.find (ipv4_nextHop);

      if (it == m_addressToOif.end ())
        {
          NS_FATAL_ERROR ("We don't have the output interface for the next hop");
        }

      int32_t interfaceIdx = it->second;

      if (interfaceIdx < 0)
        {
          NS_FATAL_ERROR ("Our interface is less than zero; error at the insertion time");
        }
      if (m_ipv4->GetNAddresses (interfaceIdx) > 1)
        {
          NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and GPSR");
        }

      sockerr = Socket::ERROR_NOTERROR;
      Ptr<Ipv4Route> route = Create<Ipv4Route> ();

      NS_LOG_DEBUG ("Destination: " << dst << " next hop: " << ipv4_nextHop);

      route->SetDestination (dst);
      route->SetGateway (ipv4_nextHop);
      route->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
      route->SetSource (m_ipv4->GetAddress (interfaceIdx, 0).GetLocal ());

      if (oif != 0 && m_ipv4->GetInterfaceForDevice (oif) != interfaceIdx)
        {
          // We do not attempt to perform a constrained routing search
          // if the caller specifies the oif; we just enforce that
          // that the found route matches the requested outbound interface
          NS_LOG_DEBUG ("GPSR node " << m_ipv4->GetObject<Node> ()->GetId ()
                                     << ": RouteOutput for dest=" << header.GetDestination ()
                                     << " Route interface " << interfaceIdx
                                     << " does not match requested output interface "
                                     << m_ipv4->GetInterfaceForDevice (oif));
          sockerr = Socket::ERROR_NOROUTETOHOST;
          return 0;
        }

      return route;
    }
  else
    {
      // Old code in this case just "defer" the routing phase.
      // This is clearly _stupid_. We have been asked for a route, we don't
      // have a valid nexthop. We can't do much.
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_WARN ("No nexthop for gpsr");
      return 0;
    }
}

bool
GpsrIpv4RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
                                     Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                                     MulticastForwardCallback mcb, LocalDeliverCallback lcb,
                                     ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << header);
  NS_ASSERT (m_ipv4 != 0);
  NS_ASSERT (p != 0);
  NS_ASSERT (idev != 0);
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);

  int32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  Ipv4Address dst = header.GetDestination ();

  if (dst.IsMulticast ())
    {
      NS_LOG_LOGIC ("Multicast destination");
      return false; // Let other routing protocols try to handle this

    }

  if (m_ipv4->IsDestinationAddress (dst, iif))
    {
      // Old code checks for GPSR headers and remove them.
      // But if packets are for our socket, we should do this on RecvGPSR function.
      // TODO Check for headers that are not removed

      if (!lcb.IsNull ())
        {
          NS_LOG_LOGIC ("Local delivery to " << header.GetDestination ());
          lcb (p, header, iif);
          return true;
        }
      else
        {
          // The local delivery callback is null.  This may be a multicast
          // or broadcast packet, so return false so that another
          // multicast routing protocol can handle it.  It should be possible
          // to extend this to explicitly check whether it is a unicast
          // packet, and invoke the error callback if so
          return false;
        }
    }

  if (m_socketToAddresses.empty ())
    {
      NS_LOG_WARN ("No GPSR interfaces, no neighbour, returning false");
      return false;
    }

  return Forwarding (p, header, ucb, ecb);
}

bool
GpsrIpv4RoutingProtocol::Forwarding (Ptr<const Packet> packet, const Ipv4Header & header,
                                     UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> p = packet->Copy ();
  Ipv4Address dst = header.GetDestination ();

  m_neighborsTable.Purge ();

  uint32_t updated = 0;
  Vector packetPos;
  Vector recoveryPos;
  bool inRec = false;

  GpsrPositionHeader posHeader;
  uint32_t bytesRead = p->PeekHeader (posHeader);
  if (bytesRead > 0)
    {
      p->RemoveHeader (posHeader);
      packetPos.x = posHeader.GetDstPosx ();
      packetPos.y = posHeader.GetDstPosy ();
      updated = posHeader.GetUpdated ();
      recoveryPos.x = posHeader.GetRecPosx ();
      recoveryPos.y = posHeader.GetRecPosy ();
      if (posHeader.GetInRec () == 1)
        {
          inRec = true;
        }
    }
  else
    {
      // TODO note no posHeader removed. This happens on the first node
      // (because RouteOutput does not set a GpsrPositionHeader on the packet)
    }

  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();

  if (MM == 0)
    {
      NS_FATAL_ERROR ("Cannot use GPSR without a mobility model");
    }

  Vector myPos = MM->GetPosition ();
  Ipv4Address nextHop;
  Ptr<NetDevice> oif;

  if (inRec)
    {
      if (CalculateDistance (myPos, packetPos) < CalculateDistance (recoveryPos, packetPos))
        {
          inRec = false;
          NS_LOG_LOGIC ("No longer in Recovery to " << dst << " in " << myPos);
        }
    }
  else
    {
      if (m_neighborsTable.IsNeighbour (dst))
        {
          nextHop = dst;
        }
      else
        {
          Address foundNextHop = m_neighborsTable.BestNeighbour (packetPos, myPos);
          if (foundNextHop.IsInvalid ())
            {
              inRec = true;
            }
          else
            {
              nextHop = Ipv4Address::ConvertFrom (foundNextHop);
            }
        }
    }

  uint32_t myUpdated = (uint32_t) m_locationTable->GetEntryUpdateTime (dst).GetSeconds ();

  if (myUpdated > updated)
    {
      packetPos = m_locationTable->GetPosition (dst);
      updated = myUpdated;
    }

  posHeader.SetDstPosx (packetPos.x); // This can be updated, set it
  posHeader.SetDstPosy (packetPos.y); // This can be updated, set it
  posHeader.SetUpdated(updated);

  if (inRec)
    {
      Vector previousHop;
      previousHop.x = posHeader.GetLastPosx ();
      previousHop.y = posHeader.GetLastPosy ();

      posHeader.SetInRec (1);
      posHeader.SetRecPosx (myPos.x);
      posHeader.SetRecPosy (myPos.y);

      Address foundNextHop = m_neighborsTable.BestAngle (previousHop, myPos);

      if (foundNextHop.IsInvalid ())
        {
          NS_LOG_WARN ("No route found, even in recovery mode.");
          ecb (p, header, Socket::ERROR_NOROUTETOHOST);
          return false; // try some other protocol
        }

      posHeader.SetLastPosx (packetPos.x); // when entering Recovery, the first edge is the Dst
      posHeader.SetLastPosy (packetPos.y); // TODO does it make sense?

      nextHop = Ipv4Address::ConvertFrom (foundNextHop);

      NS_LOG_LOGIC ("Entering recovery-mode to " << dst);
    }
  else
    {
      posHeader.SetInRec (0);
      posHeader.SetRecPosx (0);
      posHeader.SetRecPosy (0);
      posHeader.SetLastPosx (myPos.x);
      posHeader.SetLastPosy (myPos.y);
    }

  // From here, nextHop is valid, or there is a bug :/ . So now retrieve
  // the interface on which nextHop can be reached.
  AddressMap::iterator it = m_addressToOif.find (nextHop);

  if (it == m_addressToOif.end ())
    {
      NS_FATAL_ERROR ("We don't have the output interface for the next hop");
    }

  int32_t interfaceIdx = it->second;

  if (interfaceIdx < 0)
    {
      NS_FATAL_ERROR ("Our interface is less than zero; error at the insertion time");
    }
  if (m_ipv4->GetNAddresses (interfaceIdx) > 1)
    {
      NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and GPSR");
    }

  oif = m_ipv4->GetNetDevice (interfaceIdx);

  Ptr<Ipv4Route> route = Create<Ipv4Route> ();

  route->SetDestination (dst);
  route->SetSource (header.GetSource ());
  route->SetGateway (nextHop);
  route->SetOutputDevice (oif);

  NS_LOG_DEBUG ("Exist route to " << route->GetDestination () <<
                " from interface " << route->GetOutputDevice ());

  p->AddHeader (posHeader);
  ucb (route, p, header);
  return true;
}

} //namespace ns3
