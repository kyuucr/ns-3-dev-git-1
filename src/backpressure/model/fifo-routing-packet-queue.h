#ifndef FIFO_ROUTING_PACKET_QUEUE_H
#define FIFO_ROUTING_PACKET_QUEUE_H

#include "ns3/routing-packet-queue.h"

namespace ns3 {

class FifoRoutingPacketQueue: public RoutingPacketQueue {
public:
  static TypeId GetTypeId (void);
  
  FifoRoutingPacketQueue ();
  virtual ~FifoRoutingPacketQueue ();
  
  FifoRoutingPacketQueue* Copy (void) const;//virtual copy constructor

private:
//Original functions
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


  typedef std::list<EntryRoutingPacketQueue> ListPacketQueue;
  typedef std::list<EntryRoutingPacketQueue>::iterator ListPacketQueueI;
  ListPacketQueue m_packetQueue;
  std::vector<uint32_t> m_queue_directions;

};

}; //namespace ns3

#endif //FIFO ROUTING PACKET QUEUE
