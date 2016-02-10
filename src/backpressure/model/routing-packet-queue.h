#ifndef ROUTING_PACKET_QUEUE_H
#define ROUTING_PACKET_QUEUE_H


#include <list>
#include <map>
#include <utility>
#include "ns3/mac48-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
class EntryRoutingPacketQueue 
{
public:
  typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
  EntryRoutingPacketQueue(Ptr<const Packet> p = 0, Ipv4Header const header = Ipv4Header (), const Mac48Address from = Mac48Address("00:00:00:00:00:00"), 
		UnicastForwardCallback ucb = UnicastForwardCallback (), Time tstamp = Simulator::Now ()) :
                m_packet(p), m_header(header), m_ucb(ucb), m_tstamp(tstamp) 
  {
    m_hwaddress = from;
  }
  
  Ptr<const Packet> GetPacket(){return m_packet;}
  Ipv4Header GetHeader(){return m_header;} 
  Mac48Address GetHwAddres(){return m_hwaddress;}
  UnicastForwardCallback GetUcb(){return m_ucb;}
  Time GetTstamp(){return m_tstamp;}
  void SetPacket(Ptr<Packet> p){m_packet=p;}
  void SetHeader(Ipv4Header header){m_header=header;}
  void SetUcb(UnicastForwardCallback ucb){m_ucb=ucb;}
  void SetTstamp(Time tstamp){m_tstamp=tstamp;}
    
  Ptr<const Packet> m_packet;
  Mac48Address m_hwaddress;
  Ipv4Header  m_header;
  UnicastForwardCallback m_ucb;
  Time m_tstamp;
  uint32_t requeued; //0 no requeued, 1 has been requeued, 2 requeued by congestion at source node
};

class RoutingPacketQueue: public Object
{
public: 
  static TypeId GetTypeId (void);
  RoutingPacketQueue ();
  ~RoutingPacketQueue ();
  
  typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
  //original enqueueing/dequeueing/ReadEntry operations
  bool Enqueue(Ptr<const Packet> packet, Ipv4Header header, const Mac48Address &from, UnicastForwardCallback ucb);
  bool Dequeue(EntryRoutingPacketQueue &entry);
  bool ReadEntry(EntryRoutingPacketQueue &entry);
  //added enqueued/dequeueing/ReadEntry operations
  bool Enqueue(Ptr<const Packet> packet, Ipv4Header header, const Mac48Address &from, UnicastForwardCallback ucb, uint32_t direction);
  bool Enqueue(Ptr<const Packet> packet, Ipv4Header header, const Mac48Address &from, UnicastForwardCallback ucb, uint32_t direction, uint32_t requeue, Time time);
  bool Dequeue(EntryRoutingPacketQueue &entry, uint32_t &direction);
  bool ReadEntry(EntryRoutingPacketQueue &entry, uint32_t &direction);

  void SetMaxSize (uint32_t maxSize);
  void SetMaxDelay (Time delay);
  uint32_t GetMaxSize (void) const;
  uint32_t GetSize(void) const;  
  uint32_t GetNBytes(void) const;  
  uint32_t GetNPackets(void) const;  
  uint32_t GetSizePrev(void) const;
  uint32_t GetVSize(void) const;  
  void SetVSize(uint32_t);  
  void SetSize(uint32_t);  
  void SetInterfaces(uint32_t);
  uint32_t GetNInterfaces(void) const;
  uint32_t GetIfaceSize(uint32_t);
  Time GetMaxDelay (void) const;
  std::vector<uint32_t> GetQueuesSize ();


  void Flush();

  virtual RoutingPacketQueue* Copy (void) const = 0;//virtual copy constructor


private:
  uint32_t m_maxSize;
  uint32_t m_size;
  uint32_t m_interfaces;

  std::vector<uint32_t> m_size_iface;

  uint32_t m_nbytes;
  std::vector<uint32_t> m_nbytes_iface;

  uint32_t m_npackets;
  uint32_t m_sizePrev;
  uint32_t m_Vsize;
  Time m_maxDelay;
  Time m_Prev;
 
  //Original Do...Enqueueing/Dequeueing/ReadEntry functions
  //virtual bool DoFlush()=; 
  virtual bool DoEnqueue(EntryRoutingPacketQueue entry)=0;
  //virtual bool DoEnqueue(Ptr<const Packet> packet, Ipv4Header header, UnicastForwardCallback ucb, Time now);
  virtual bool DoDequeue(EntryRoutingPacketQueue &entry)=0;
  virtual bool DoReadEntry(EntryRoutingPacketQueue &entry)=0;
  //Added Do...enqueued/dequeueing/ReadEntry operations
  virtual bool DoEnqueue(EntryRoutingPacketQueue entry, uint32_t direction)=0;
  virtual bool DoEnqueueFirst(EntryRoutingPacketQueue entry, uint32_t direction)=0;
  virtual bool DoDequeue(EntryRoutingPacketQueue &entry, uint32_t &direction)=0;
  virtual bool DoDequeueLast(EntryRoutingPacketQueue &entry, uint32_t &direction)=0;
  virtual bool DoReadEntry(EntryRoutingPacketQueue &entry, uint32_t &direction)=0;

};

} //namespace ns3

#endif
