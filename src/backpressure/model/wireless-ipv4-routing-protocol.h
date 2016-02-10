#ifndef WIRELESS_IPV4_ROUTING_PROTOCOL_H
#define WIRELESS_IPV4_ROUTING_PROTOCOL_H

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/ipv4-routing-protocol.h"

namespace ns3 {

class RoutingMetric;

class WirelessIpv4RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);
  virtual ~WirelessIpv4RoutingProtocol();
  virtual void SendHello();
  void SetRoutingMetric(Ptr<RoutingMetric> rtMetric);
  Ptr<RoutingMetric> GetRoutingMetric();

 virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr) = 0;
 virtual void DoDispose ();

protected:
  Ptr<RoutingMetric> m_rtMetric;
};

} // namespace ns3

#endif /*WIRELESS_IPV4_ROUTING_PROTOCOL_H*/
