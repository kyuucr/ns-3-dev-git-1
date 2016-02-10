#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/priority-routing-packet-queue.h"
#include <algorithm>

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PriorityRoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (PriorityRoutingPacketQueue);

TypeId 
PriorityRoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityRoutingPacketQueue")
    .SetParent<Object> ()
    .AddConstructor<PriorityRoutingPacketQueue> ()
    ;
  return tid;
}

PriorityRoutingPacketQueue::PriorityRoutingPacketQueue ()
   : RoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
}

PriorityRoutingPacketQueue::~PriorityRoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
  DoFlush();
}

PriorityRoutingPacketQueue* 
PriorityRoutingPacketQueue::Copy (void) const //virtual copy constructor
{
  return new PriorityRoutingPacketQueue(*this);
}

bool
PriorityRoutingPacketQueue::ComparePackets(EntryRoutingPacketQueue e1, EntryRoutingPacketQueue e2)
{
  //NS_LOG_FUNCTION (e1 << e2);
  bool result = false;
  /*uint32_t htime1 = e1.GetHopTime();
  uint32_t htime2 = e2.GetHopTime();*/
  if ((e1.GetHeader().GetTtl())<(e2.GetHeader().GetTtl()))
    {
      result = true;
    }
  return result;
}

//Original functions
bool 
PriorityRoutingPacketQueue::DoEnqueue(EntryRoutingPacketQueue entry)
{
//  NS_LOG_UNCOND("[PRIORITY::DoEnqueue]:ENQUEUE TTL is "<<(int)entry.GetHeader().GetTtl());
    // m_priorityPacketQueue.push_back (entry);

  //TODO: a) Check  whether some data packet should be flushed or not by TTL
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease

  ListPriorityPacketQueueI i = std::upper_bound ( m_priorityPacketQueue.begin (), m_priorityPacketQueue.end (), 
					    entry, &PriorityRoutingPacketQueue::ComparePackets);
  m_priorityPacketQueue.insert (i, entry);
  return true;
}

bool
PriorityRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry)
{
  //TODO: a) Update TTL values and flush it TTL expired 
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease
   if (m_priorityPacketQueue.empty ())
     {
       NS_LOG_DEBUG("[PRIORITY::DoDequeue]: can't dequeue packet from an empty queue");
       return false;
     }
   NS_LOG_DEBUG("[PRIORITY::DoDequeue]: dequeueing from a NON empty queue");
   entry = m_priorityPacketQueue.front ();
   m_priorityPacketQueue.pop_front ();
   return true;
}

bool
PriorityRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry)
{
    if (m_priorityPacketQueue.empty ()) 
      {
        NS_LOG_DEBUG("[Priority::ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
    entry = m_priorityPacketQueue.front ();
    return true;
}

bool
PriorityRoutingPacketQueue::DoFlush()
{
  m_priorityPacketQueue.erase (m_priorityPacketQueue.begin (), m_priorityPacketQueue.end ());
  m_queue_directions.erase (m_queue_directions.begin(), m_queue_directions.end());
  return true;
}

//Added functions
bool 
PriorityRoutingPacketQueue::DoEnqueue(EntryRoutingPacketQueue entry, uint32_t direction)
{
//  NS_LOG_UNCOND("[PRIORITY::DoEnqueue]:ENQUEUE TTL is "<<(int)entry.GetHeader().GetTtl());
    // m_priorityPacketQueue.push_back (entry);

  //TODO: a) Check  whether some data packet should be flushed or not by TTL
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease

  ListPriorityPacketQueueI i = std::upper_bound ( m_priorityPacketQueue.begin (), m_priorityPacketQueue.end (), 
					    entry, &PriorityRoutingPacketQueue::ComparePackets);
  m_priorityPacketQueue.insert (i, entry);
  uint32_t n=0;
  uint32_t j=0;
    for(ListPriorityPacketQueueI it = m_priorityPacketQueue.begin(); it != m_priorityPacketQueue.end(); ++it) 
      {
	  if (it == i)
	    j=n;
	  n++;
      }
  m_queue_directions.insert(m_queue_directions.begin()+j, direction);
  return true;
}

bool
PriorityRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
  //TODO: a) Update TTL values and flush it TTL expired 
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease
   if (m_priorityPacketQueue.empty ())
     {
       NS_LOG_DEBUG("[PRIORITY::DoDequeue]: can't dequeue packet from an empty queue");
       return false;
     }
   NS_LOG_DEBUG("[PRIORITY::DoDequeue]: dequeueing from a NON empty queue");
   entry = m_priorityPacketQueue.front ();
   m_priorityPacketQueue.pop_front ();
   m_queue_directions.erase(m_queue_directions.begin());
   return true;
}

bool
PriorityRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
    if (m_priorityPacketQueue.empty ()) 
      {
        NS_LOG_DEBUG("[Priority::ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
    entry = m_priorityPacketQueue.front ();
    direction = m_queue_directions.front(); //like lifo and fifo
    return true;
}

bool 
PriorityRoutingPacketQueue::DoEnqueueFirst(EntryRoutingPacketQueue e, uint32_t output_iface)
{
  return true;
}

bool 
PriorityRoutingPacketQueue::DoDequeueLast(EntryRoutingPacketQueue &e, uint32_t &output_iface)
{
  return true;
}

}//namespace ns3
