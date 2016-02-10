
#ifndef ROUTING_DATA_QUEUE_H
#define ROUTING_DATA_QUEUE_H


#include <list>
#include <utility>
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/simulator.h"

namespace ns3 {
class EntryDataQueue
{
public:
  typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
  EntryDataQueue(Ptr<const Packet> p = 0, Ipv4Header const header = Ipv4Header (), 
		UnicastForwardCallback ucb = UnicastForwardCallback (), Time tstamp = Simulator::Now ()) :
                m_packet(p), m_header(header), m_ucb(ucb), m_tstamp(tstamp)
  {}
  Ptr<const Packet> GetPacket(){return m_packet;}
  Ipv4Header GetHeader(){return m_header;} 
  UnicastForwardCallback GetUcb(){return m_ucb;}
  Time GetTstamp(){return m_tstamp;}
  void SetPacket(Ptr<Packet> p){m_packet=p;}
  void SetHeader(Ipv4Header header){m_header=header;}
  void SetUcb(UnicastForwardCallback ucb){m_ucb=ucb;}
  void SetTstamp(Time tstamp){m_tstamp=tstamp;}

  Ptr<const Packet> m_packet;
  Ipv4Header  m_header;
  UnicastForwardCallback m_ucb;
  Time m_tstamp;
};

//superclass DataQueue implements FIFO policy by default
class DataQueue: public Object
{
public: 
  static TypeId GetTypeId (void);
  DataQueue ();
  ~DataQueue ();
  
  typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
  virtual bool Enqueue(Ptr<const Packet> packet, Ipv4Header header, UnicastForwardCallback ucb);
  virtual bool Dequeue(EntryDataQueue &entry);
  bool ReadEntry(EntryDataQueue &entry);

  void SetMaxSize (uint32_t maxSize);
  void SetMaxDelay (Time delay);
  uint32_t GetMaxSize (void) const;
  uint32_t GetSize(void) const;  
  uint32_t GetMaxSize(void) const;  
  uint32_t GetSizePrev(void) const;
  uint32_t GetVSize(void) const;  
  void SetVSize(uint32_t);  
  void SetSize(uint32_t);  
  Time GetMaxDelay (void) const;

  void Flush();

protected:
  uint32_t m_maxSize;
  uint32_t m_size;
  uint32_t m_sizePrev;
  uint32_t m_Vsize;
  Time m_maxDelay;
  Time m_Prev;

  typedef std::list<EntryDataQueue> PacketQueue;
  typedef std::list<EntryDataQueue>::iterator PacketQueueI;

  PacketQueue m_packetQueue;

  void Cleanup();
  
};

} //namespace ns3

#endif
