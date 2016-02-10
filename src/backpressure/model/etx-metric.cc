#include "ns3/wireless-ipv4-routing-protocol.h"
#include "ns3/backpressure.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-list-routing.h"
#include "etx-metric.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("EtxMetric");

NS_OBJECT_ENSURE_REGISTERED (EtxMetric);
TypeId EtxMetric::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EtxMetric")
    .SetParent<RoutingMetric> ()
    .AddConstructor<EtxMetric> ()
    .AddAttribute ("SentHellos",
		   "The number of packets to sent during a window interval",
		   UintegerValue(0),
		   MakeUintegerAccessor(&EtxMetric::m_sntHellos),
   		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RcvdHellos",
		   "The number of received packets during a window interval",
		   UintegerValue(0),
		   MakeUintegerAccessor(&EtxMetric::m_rcvHellos),
		   MakeUintegerChecker<uint32_t> ())
    /*.AddAttribute ("Period",
		   "The mean distribution of the probing",
		   TimeValue(Seconds(0.1)),
		   MakeTimeAccessor(&EtxMetric::m_period),
		   MakeTimeChecker ())*/
    .AddAttribute ("Alpha",
                   "The weight of the EWMA",
                   DoubleValue(0.0),
                   MakeDoubleAccessor(&EtxMetric::m_alpha),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("WindowInterval",
		   "The window Interval",
		   TimeValue(Seconds(1)),
		   MakeTimeAccessor(&EtxMetric::m_windowInterval),
		   MakeTimeChecker ())
    ;
  return tid;
}


EtxMetric::EtxMetric () :
  m_probingTimer(Timer::CANCEL_ON_DESTROY),
  m_windowTimer(Timer::CANCEL_ON_DESTROY)
{
  //NS_LOG_DEBUG ("Initialize etx metric object");
  m_probingTimer.SetFunction (&EtxMetric::ProbingTimerExpire, this);
  m_windowTimer.SetFunction (&EtxMetric::WindowTimerExpire, this);
  //Simulator::Schedule(Seconds(30), &EtxMetric::BeginCountingReceivedPkts,this);//jnunez to count convergence time
  //Simulator::Schedule(Seconds(0), &EtxMetric::BeginCountingReceivedPkts,this);
  m_sntHellos = 0;
  m_rcvHellos = 0;
  m_pdr2Send = 0;
}

EtxMetric::~EtxMetric ()
{
}

uint16_t EtxMetric::SendPacketProbing()
{
  return 0;
}

void
EtxMetric::AddEtxNeighborTuple (const EtxNeighborTuple &tuple)
{
  for (EtxSet::iterator it = m_etxSet.begin ();
       it != m_etxSet.end (); it++)
    {
      if (it->addr == tuple.addr)
        {
          // Update it
          *it = tuple;
          return;
        }
    }
  m_etxSet.push_back (tuple);
}

EtxNeighborTuple*
EtxMetric::FindNeighborEtxTuple(Ipv4Address const &addr)
{
  for (EtxSet::iterator it = m_etxSet.begin ();
       it != m_etxSet.end (); it++)
    {
      if (it->addr == addr)
        return &(*it);
    }
  return NULL;
}

void 
EtxMetric::RcvPacketProbing(const Ipv4Address &senderIface, uint32_t recvSequenceNumber, uint16_t metric)
{
  EtxNeighborTuple *etx_tuple = FindNeighborEtxTuple (senderIface);
  if (etx_tuple != NULL)
    {
      etx_tuple->addr = senderIface;
      etx_tuple->lastRcvSeqNum = recvSequenceNumber;
      etx_tuple->rcvHellos++;
      etx_tuple->fdr = double(metric)/100;
//NS_LOG_UNCOND ("[EtxMetric::RcvPacketProbing] ipv4 address is "<<senderIface<<" FDR is double is" <<etx_tuple->fdr<<" fdr in uint is "<< metric);
    }
  else
    { //new neighbor to be added to the list
      EtxNeighborTuple aetx_tuple;
      aetx_tuple.addr = senderIface;
      aetx_tuple.lastRcvSeqNum = recvSequenceNumber;
      aetx_tuple.firstRcvSeqNum = recvSequenceNumber;
      aetx_tuple.rcvHellos=1;
      aetx_tuple.pdr = 1.0;
      aetx_tuple.fdr = double(metric)/100;  //the metric is the fdr of the hello sender
//NS_LOG_UNCOND ("[EtxMetric::RcvPacketProbing] FDR is " <<aetx_tuple.fdr);
//NS_LOG_UNCOND ("[EtxMetric::RcvPacketProbing] New SequenceNumber Received is " <<recvSequenceNumber);
//NS_LOG_UNCOND ("[EtxMetric::RcvPacketProbing] sender ipv4 address is "<<senderIface<<" FDR is double is " <<aetx_tuple.fdr<<" fdr in uint is "<< metric);
      AddEtxNeighborTuple(aetx_tuple);
    }
//NS_LOG_DEBUG ("[EtxMetric::RcvPacketProbing] SequenceNumber Received is " <<recvSequenceNumber<< "  received pakets are "<<m_rcvHellos);
  //NS_LOG_DEBUG ("[EtxMetric::RcvPacketProbing] Received Packet");
}

uint16_t
EtxMetric::GetMetric4Hello(Ipv4Address &addr) //this code needs REVIEW
{
  if (m_etxSet.size()>0)
    {
      uint16_t aPdr = (uint16_t) (m_etxSet[m_pdr2Send].pdr*100);
      addr =  (m_etxSet[m_pdr2Send].addr);
NS_LOG_DEBUG ("Address is "<<m_etxSet[m_pdr2Send].addr);
      m_pdr2Send++;
      if ( m_pdr2Send >= m_etxSet.size() )
        {
          m_pdr2Send=0;
        }
      return aPdr;
    }
  else
    {
      addr = Ipv4Address("0.0.0.0");
      return 100;
    }
}

void
EtxMetric::DoStart()
{
  //NS_LOG_DEBUG ("calling probingtimerexpire");
  ProbingTimerExpire ();
  WindowTimerExpire ();
}

void
EtxMetric::DoDispose()
{
  RoutingMetric::DoDispose();
}

void
EtxMetric::WindowTimerExpire()
{
  //recalculate ETX metric for every neighbor
  //EWMA
  for (EtxSet::iterator it = m_etxSet.begin ();
       it != m_etxSet.end (); it++)
    {
      NS_ASSERT(it->lastRcvSeqNum >= it->firstRcvSeqNum);
      it->pdr = (double) it->rcvHellos/(it->lastRcvSeqNum-it->firstRcvSeqNum);
      it->etx = m_alpha*(it->etx) + (1-m_alpha)*(1/(it->pdr*it->fdr));
//NS_LOG_UNCOND("m_alpha is "<<m_alpha<<" etx is "<<it->etx<<" fdr is "<<it->fdr<<" pdr is "<<it->pdr<<"hellos received are "<<it->rcvHellos<<" last Seq "<<it->lastRcvSeqNum<<" firs Seq "<<it->firstRcvSeqNum);
      it->firstRcvSeqNum = it->lastRcvSeqNum;
      it->rcvHellos=0;
    }
    Simulator::Schedule(m_windowInterval, &EtxMetric::WindowTimerExpire, this);
}

void
EtxMetric::ProbingTimerExpire()
{
  //locate instance of the routing protocol and call SendHello(etx-info)
  // NS_LOG_UNCOND ("[EtxMetric] Send hello");
  Ptr<Ipv4RoutingProtocol> tmp = m_ipv4->GetRoutingProtocol();
  Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (tmp);

  Ptr<backpressure::RoutingProtocol> rp;

  for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
    {
      int16_t priority;
      Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
      if (DynamicCast<backpressure::RoutingProtocol> (temp))
        {
          rp = DynamicCast<backpressure::RoutingProtocol> (temp);
        }
    }

  //Ptr<WirelessIpv4RoutingProtocol> theWirelessRoutingProtocol = DynamicCast<WirelessIpv4RoutingProtocol> (thetmp);
  //Ptr<backpressure::RoutingProtocol> theRoutingProtocol = DynamicCast<backpressure::RoutingProtocol> (theWirelessRoutingProtocol);
 
  rp->SendHello();
  //ExponentialVariable poissonIntArrival(m_period.GetSeconds());
  //periodically sent link probe
  //NS_LOG_UNCOND("hello interval "<<m_helloInterval);
  m_probingTimer.Schedule(m_helloInterval);
  m_sntHellos++;
  //probingTimer.Schedule(Seconds(poissonIntArrival.GetValue()));
  //m_probingTimer.Schedule(Seconds(0.1));
} 

void
EtxMetric::BeginCountingReceivedPkts()
{
  //instant 30 begin to count received packets
  m_rcvHellos = 1;
}

uint32_t 
EtxMetric::GetPktsReceived()
{
  return m_rcvHellos;		
}
} // namespace ns3
