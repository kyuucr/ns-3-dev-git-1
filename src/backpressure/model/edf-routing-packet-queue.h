#ifndef EDF_ROUTING_PACKET_QUEUE_H
#define EDF_ROUTING_PACKET_QUEUE_H

#include "ns3/routing-packet-queue.h"

namespace ns3 {

class EdfRoutingPacketQueue : public RoutingPacketQueue
{
public:
  static TypeId GetTypeId (void);
  
  EdfRoutingPacketQueue ();
  virtual ~EdfRoutingPacketQueue ();
  
  EdfRoutingPacketQueue* Copy (void) const;//virtual copy constructor

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

  typedef std::list<EntryRoutingPacketQueue> ListEdfPacketQueue;
  typedef std::list<EntryRoutingPacketQueue>::iterator ListEdfPacketQueueI;
  
  ListEdfPacketQueue m_edfPacketQueue;
  std::vector<uint32_t> m_queue_directions;

  static bool ComparePackets(EntryRoutingPacketQueue e1, EntryRoutingPacketQueue e2);
};

}; //namespace ns3

#endif //EDF ROUTING PACKET QUEUE
