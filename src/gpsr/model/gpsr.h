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
#ifndef GPSR_H
#define GPSR_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/god-location-table.h"

#include "gpsr-neighbour-table.h"

namespace ns3 {

/**
 * \brief The Gpsr Routing Protocol
 *
 */
class GpsrIpv4RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  static const uint32_t GPSR_PORT;
  static TypeId GetTypeId (void);

  GpsrIpv4RoutingProtocol ();
  ~GpsrIpv4RoutingProtocol ();

  enum GpsrLocationTableType
  {
    GPSR_GOD = 0
  };

  // From Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                                      Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
                           Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                           MulticastForwardCallback mcb, LocalDeliverCallback lcb,
                           ErrorCallback ecb);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (ns3::Ptr<ns3::OutputStreamWrapper>) const;

protected:
  void Initialize ();

private:
  void DoDispose ();
  void HelloTimerExpire ();
  void RecvGPSR (Ptr<Socket> socket);
  void SendHello ();
  bool Forwarding (Ptr<const Packet> packet, const Ipv4Header & header,
                   UnicastForwardCallback ucb, ErrorCallback ecb);
  Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;

private:
  typedef std::map< Ptr<Socket>, Ipv4InterfaceAddress > SocketMap;
  typedef std::map< Ipv4Address, int32_t > AddressMap;

  Timer m_helloTimer;
  Timer m_checkQueueTimer;
  Ptr<Ipv4> m_ipv4;
  GpsrIpv4NeighbourTable m_neighborsTable;
  AbstractLocationTable *m_locationTable;

  /// Raw socket per each IP interface, map socket -> iface address (IP + mask)
  SocketMap m_socketToAddresses;
  AddressMap m_addressToOif;

  Time m_helloInterval;                      //!< Interval of HELLO packets
  GpsrLocationTableType m_locationTableType; //!< Type of the Location Service
};

} // namespace ns3
#endif /* GPSRROUTINGPROTOCOL_H */
