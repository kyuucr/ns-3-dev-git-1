#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/edf-routing-packet-queue.h"
#include <algorithm>

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EdfRoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (EdfRoutingPacketQueue);

TypeId 
EdfRoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdfRoutingPacketQueue")
    .SetParent<Object> ()
    .AddConstructor<EdfRoutingPacketQueue> ()
    ;
  return tid;
}

EdfRoutingPacketQueue::EdfRoutingPacketQueue ()
   : RoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
}

EdfRoutingPacketQueue::~EdfRoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
  DoFlush();
}

EdfRoutingPacketQueue* 
EdfRoutingPacketQueue::Copy (void) const //virtual copy constructor
{
  return new EdfRoutingPacketQueue(*this);
}

bool
EdfRoutingPacketQueue::ComparePackets(EntryRoutingPacketQueue e1, EntryRoutingPacketQueue e2)
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
EdfRoutingPacketQueue::DoEnqueue(EntryRoutingPacketQueue entry)
{

  //TODO: a) Check  whether some data packet should be flushed or not by TTL
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease

  ListEdfPacketQueueI i = std::upper_bound ( m_edfPacketQueue.begin (), m_edfPacketQueue.end (), 
					    entry, &EdfRoutingPacketQueue::ComparePackets);
  m_edfPacketQueue.insert (i, entry);
  return true;
}

bool
EdfRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry)
{
  //TODO: a) Update TTL values and flush it TTL expired 
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease
   if (m_edfPacketQueue.empty ())
     {
       NS_LOG_DEBUG("[PRIORITY::DoDequeue]: can't dequeue packet from an empty queue");
       return false;
     }
   NS_LOG_DEBUG("[PRIORITY::DoDequeue]: dequeueing from a NON empty queue");
   entry = m_edfPacketQueue.front ();
   m_edfPacketQueue.pop_front ();
   return true;
}

bool
EdfRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry)
{
    if (m_edfPacketQueue.empty ()) 
      {
        NS_LOG_DEBUG("[Edf::ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
    entry = m_edfPacketQueue.front ();
    return true;
}

bool
EdfRoutingPacketQueue::DoFlush()
{
  m_edfPacketQueue.erase (m_edfPacketQueue.begin (), m_edfPacketQueue.end ());
  m_queue_directions.erase (m_queue_directions.begin(), m_queue_directions.end());
  return true;
}
//////////////////Added functions
bool 
EdfRoutingPacketQueue::DoEnqueue(EntryRoutingPacketQueue entry, uint32_t direction)
{

  //TODO: a) Check  whether some data packet should be flushed or not by TTL
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease

  ListEdfPacketQueueI i = std::upper_bound ( m_edfPacketQueue.begin (), m_edfPacketQueue.end (), 
					    entry, &EdfRoutingPacketQueue::ComparePackets);
  
  m_edfPacketQueue.insert (i, entry);
  uint32_t n=0;
  uint32_t j=0;
    for(ListEdfPacketQueueI it = m_edfPacketQueue.begin(); it != m_edfPacketQueue.end(); ++it) 
      {
	  if (it == i)
	    j=n;
	  n++;
      }
  m_queue_directions.insert(m_queue_directions.begin()+j, direction);
  return true;
}

bool
EdfRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
  //TODO: a) Update TTL values and flush it TTL expired 
  //      b) EntryRoutingPacketQueue should be modified including these variables: 
		//	--dynamic TTL
		//	--timeout since last TTL decrease
   if (m_edfPacketQueue.empty ())
     {
       NS_LOG_DEBUG("[PRIORITY::DoDequeue]: can't dequeue packet from an empty queue");
       return false;
     }
   NS_LOG_DEBUG("[PRIORITY::DoDequeue]: dequeueing from a NON empty queue");
   entry = m_edfPacketQueue.front ();
   m_edfPacketQueue.pop_front ();
   m_queue_directions.erase(m_queue_directions.begin());
   return true;
}

bool
EdfRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry, uint32_t &direction)
{
    if (m_edfPacketQueue.empty ()) 
      {
        NS_LOG_DEBUG("[Edf::ReadEntry]: dequeueing from an empty queue");
        return false; 
      }
    entry = m_edfPacketQueue.front ();
    direction = m_queue_directions.front(); //like lifo and fifo
    return true;
}

bool 
EdfRoutingPacketQueue::DoEnqueueFirst(EntryRoutingPacketQueue e, uint32_t output_iface)
{
  return true;
}

bool 
EdfRoutingPacketQueue::DoDequeueLast(EntryRoutingPacketQueue &e, uint32_t &output_iface)
{
  return true;
}

}//namespace ns3
