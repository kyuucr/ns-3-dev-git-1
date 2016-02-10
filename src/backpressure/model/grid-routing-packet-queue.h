#ifndef GRID_ROUTING_PACKET_QUEUE_H
#define GRID_ROUTING_PACKET_QUEUE_H

#include "ns3/routing-packet-queue.h"

namespace ns3 {

class GridRoutingPacketQueue: public RoutingPacketQueue {
public:
  static TypeId GetTypeId (void);
  
  GridRoutingPacketQueue ();
  GridRoutingPacketQueue (uint32_t);
  virtual ~GridRoutingPacketQueue ();
  
  GridRoutingPacketQueue* Copy (void) const;//virtual copy constructor

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
  std::vector<ListPacketQueue> m_packetQueue;
  uint32_t m_interfaces;
  
  //packet are queued at the "corresponding" output queue, initialized with the constructor

};

}; //namespace ns3

#endif //GRID ROUTING PACKET QUEUE
