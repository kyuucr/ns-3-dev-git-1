#ifndef ROUTING_METRIC_H
#define ROUTING_METRIC_H

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/timer.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

//class WirelessIpv4RoutingProtocol;
class Ipv4;

class RoutingMetric : public Object
{
public:
  static TypeId GetTypeId (void);
  RoutingMetric();
  virtual ~RoutingMetric();
  virtual uint16_t SendPacketProbing();
  virtual void RcvPacketProbing(const Ipv4Address &addr1, uint32_t seq, uint16_t metric);
  virtual uint16_t GetMetric4Hello(Ipv4Address &addr);
  //void SetWirelessIpv4RoutingProtocol(Ptr<WirelessIpv4RoutingProtocol> rtProtocol);
  void SetIpv4(Ptr<Ipv4> theIpv4);
  void SetHelloInterval(Time helloInterval);
protected:  
  //Ptr<WirelessIpv4RoutingProtocol> m_rtProtocol;
  virtual void DoDispose();
  Ptr<Ipv4> m_ipv4;
  Time m_helloInterval;
};

} // namespace ns3

#endif /*ROUTING_METRIC_H*/
