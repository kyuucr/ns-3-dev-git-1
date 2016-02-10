//#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/grid-routing-packet-queue.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GridRoutingPacketQueue");

NS_OBJECT_ENSURE_REGISTERED (GridRoutingPacketQueue);

TypeId 
GridRoutingPacketQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GridRoutingPacketQueue")
    .SetParent<RoutingPacketQueue> ()
    .AddConstructor<GridRoutingPacketQueue> ()
    ;
  return tid;
}

GridRoutingPacketQueue::GridRoutingPacketQueue ()
   : RoutingPacketQueue ()
{
  //m_interfaces= 4; //in a grid this is the maximum number of interfaces
  m_interfaces = 10; //the problem is this hardcoded parameter!! TODO: Change
  NS_LOG_FUNCTION_NOARGS();
  ListPacketQueue eth_queue;
  for (uint32_t i=0; i<m_interfaces; i++)
    {
      m_packetQueue.push_back(eth_queue);
    }
}

GridRoutingPacketQueue::GridRoutingPacketQueue(uint32_t interfaces)
{
  m_interfaces = interfaces;
  ListPacketQueue eth_queue;
  for (uint32_t i=0; i<m_interfaces; i++)
    {
      m_packetQueue.push_back(eth_queue);
    }
}

GridRoutingPacketQueue::~GridRoutingPacketQueue ()
{
  NS_LOG_FUNCTION_NOARGS();
  DoFlush();
}

GridRoutingPacketQueue* 
GridRoutingPacketQueue::Copy (void) const //virtual copy constructor
{
  return new GridRoutingPacketQueue(*this);
}

//Original functions: not effective, always extracting data from the first queue
bool 
GridRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry)
{
  m_packetQueue[0].push_back(entry);
  return true;
}

bool 
GridRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry)
{
  //ListPacketQueue *m_packetQueue_tmp;
  //m_packetQueue_tmp =&m_packetQueue[0];
  //if (m_packetQueue_tmp->empty ()) 
  if (m_packetQueue[0].empty())
    {
      NS_LOG_DEBUG("[GRID::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[GRID::DoDequeue]: dequeueing from a NON empty queue");
  //entry = m_packetQueue_tmp->front ();
  entry = m_packetQueue[0].front();
  //m_packetQueue_tmp->pop_front ();
  m_packetQueue[0].pop_front ();
  NS_LOG_DEBUG("[GRID::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
GridRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry)
{
  //ListPacketQueue *m_packetQueue_tmp;
  //std::cout<<"fistro2v. El valor de output_iface:"<<output_iface<<std::endl;
  //m_packetQueue_tmp = &m_packetQueue[0];
  //if (m_packetQueue_tmp->empty ()) 
  if (m_packetQueue[0].empty())
    {
      NS_LOG_DEBUG("[GRID::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[GRID::DoDequeue]: dequeueing from a NON empty queue");
  //entry = m_packetQueue_tmp->front ();
  entry = m_packetQueue[0].front();
  //direction = m_queue_directions.front();
  NS_LOG_DEBUG("[GRID::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

//Added functions
bool 
GridRoutingPacketQueue::DoEnqueue (EntryRoutingPacketQueue entry, uint32_t input_iface)
{
  //std::cout<<"El time stamp del paquete a meter es: "<<entry.m_tstamp<<std::endl;
  m_packetQueue[input_iface].push_back(entry);
  return true;
}

bool
GridRoutingPacketQueue::DoEnqueueFirst(EntryRoutingPacketQueue entry, uint32_t output_iface)
{
  
  m_packetQueue[output_iface].push_front(entry);
  return true;
}


bool
GridRoutingPacketQueue::DoDequeue (EntryRoutingPacketQueue &entry, uint32_t &output_iface)
{
  //ListPacketQueue *m_packetQueue_tmp;
  //m_packetQueue_tmp =&m_packetQueue[output_iface];
  //if (m_packetQueue_tmp->empty ()) 
  if (m_packetQueue[output_iface].empty())
    {
      NS_LOG_DEBUG("[GRID::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[GRID::DoDequeue]: dequeueing from a NON empty queue");
  //entry = m_packetQueue_tmp->front ();
  entry = m_packetQueue[output_iface].front ();
  //m_packetQueue_tmp->pop_front ();
  m_packetQueue[output_iface].pop_front ();
  NS_LOG_DEBUG("[GRID::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool 
GridRoutingPacketQueue::DoDequeueLast (EntryRoutingPacketQueue &entry, uint32_t &output_iface)
{
  //ListPacketQueue *m_packetQueue_tmp;
  //m_packetQueue_tmp = &m_packetQueue[output_iface];
  //if (m_packetQueue_tmp->empty ()) 
  if (m_packetQueue[output_iface].empty())
    {
      NS_LOG_DEBUG("[GRID::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[GRID::DoDequeue]: dequeueing from a NON empty queue");
  //entry = m_packetQueue_tmp->back ();
  entry = m_packetQueue[output_iface].back();
  //m_packetQueue_tmp->pop_back ();
  m_packetQueue[output_iface].pop_back ();
  NS_LOG_DEBUG("[GRID::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
GridRoutingPacketQueue::DoReadEntry (EntryRoutingPacketQueue &entry, uint32_t &output_iface)
{
  //ListPacketQueue *m_packetQueue_tmp;
  //std::cout<<"fistro2v. El valor de output_iface:"<<output_iface<<"y tengo "<<m_packetQueue.size()<<" colas e interaces: "<<m_interfaces<<std::endl;
  //m_packetQueue_tmp = &m_packetQueue[output_iface];
  //if (m_packetQueue_tmp->empty ()) 
  if (m_packetQueue[output_iface].empty())
    {
      NS_LOG_DEBUG("[GRID::DoDequeue]: can't dequeue packet from an empty queue");
      return false; 
    }
  NS_LOG_DEBUG("[GRID::DoDequeue]: dequeueing from a NON empty queue");
  //entry = m_packetQueue_tmp->front ();
  entry = m_packetQueue[output_iface].front();
  NS_LOG_DEBUG("[GRID::DoDequeue]: packet dequeued, decrementing queue size"); 
  return true;
}

bool
GridRoutingPacketQueue::DoFlush (void)
{
  for (uint32_t i=0; i<m_interfaces;i++)
  {
    if (!m_packetQueue[i].empty())
      //m_packetQueue[i].erase(m_packetQueue[i].begin(), m_packetQueue[i].end());
      m_packetQueue[i].clear();
  }
  m_packetQueue.erase(m_packetQueue.begin(), m_packetQueue.end());
    
  return true;
}

}//namespace ns3
