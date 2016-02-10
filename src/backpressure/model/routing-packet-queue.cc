#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "routing-packet-queue.h"
#define MAX_SIZE 1440
using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (RoutingPacketQueue);

TypeId 
RoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RoutingPacketQueue")
    .SetParent<Object> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (200),
                   MakeUintegerAccessor (&RoutingPacketQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (Seconds (500.0)),
                   MakeTimeAccessor (&RoutingPacketQueue::m_maxDelay),
                   MakeTimeChecker ())
    ;
  return tid;
}

RoutingPacketQueue::RoutingPacketQueue ()
   : m_size (0)
{
  m_maxDelay = Seconds(5.0);
  m_maxSize=200;
  m_size=0;
  m_nbytes=0;
  m_npackets=0;
  m_sizePrev=0;
  m_Prev=Simulator::Now();
}

RoutingPacketQueue::~RoutingPacketQueue ()
{
  Flush();
}

/*
void
RoutingPacketQueue::Cleanup (void)
{
  if (m_packetQueue.empty ()) 
    {
      return;
    }

  Time now = Simulator::Now ();
  uint32_t n = 0;
  for (PacketQueueI i = m_packetQueue.begin (); i != m_packetQueue.end ();) 
    {
      if ( ( i->GetTstamp() + m_maxDelay ) > now )
        {
          i++;
        }
      else
        {
          i = m_packetQueue.erase (i);
          n++;
        }
    }
  m_size -= n;
}
*/

/////////////////////Original Enqueueing/Dequeueing/ReadEntryFunctions/////////////////////////////

bool
RoutingPacketQueue::Enqueue(Ptr<const Packet> packet, Ipv4Header header, const Mac48Address &from, UnicastForwardCallback ucb)
{
  if (m_size > m_maxSize)
    {
      NS_LOG_DEBUG("[Enqueue]: queue overflow; size of the routing queue");
      if (m_maxSize>0)
        {
          return false;
        }
    }
  if ((Simulator::Now().GetSeconds()-m_Prev.GetSeconds())>0.1)
    {
      m_sizePrev = m_size;
      m_Prev = Simulator::Now();
    }
  Time now = Simulator::Now ();
  //trace Enqueue
  EntryRoutingPacketQueue entry = EntryRoutingPacketQueue (packet, header, from, ucb, now);
  bool ret=DoEnqueue(entry);
  if (ret)
    {
      m_npackets++;
      m_nbytes += packet->GetSize();
      //m_size = m_nbytes/MAX_SIZE;
      m_size++;
      //by default, the original functions counts only with one interface
      m_size_iface[0]= m_size;
    }
  return ret;
}   

/*
bool 
RoutingPacketQueue::DoEnqueue(Ptr<const Packet> packet, Ipv4Header header, UnicastForwardCallback ucb, Time now)
{
  return true;
}
*/
bool 
RoutingPacketQueue::DoEnqueue(EntryRoutingPacketQueue entry)
{
  return true;
}


bool
RoutingPacketQueue::Dequeue (EntryRoutingPacketQueue &entry)
{
  //Cleanup ();
  //loop with a constant to extract N packets
    bool ret = DoDequeue(entry);
    if (ret)
      {
        m_npackets--;
        m_nbytes -= entry.GetPacket()->GetSize();
        //m_size = m_nbytes/MAX_SIZE;
        m_size--;
	//by default, the original functions counts only with one interface
	m_size_iface[0]= m_size;
        NS_LOG_DEBUG("Dequeued a data packet");
	return true;
      }
    else
      {
	return false;
      }
}

bool
RoutingPacketQueue::ReadEntry(EntryRoutingPacketQueue &entry)
{
    bool ret = DoReadEntry(entry);
    return ret;
/*    if (m_packetQueue.empty ()) 
      {
        NS_LOG_DEBUG("[ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
    entry = m_packetQueue.front();
    return true;*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/////Added Enqueueing/Dequeueing/ReadEntry operations////////////////////////////////////////////

bool
RoutingPacketQueue::Enqueue(Ptr<const Packet> packet, Ipv4Header header, const Mac48Address &from, UnicastForwardCallback ucb, uint32_t direction)
{
  if (m_size_iface[direction] > m_maxSize)
    {
      NS_LOG_DEBUG("[Enqueue]: queue overflow; size of the routing queue");
      if (m_maxSize>0)
        {
          return false;
        }
    }
  if ((Simulator::Now().GetSeconds()-m_Prev.GetSeconds())>0.1)
    {
      m_sizePrev = m_size;
      m_Prev = Simulator::Now();
    }
  Time now = Simulator::Now ();
  //trace Enqueue
  EntryRoutingPacketQueue entry = EntryRoutingPacketQueue (packet, header, from, ucb, now);
  bool ret = DoEnqueue(entry, direction);
  if (ret)
    {
      m_npackets++;
      m_nbytes += packet->GetSize();
      m_size = m_nbytes/MAX_SIZE;        
      m_nbytes_iface[direction]+=packet->GetSize();
      m_size_iface[direction]= m_nbytes_iface[direction]/MAX_SIZE;  
    }
  return ret;
}   

//new enqueue function
bool
RoutingPacketQueue::Enqueue(Ptr<const Packet> packet, Ipv4Header header, const Mac48Address &from, UnicastForwardCallback ucb, uint32_t direction, uint32_t requeue, Time time)
{
  //std::cout<<"Encolando la hwaddr: "<<from<<std::endl;
  if (m_size_iface[direction] >= m_maxSize)
    {
      NS_LOG_DEBUG("[Enqueue]: queue overflow; size of the routing queue");
      if (m_maxSize>0)
        {
	  if (requeue == 1) //this one enters in the queue but the last is discarded
	  {
	    EntryRoutingPacketQueue entry;
	    bool ret;
	    while (m_size_iface[direction] >= m_maxSize)
	      {//Be careful: if there are packets of different size
		ret = DoDequeueLast(entry,direction);
		if (ret)
		  {
		    m_npackets--;
                    m_nbytes -= entry.GetPacket()->GetSize();
                    m_size = m_nbytes/MAX_SIZE;
                    m_nbytes_iface[direction]-= entry.GetPacket()->GetSize();
                    m_size_iface[direction]= m_nbytes_iface[direction]/MAX_SIZE;
                    NS_LOG_DEBUG("Dequeued a data packet"); 
		  }
	      }
	    //Time instant = Simulator::Now ();56486
	    entry = EntryRoutingPacketQueue(packet, header, from, ucb, time);
	    ret = DoEnqueueFirst(entry,direction);  
	    if (ret)
             {
               m_npackets++;
               m_nbytes += packet->GetSize();
               m_size = m_nbytes/MAX_SIZE;
               m_nbytes_iface[direction]+=packet->GetSize();
               m_size_iface[direction]= m_nbytes_iface[direction]/MAX_SIZE;
              }
	  }
	  return false;          
        }
    }
  if ((Simulator::Now().GetSeconds()-m_Prev.GetSeconds())>0.1)
    {
      m_sizePrev = m_size;
      m_Prev = Simulator::Now();
    }
    
  Time now = Simulator::Now ();
  //trace Enqueue
  EntryRoutingPacketQueue entry;
  bool ret;
  if (requeue==1)
    {
      entry= EntryRoutingPacketQueue(packet, header, from, ucb, time);
      entry.requeued = requeue;
      ret= DoEnqueueFirst(entry, direction); 
    }
  else
    {
      entry= EntryRoutingPacketQueue(packet, header, from, ucb, now);
      entry.requeued = requeue;
      ret= DoEnqueue(entry, direction);
      //std::cout<<"Lo meto en colaaaa: "<<from<<std::endl;
    }
    //ret=DoEnqueue(entry, direction);
  if (ret)
    {
      m_npackets++;
      m_nbytes += packet->GetSize();
      m_size = m_nbytes/MAX_SIZE;
      //m_size++;
      m_nbytes_iface[direction]+=packet->GetSize();
      m_size_iface[direction]= m_nbytes_iface[direction]/MAX_SIZE;
    }
  return ret;
}   

bool
RoutingPacketQueue::Dequeue (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
  //Cleanup ();
  //loop with a constant to extract N packets
    bool ret = DoDequeue(entry, direction);
    if (ret)
      {
        m_npackets--;
        m_nbytes -= entry.GetPacket()->GetSize();
        m_size = m_nbytes/MAX_SIZE;
	m_nbytes_iface[direction]-= entry.GetPacket()->GetSize();
        m_size_iface[direction]= m_nbytes_iface[direction]/MAX_SIZE;
	NS_LOG_DEBUG("Dequeued a data packet");
	return true;
      }
    else
      {
	return false;
      }
}

bool
RoutingPacketQueue::ReadEntry(EntryRoutingPacketQueue &entry, uint32_t &direction)
{
    bool ret = DoReadEntry(entry, direction);
    return ret;
}


///////////////////////////////////////////////////////////////////////////////////////////////


void 
RoutingPacketQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}

void 
RoutingPacketQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

uint32_t
RoutingPacketQueue::GetMaxSize (void) const
{
  return m_maxSize;
}


uint32_t 
RoutingPacketQueue::GetNBytes(void) const
{
  return m_nbytes;
}

uint32_t 
RoutingPacketQueue::GetNPackets(void) const
{
  return m_npackets;
}
 

void RoutingPacketQueue::SetVSize (uint32_t size)
{
  m_Vsize = size;
}

void RoutingPacketQueue::SetSize (uint32_t size)
{
  m_size = size;
}

uint32_t RoutingPacketQueue::GetVSize (void) const
{
  return m_Vsize;
}
/*
*Return the data queue length bounded by the theoretical maximum size
* We allow to fulfill more than m_maxSize for fair latency comparisons
*/
uint32_t RoutingPacketQueue::GetSize (void) const
{
  //return min(m_size,m_maxSize);
  return m_size;
}

/*
uint32_t RoutingPacketQueue::GetRealSize (void) const
{
  return m_size;
}
*/
uint32_t RoutingPacketQueue::GetSizePrev (void) const
{
  return m_sizePrev;
}

Time
RoutingPacketQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

uint32_t
RoutingPacketQueue::GetIfaceSize(uint32_t index) 
{
  return m_size_iface[index];
}

uint32_t
RoutingPacketQueue::GetNInterfaces(void) const
{
  return m_interfaces;
}

void
RoutingPacketQueue::SetInterfaces(uint32_t interfaces)
{
  m_interfaces= interfaces;
  uint32_t element=0;
  for (uint32_t i=0; i< m_interfaces; i++)
    {
      m_size_iface.push_back(element);
      m_nbytes_iface.push_back(element);
    }
}

std::vector<uint32_t> 
RoutingPacketQueue::GetQueuesSize ()
{
  return m_size_iface;
}

/*
bool 
RoutingPacketQueue::DoFlush()
{
  return true;
}
*/
void
RoutingPacketQueue::Flush (void)
{
  m_size=0;
  m_nbytes = 0;
  for (uint32_t i=0; i< m_interfaces; i++)
    {
      m_size_iface[i]=0;
      m_nbytes_iface[i] =0;
    }
    //m_size_iface.erase(m_size_iface.begin(), m_size_iface.end());
    //m_nbytes_iface.erase(m_nbytes_iface.begin(), m_nbytes_iface.end());
/*
  bool ret = DoFlush();
  if (ret)
    {
      m_size = 0;
    }*/
}

}//namespace ns3
