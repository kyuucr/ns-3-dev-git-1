#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/lifo-routing-packet-queue.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LifoRoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (LifoRoutingPacketQueue);

TypeId 
LifoRoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LifoRoutingPacketQueue")
    .SetParent<RoutingPacketQueue> ()
    .AddConstructor<LifoRoutingPacketQueue> ()
    ;
  return tid;
}

LifoRoutingPacketQueue::LifoRoutingPacketQueue ()
   : RoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
}

LifoRoutingPacketQueue::~LifoRoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
  DoFlush();
}

LifoRoutingPacketQueue* 
LifoRoutingPacketQueue::Copy (void) const //virtual copy constructor
{
  return new LifoRoutingPacketQueue(*this);
}
//Original functions
bool 
LifoRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry)
{
  //Cleanup ();might be inefficient
  /*
  if (m_size > m_maxSize)
    {
      NS_LOG_DEBUG("[LIFO::Enqueue]: queue overflow; size of the routing queue");
      return false;
    }
  */
  //NS_LOG_DEBUG("[LIFO::Enqueue]: the destination of the packet to enqueue is "<<header.GetDestination());
  m_packetQueue.push_front (entry);
  //NS_LOG_DEBUG("[LIFO::Enqueue]: size of the queue is ");
  return true;
}

bool
LifoRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry)
{
    //Cleanup ();might be inefficient
    //loop with a constant to extract N packets
    if (m_packetQueue.empty ()) 
      {
        NS_LOG_DEBUG("[LIFO::Dequeue]: can't dequeue packet from an empty queue");
        return false; 
      }
    NS_LOG_DEBUG("[LIFO::Dequeue]: dequeueing from a NON empty queue");
    entry = m_packetQueue.front ();
    m_packetQueue.pop_front ();
    NS_LOG_DEBUG("[LIFO::Dequeue]: packet dequeued, decrementing queue size"); 
    return true;
}

bool
LifoRoutingPacketQueue::DoReadEntry(EntryRoutingPacketQueue &entry)
{
    if (m_packetQueue.empty ()) 
      {
        NS_LOG_DEBUG("[LIFO::ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
    entry = m_packetQueue.front();
    return true;
}

///////////////////////////Added Functions
bool 
LifoRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry, uint32_t direction)
{
  //Cleanup ();might be inefficient
  /*
  if (m_size > m_maxSize)
    {
      NS_LOG_DEBUG("[LIFO::Enqueue]: queue overflow; size of the routing queue");
      return false;
    }
  */
  //NS_LOG_DEBUG("[LIFO::Enqueue]: the destination of the packet to enqueue is "<<header.GetDestination());
  m_packetQueue.push_front (entry);
  std::vector<uint32_t>::iterator it;
  it = m_queue_directions.begin();
  it = m_queue_directions.insert ( it , direction );
  //NS_LOG_DEBUG("[LIFO::Enqueue]: size of the queue is ");
  return true;
}

bool
LifoRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
    //Cleanup ();might be inefficient
    //loop with a constant to extract N packets
    if (m_packetQueue.empty ()) 
      {
        NS_LOG_DEBUG("[LIFO::Dequeue]: can't dequeue packet from an empty queue");
        return false; 
      }
    NS_LOG_DEBUG("[LIFO::Dequeue]: dequeueing from a NON empty queue");
    entry = m_packetQueue.front ();
    direction = m_queue_directions.front();
    m_packetQueue.pop_front ();
    m_queue_directions.erase(m_queue_directions.begin());
    NS_LOG_DEBUG("[LIFO::Dequeue]: packet dequeued, decrementing queue size"); 
    return true;
}

bool
LifoRoutingPacketQueue::DoReadEntry(EntryRoutingPacketQueue &entry, uint32_t &direction)
{
    if (m_packetQueue.empty ()) 
      {
        NS_LOG_DEBUG("[LIFO::ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
      NS_LOG_DEBUG("[LIFO::DoDequeue]: dequeueing from a NON empty queue");
      entry = m_packetQueue.front();
      direction = m_queue_directions.front();
      NS_LOG_DEBUG("[LIFO::DoDequeue]: packet dequeued");
    return true;
}

bool 
LifoRoutingPacketQueue::DoEnqueueFirst(EntryRoutingPacketQueue e, uint32_t output_iface)
{
  return true;
}

bool 
LifoRoutingPacketQueue::DoDequeueLast(EntryRoutingPacketQueue &e, uint32_t &output_iface)
{
  return true;
}
///////////////////////////////////////////////////

bool
LifoRoutingPacketQueue::DoFlush()
{
  m_packetQueue.erase (m_packetQueue.begin (), m_packetQueue.end ());
  m_queue_directions.erase (m_queue_directions.begin(), m_queue_directions.end());
  return true;
}

}//namespace ns3
