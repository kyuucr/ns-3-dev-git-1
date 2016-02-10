#ifndef PRIORITY_ROUTING_PACKET_QUEUE_H
#define PRIORITY_ROUTING_PACKET_QUEUE_H

#include "ns3/routing-packet-queue.h"

namespace ns3 {

/*
class EntryPriorityRoutingPacketQueue : public EntryRoutingPacketQueue
{
public:
  EntryPriorityRoutingPacketQueue(Ptr<const Packet> p = 0, Ipv4Header const header = Ipv4Header (), 
		UnicastForwardCallback ucb = UnicastForwardCallback (), Time tstamp = Simulator::Now (), uint32_t htime=64) :
                EntryRoutingPacketQueue(p, header, ucb, tstamp), m_HopTime(htime)
  {}
  uint32_t GetHopTime() {return m_HopTime;}
  void SetHopTime(uint32_t htime) {m_HopTime=htime;}
  uint32_t m_HopTime;  //time in hops including queueing delays
};
*/

class PriorityRoutingPacketQueue : public RoutingPacketQueue
{
public:
  static TypeId GetTypeId (void);
  
  PriorityRoutingPacketQueue ();
  virtual ~PriorityRoutingPacketQueue ();
  
  PriorityRoutingPacketQueue* Copy (void) const;//virtual copy constructor

private:
//Original functions
  //virtual bool DoEnqueue(Ptr<const Packet> packet, Ipv4Header header, UnicastForwardCallback ucb, Time now);
  virtual bool DoEnqueue(EntryRoutingPacketQueue e);
  virtual bool DoDequeue(EntryRoutingPacketQueue &e);
  virtual bool DoReadEntry(EntryRoutingPacketQueue &e);
  virtual bool DoFlush();
//Added functions
  virtual bool DoEnqueue(EntryRoutingPacketQueue e, uint32_t direction);
  virtual bool DoDequeue(EntryRoutingPacketQueue &e, uint32_t &direction);
  virtual bool DoReadEntry(EntryRoutingPacketQueue &e, uint32_t &direction);
  virtual bool DoEnqueueFirst(EntryRoutingPacketQueue e, uint32_t direction);
  virtual bool DoDequeueLast(EntryRoutingPacketQueue &e, uint32_t &direction);


  typedef std::list<EntryRoutingPacketQueue> ListPriorityPacketQueue;
  typedef std::list<EntryRoutingPacketQueue>::iterator ListPriorityPacketQueueI;
  
  ListPriorityPacketQueue m_priorityPacketQueue;
  std::vector<uint32_t> m_queue_directions;

  static bool ComparePackets(EntryRoutingPacketQueue e1, EntryRoutingPacketQueue e2);
};

}; //namespace ns3

#endif //PRIORITY ROUTING PACKET QUEUE
