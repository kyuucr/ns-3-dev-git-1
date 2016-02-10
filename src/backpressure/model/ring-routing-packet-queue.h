#ifndef RING_ROUTING_PACKET_QUEUE_H
#define RING_ROUTING_PACKET_QUEUE_H

#include "ns3/routing-packet-queue.h"

namespace ns3 {

class RingRoutingPacketQueue: public RoutingPacketQueue {
public:
  static TypeId GetTypeId (void);
  
  RingRoutingPacketQueue ();
  virtual ~RingRoutingPacketQueue ();
  
  RingRoutingPacketQueue* Copy (void) const;//virtual copy constructor

private:
  //Original functions
  virtual bool DoEnqueue(EntryRoutingPacketQueue e);
  virtual bool DoDequeue(EntryRoutingPacketQueue &e);
  virtual bool DoReadEntry(EntryRoutingPacketQueue &e);
//Added functions
  virtual bool DoEnqueue(EntryRoutingPacketQueue e, uint32_t input_iface);
  virtual bool DoEnqueueFirst(EntryRoutingPacketQueue e, uint32_t output_iface);
  virtual bool DoDequeue(EntryRoutingPacketQueue &e, uint32_t &output_iface);
  virtual bool DoDequeueLast(EntryRoutingPacketQueue &e, uint32_t &output_iface);
  virtual bool DoReadEntry(EntryRoutingPacketQueue &e, uint32_t &output_iface);
  virtual bool DoFlush();
  
  typedef std::list<EntryRoutingPacketQueue> ListPacketQueue;
  typedef std::list<EntryRoutingPacketQueue>::iterator ListPacketQueueI;
  ListPacketQueue m_packetQueue_if1; //packet are queued at the "corresponding" output queue.
  ListPacketQueue m_packetQueue_if2;
  //std::vector<uint32_t> m_queue_directions; ->implicit in the queue the packet is inserted

};

}; //namespace ns3

#endif //RING ROUTING PACKET QUEUE
