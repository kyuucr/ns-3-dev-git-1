#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "routing-data-queue.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingDataQueue");

NS_OBJECT_ENSURE_REGISTERED (DataQueue);

TypeId 
DataQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DataQueue")
    .SetParent<Object> ()
    .AddConstructor<DataQueue> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (200),
                   MakeUintegerAccessor (&DataQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (Seconds (5.0)),
                   MakeTimeAccessor (&DataQueue::m_maxDelay),
                   MakeTimeChecker ())
    ;
  return tid;
}

DataQueue::DataQueue ()
   : m_size (0)
{
  m_maxDelay = Seconds(5.0);
  m_size=0;
  m_sizePrev=0;
  m_Prev=Simulator::Now();
  //m_maxSize=1000;
}

DataQueue::~DataQueue ()
{
  Flush();
}

bool 
DataQueue::Enqueue (Ptr<const Packet> packet, Ipv4Header header, UnicastForwardCallback ucb)
{
  //Cleanup ();
  /*if (header.GetTtl()==64)
    {
      //NS_LOG_UNCOND("Packet in the transport queue");
      Time now = Simulator::Now ();
     //NS_LOG_UNCOND("the destination of the packet to enqueue is "<<header.GetDestination());
      m_packetQueue.push_back (EntryDataQueue (packet, header, ucb, now));
      m_size++;
      return true;
    }*/
  if ((Simulator::Now().GetSeconds()-m_Prev.GetSeconds())>0.1)
    {
      m_sizePrev = m_size;
      m_Prev = Simulator::Now();
    }
  if (m_size >= m_maxSize)
    {
    //NS_LOG_UNCOND("[Enqueue]: queue overflow "<<(int)m_size);
      return false;
    }
  Time now = Simulator::Now ();
  //NS_LOG_UNCOND("the destination of the packet to enqueue is "<<header.GetDestination());
  m_packetQueue.push_back (EntryDataQueue (packet, header, ucb, now));
  m_size++;
  return true;
  //NS_LOG_UNCOND("[Enqueue]: size of the queue is "<<(int)m_size);
}

void
DataQueue::Cleanup (void)
{
  if (m_packetQueue.empty ()) 
    {
      return;
    }

  Time now = Simulator::Now ();
  uint32_t n = 0;
  for (PacketQueueI i = m_packetQueue.begin (); i != m_packetQueue.end ();) 
    {
      /*
      NS_LOG_UNCOND("timestamp a la cua es " << i->tstamp);
      NS_LOG_UNCOND("max Delay es " << m_maxDelay);
      NS_LOG_UNCOND("time now is " << now);
      */
      if ( ( i->GetTstamp() + m_maxDelay ) > now )
        {
          i++;
        }
      else
        {
          //NS_LOG_UNCOND("[Cleanup]: delete elements of the queue");
          i = m_packetQueue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

bool
DataQueue::Dequeue (EntryDataQueue &entry)
{
  Cleanup ();
//loop with a constant to extract N packets
    if (m_packetQueue.empty ()) 
    {
     //NS_LOG_UNCOND("[Dequeue]: dequeueing from an empty queue");
     return false; 
    }
    //NS_LOG_UNCOND("[Dequeue]: dequeueing from a NON empty queue");
    entry = m_packetQueue.front ();
    m_packetQueue.pop_front ();
    m_size--;
    return true;
    //NS_LOG_UNCOND("[Dequeue]: decrementing queue size the queue length is "<<(int) m_size); 
}

bool
DataQueue::ReadEntry(EntryDataQueue &entry)
{
    if (m_packetQueue.empty ()) 
    {
     //NS_LOG_UNCOND("[Dequeue]: dequeueing from an empty queue");
     return false; 
    }
    entry = m_packetQueue.front();
    return true;
}

void DataQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}

void DataQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

uint32_t DataQueue::GetMaxSize (void) const
{
  return m_maxSize;
}

void DataQueue::SetVSize (uint32_t size)
{
  m_Vsize = size;
}

void DataQueue::SetSize (uint32_t size)
{
  m_size = size;
}

uint32_t DataQueue::GetVSize (void) const
{
  return m_Vsize;
}
/*
* Return the data queue length bounded by the Maximum Size. In saturation conditions we allow
* the fulfill the queue more than MaxSize packet for latency issues
*/
uint32_t DataQueue::GetSize (void) const
{
  return m_size;
}
uint32_t DataQueue::GetSizePrev (void) const
{
  return m_sizePrev;
}


Time
DataQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

void
DataQueue::Flush (void)
{
  m_packetQueue.erase (m_packetQueue.begin (), m_packetQueue.end ());
  m_size = 0;
}

}//namespace ns3
