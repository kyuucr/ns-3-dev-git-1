#include "routing-metric.h"
#include "wireless-ipv4-routing-protocol.h"

NS_LOG_COMPONENT_DEFINE("WirelessIpv4RoutingProtocol");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WirelessIpv4RoutingProtocol);
TypeId WirelessIpv4RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WirelessIpv4RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    ;
  return tid;
}
void WirelessIpv4RoutingProtocol::DoDispose()
{
  m_rtMetric=0;
  Ipv4RoutingProtocol::DoDispose();
}
void 
WirelessIpv4RoutingProtocol::SendHello()
{
}

void
WirelessIpv4RoutingProtocol::SetRoutingMetric(Ptr<RoutingMetric> rtMetric)
{
}

Ptr<RoutingMetric>
WirelessIpv4RoutingProtocol::GetRoutingMetric()
{
  return m_rtMetric;
}

WirelessIpv4RoutingProtocol::~WirelessIpv4RoutingProtocol ()
{
}

} // namespace ns3
