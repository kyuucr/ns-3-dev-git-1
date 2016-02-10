#include <ostream>

//#include "ns3/log.h"
//#include "ns3/address.h"
//#include "ns3/node.h"
#include "ns3/nstime.h"
//#include "ns3/data-rate.h"
//#include "ns3/random-variable.h"
//#include "ns3/socket.h"
#include "ns3/simulator.h"
//#include "ns3/socket-factory.h"
//#include "ns3/packet.h"
#include "ns3/uinteger.h"
//#include "ns3/trace-source-accessor.h"
//#include "ns3/udp-socket-factory.h"

#include "seqtag.h"

using namespace std;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SequenceTag);

//----------------------------------------------------------------------
//-- SequenceTag
//------------------------------------------------------

TypeId 
SequenceTag::GetTypeId(void)
{
  static TypeId tid = TypeId ("SequenceTag")
    .SetParent<Tag> ()
    .AddConstructor<SequenceTag> ()
    .AddAttribute ("Sequence",
                   "Packet Sequence Number",
                   EmptyAttributeValue(),
                   MakeUintegerAccessor(&SequenceTag::m_sequence),
                   MakeUintegerChecker<uint32_t>(1))
    ;
  return tid;
}
TypeId 
SequenceTag::GetInstanceTypeId(void) const
{
  return GetTypeId ();
}

uint32_t 
SequenceTag::GetSerializedSize (void) const
{
  return 8;
}
void 
SequenceTag::Serialize (TagBuffer i) const
{
  uint32_t t = m_sequence;
  i.Write((const uint8_t *)&t, 8);
}
void 
SequenceTag::Deserialize (TagBuffer i)
{
  uint32_t t;
  i.Read((uint8_t *)&t, 8);
  m_sequence = t;
}

void
SequenceTag::SetSequence(uint32_t sequence)
{
  m_sequence = sequence;
}

uint32_t SequenceTag::GetSequence(void) const
{
  return m_sequence;
}

void 
SequenceTag::Print(std::ostream &os) const
{
  os << "t=" << m_sequence;
}

//----------------------------------------------------------------------
//-- TimestampTag
//------------------------------------------------------

TypeId 
TimestampTag::GetTypeId(void)
{
  static TypeId tid = TypeId ("TimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<TimestampTag> ()
    .AddAttribute ("Timestamp",
                   "Some momentous point in time!",
                   EmptyAttributeValue(),
                   MakeTimeAccessor(&TimestampTag::GetTimestamp),
                   MakeTimeChecker())
    ;
  return tid;
}
TypeId 
TimestampTag::GetInstanceTypeId(void) const
{
  return GetTypeId ();
}

uint32_t 
TimestampTag::GetSerializedSize (void) const
{
  return 8;
}
void 
TimestampTag::Serialize (TagBuffer i) const
{
  int64_t t = m_timestamp.GetNanoSeconds();
  i.Write((const uint8_t *)&t, 8);
}
void 
TimestampTag::Deserialize (TagBuffer i)
{
  int64_t t;
  i.Read((uint8_t *)&t, 8);
  m_timestamp = NanoSeconds(t);
}

void
TimestampTag::SetTimestamp(Time time)
{
  m_timestamp = time;
}
Time
TimestampTag::GetTimestamp(void) const
{
  return m_timestamp;
}

void 
TimestampTag::Print(std::ostream &os) const
{
  os << "t=" << m_timestamp;
}

} //Namespace ns3
