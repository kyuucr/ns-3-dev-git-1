//#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/ring-routing-packet-queue.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingRoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (RingRoutingPacketQueue);

TypeId 
RingRoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingRoutingPacketQueue")
    .SetParent<RoutingPacketQueue> ()
    .AddConstructor<RingRoutingPacketQueue> ()
    ;
  return tid;
}

RingRoutingPacketQueue::RingRoutingPacketQueue ()
   : RoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
}

RingRoutingPacketQueue::~RingRoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
  DoFlush();
}

RingRoutingPacketQueue* 
RingRoutingPacketQueue::Copy (void) const //virtual copy constructor
{
  return new RingRoutingPacketQueue(*this);
}

//Original functions: not effective, always extracting data from the first queue
bool 
RingRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry)
{
  m_packetQueue_if1.push_back (entry);
  return true;
}

bool
RingRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry)
{
  ListPacketQueue *m_packetQueue;
  m_packetQueue = &m_packetQueue_if1;
  if (m_packetQueue->empty ()) 
    {
      NS_LOG_DEBUG("[RING::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[RING::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue->front ();
  m_packetQueue->pop_front ();
  NS_LOG_DEBUG("[RING::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
RingRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry)
{
  ListPacketQueue *m_packetQueue;
  m_packetQueue = &m_packetQueue_if1; 
  if (m_packetQueue->empty ()) 
    {
      NS_LOG_DEBUG("[RING::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[RING::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue->front ();
  NS_LOG_DEBUG("[RING::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}


//Added functions : the ones used in the code
bool 
RingRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry, uint32_t input_iface)
{
  //std::cout<<"El time stamp del paquete a meter es: "<<entry.m_tstamp<<std::endl;
  switch (input_iface)
  {
    case 1:
      m_packetQueue_if1.push_back (entry);
      break;
    case 2:
      m_packetQueue_if2.push_back (entry);
      break;
  }
  return true;
}

bool
RingRoutingPacketQueue::DoEnqueueFirst(EntryRoutingPacketQueue entry, uint32_t output_iface)
{
   switch (output_iface)
    {
      case 1:
        m_packetQueue_if1.push_front(entry);
        break;
      case 2:
        m_packetQueue_if2.push_front(entry);
        break;
     }
  return true;
}


bool
RingRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry, uint32_t &output_iface)
{
  ListPacketQueue *m_packetQueue=&m_packetQueue_if1;
  switch (output_iface)
    {
      case 1:
        m_packetQueue = &m_packetQueue_if1;
        break;
      case 2:
        m_packetQueue = &m_packetQueue_if2;
        break;
    }
  if (m_packetQueue->empty ()) 
    {
      NS_LOG_DEBUG("[RING::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[RING::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue->front ();
  m_packetQueue->pop_front ();
  //direction = m_queue_directions.front();
  //m_queue_directions.erase(m_queue_directions.begin());
  NS_LOG_DEBUG("[RING::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool 
RingRoutingPacketQueue::DoDequeueLast (EntryRoutingPacketQueue &entry, uint32_t &output_iface)
{
  ListPacketQueue *m_packetQueue=&m_packetQueue_if1;
  switch (output_iface)
    {
      case 1:
	m_packetQueue = &m_packetQueue_if1;
	break;
      case 2:
	m_packetQueue = &m_packetQueue_if2;
        break;
    }
  if (m_packetQueue->empty ()) 
    {
      NS_LOG_DEBUG("[RING::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[RING::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue->back ();
  m_packetQueue->pop_back ();
  NS_LOG_DEBUG("[RING::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
RingRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry, uint32_t &output_iface)
{
  ListPacketQueue *m_packetQueue=&m_packetQueue_if1;
  //std::cout<<"fistro2v. El valor de output_iface:"<<output_iface<<std::endl;
  switch (output_iface)
  {
    case 1:
      //std::cout<<"fistro2"<<std::endl;
      m_packetQueue = &m_packetQueue_if1;
      //std::cout<<"La direccion de m_packetQueue = "<<m_packetQueue<<" y el &m_packetQueue_if1 es igual a: "<<&m_packetQueue_if1<<std::endl;
      break;
    case 2:
      //std::cout<<"fistro3"<<std::endl;
      m_packetQueue = &m_packetQueue_if2;
      //std::cout<<"La direccion de m_packetQueue = "<<m_packetQueue<<" y el &m_packetQueue_if2 es igual a: "<<&m_packetQueue_if2<<std::endl;
      break;
  }
  if (m_packetQueue->empty ()) 
    {
      NS_LOG_DEBUG("[RING::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[RING::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue->front ();
  //direction = m_queue_directions.front();
  NS_LOG_DEBUG("[RING::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
RingRoutingPacketQueue::DoFlush (void)
{
  m_packetQueue_if1.erase (m_packetQueue_if1.begin (), m_packetQueue_if1.end ());
  m_packetQueue_if2.erase (m_packetQueue_if2.begin (), m_packetQueue_if2.end ());
  //m_queue_directions.erase (m_queue_directions.begin(), m_queue_directions.end());
  return true;
}

}//namespace ns3
