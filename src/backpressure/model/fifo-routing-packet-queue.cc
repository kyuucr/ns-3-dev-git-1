//#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/fifo-routing-packet-queue.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FifoRoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (FifoRoutingPacketQueue);

TypeId 
FifoRoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FifoRoutingPacketQueue")
    .SetParent<RoutingPacketQueue> ()
    .AddConstructor<FifoRoutingPacketQueue> ()
    ;
  return tid;
}

FifoRoutingPacketQueue::FifoRoutingPacketQueue ()
   : RoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
}

FifoRoutingPacketQueue::~FifoRoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
  DoFlush();
}

FifoRoutingPacketQueue* 
FifoRoutingPacketQueue::Copy (void) const //virtual copy constructor
{
  return new FifoRoutingPacketQueue(*this);
}

//Original functions
bool 
FifoRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry)
{
  m_packetQueue.push_back (entry);
  return true;
}

bool
FifoRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry)
{
  if (m_packetQueue.empty ()) 
    {
      NS_LOG_DEBUG("[FIFO::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[FIFO::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue.front ();
  m_packetQueue.pop_front ();
  NS_LOG_DEBUG("[FIFO::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
FifoRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry)
{
  if (m_packetQueue.empty ()) 
    {
      NS_LOG_DEBUG("[FIFO::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[FIFO::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue.front ();
  NS_LOG_DEBUG("[FIFO::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

//Added Functions 
bool 
FifoRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry, uint32_t direction)
{
  m_packetQueue.push_back (entry);
  m_queue_directions.push_back (direction);
  return true;
}

bool
FifoRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
  if (m_packetQueue.empty ()) 
    {
      NS_LOG_DEBUG("[FIFO::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[FIFO::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue.front ();
  direction = m_queue_directions.front();
  m_packetQueue.pop_front ();
  m_queue_directions.erase(m_queue_directions.begin());
  NS_LOG_DEBUG("[FIFO::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
FifoRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
  if (m_packetQueue.empty ()) 
    {
      NS_LOG_DEBUG("[FIFO::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[FIFO::DoDequeue]: dequeueing from a NON empty queue");
  entry = m_packetQueue.front ();
  direction = m_queue_directions.front();
  NS_LOG_DEBUG("[FIFO::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool 
FifoRoutingPacketQueue::DoEnqueueFirst(EntryRoutingPacketQueue e, uint32_t output_iface)
{
  return true;
}

bool 
FifoRoutingPacketQueue::DoDequeueLast(EntryRoutingPacketQueue &e, uint32_t &output_iface)
{
  return true;
}


bool
FifoRoutingPacketQueue::DoFlush (void)
{
  m_packetQueue.erase (m_packetQueue.begin (), m_packetQueue.end ());
  m_queue_directions.erase (m_queue_directions.begin(), m_queue_directions.end());
  return true;
}

}//namespace ns3
