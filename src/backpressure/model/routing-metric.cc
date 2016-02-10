#include "wireless-ipv4-routing-protocol.h"
#include "routing-metric.h"

NS_LOG_COMPONENT_DEFINE("RoutingMetric");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RoutingMetric);
TypeId RoutingMetric::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RoutingMetric")
    .SetParent<Object> ()
    .AddConstructor<RoutingMetric>()
    ;
  return tid;
}

RoutingMetric::RoutingMetric()
{
  m_ipv4=0;
}

uint16_t 
RoutingMetric::SendPacketProbing()
{
  return 0;
}

void 
RoutingMetric::RcvPacketProbing(const Ipv4Address &addr1, uint32_t seq, uint16_t metric)
{
}

uint16_t
RoutingMetric::GetMetric4Hello(Ipv4Address &addr)
{
  return 1;
}
void
RoutingMetric::SetIpv4(Ptr<Ipv4> theIpv4)
{
  m_ipv4 = theIpv4;
}

void
RoutingMetric::SetHelloInterval(Time helloInterval)
{
NS_LOG_UNCOND("hello interval is "<<helloInterval);
  m_helloInterval = helloInterval;
}
/*
void
RoutingMetric::SetWirelessIpv4RoutingProtocol(Ptr<WirelessIpv4RoutingProtocol> rtProtocol)
{
  m_rtProtocol = rtProtocol;
}
*/
void
RoutingMetric::DoDispose()
{
  m_ipv4=0;
}
RoutingMetric::~RoutingMetric ()
{
}

} // namespace ns3
