/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 CTTC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jose Núñez  <jose.nunez@cttc.cat>
 */

#include "ns3/assert.h"

#include "backpressure-header.h"
#include "ns3/log.h"

#define IPV4_ADDRESS_SIZE 4
#define BACKPRESSURE_MSG_HEADER_SIZE 12 
#define BACKPRESSURE_PKT_HEADER_SIZE 4

namespace ns3 {
namespace backpressure {


NS_LOG_COMPONENT_DEFINE("BackpressureHeader");

/// Scaling factor used in RFC 3626.
#define BACKPRESSURE_C 0.00625

///
/// \brief Converts a decimal number of seconds to the mantissa/exponent format.
///
/// \param seconds decimal number of seconds we want to convert.
/// \return the number of seconds in mantissa/exponent format.
///
uint8_t
SecondsToEmf (double seconds)
{
  int a, b = 0;
  
  // find the largest integer 'b' such that: T/C >= 2^b
  for (b = 0; (seconds/BACKPRESSURE_C) >= (1 << b); ++b)
    ;
  NS_ASSERT ((seconds/BACKPRESSURE_C) < (1 << b));
  b--;
  NS_ASSERT ((seconds/BACKPRESSURE_C) >= (1 << b));

  // compute the expression 16*(T/(C*(2^b))-1), which may not be a integer
  double tmp = 16*(seconds/(BACKPRESSURE_C*(1<<b))-1);

  // round it up.  This results in the value for 'a'
  a = (int) ceil (tmp);

  // if 'a' is equal to 16: increment 'b' by one, and set 'a' to 0
  if (a == 16)
    {
      b += 1;
      a = 0;
    }

  // now, 'a' and 'b' should be integers between 0 and 15,
  NS_ASSERT (a >= 0 && a < 16);
  NS_ASSERT (b >= 0 && b < 16);

  // the field will be a byte holding the value a*16+b
  return (uint8_t) ((a << 4) | b);
}

///
/// \brief Converts a number of seconds in the mantissa/exponent format to a decimal number.
///
/// \param backpressure_format number of seconds in mantissa/exponent format.
/// \return the decimal number of seconds.
///
double
EmfToSeconds (uint8_t prob_olsrFormat)
{
  int a = (prob_olsrFormat >> 4);
  int b = (prob_olsrFormat & 0xf);
  // value = C*(1+a/16)*2^b [in seconds]
  return BACKPRESSURE_C * (1 + a/16.0) * (1 << b);
}



// ---------------- BACKPRESSURE Packet -------------------------------

NS_OBJECT_ENSURE_REGISTERED (PacketHeader);

PacketHeader::PacketHeader ()
{}

PacketHeader::~PacketHeader ()
{}

TypeId
PacketHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::backpressure::PacketHeader")
    .SetParent<Header> ()
    .AddConstructor<PacketHeader> ()
    ;
  return tid;
}
TypeId
PacketHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
PacketHeader::GetSerializedSize (void) const
{
  return BACKPRESSURE_PKT_HEADER_SIZE;
}

void 
PacketHeader::Print (std::ostream &os) const
{
  // TODO
}

void
PacketHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_packetLength);
  i.WriteHtonU16 (m_packetSequenceNumber);
}

uint32_t
PacketHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_packetLength  = i.ReadNtohU16 ();
  m_packetSequenceNumber = i.ReadNtohU16 ();
  return GetSerializedSize ();
}


// ---------------- BACKPRESSURE Message -------------------------------

NS_OBJECT_ENSURE_REGISTERED (MessageHeader);

MessageHeader::MessageHeader ()
  : m_messageType (MessageHeader::MessageType (0))
{}

MessageHeader::~MessageHeader ()
{}

TypeId
MessageHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::backpressure::MessageHeader")
    .SetParent<Header> ()
    .AddConstructor<MessageHeader> ()
    ;
  return tid;
}
TypeId
MessageHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
MessageHeader::GetSerializedSize (void) const
{
  uint32_t size = BACKPRESSURE_MSG_HEADER_SIZE;
  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      NS_LOG_DEBUG ("Hello Message Size: " << size << " + " << m_message.hello.GetSerializedSize ());
      size += m_message.hello.GetSerializedSize ();
      break;
    default:
      NS_ASSERT (false);
    }
  return size;
}

void 
MessageHeader::Print (std::ostream &os) const
{
  // TODO
}

void
MessageHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_messageType);
  i.WriteU8 (m_vTime);
  i.WriteHtonU16 (GetSerializedSize ());
  i.WriteHtonU32 (m_originatorAddress.Get ());
  i.WriteU8 (m_timeToLive);
  i.WriteU8 (m_hopCount);
  i.WriteHtonU16 (m_messageSequenceNumber);

  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      m_message.hello.Serialize (i);
      break;
    default:
      NS_ASSERT (false);
    }

}

uint32_t
MessageHeader::Deserialize (Buffer::Iterator start)
{
  uint32_t size;
  Buffer::Iterator i = start;
  m_messageType  = (MessageType) i.ReadU8 ();
  NS_ASSERT (m_messageType == HELLO_MESSAGE);
  m_vTime  = i.ReadU8 ();
  m_messageSize  = i.ReadNtohU16 ();
  m_originatorAddress = Ipv4Address (i.ReadNtohU32 ());
  m_timeToLive  = i.ReadU8 ();
  m_hopCount  = i.ReadU8 ();
  m_messageSequenceNumber = i.ReadNtohU16 ();
  size = BACKPRESSURE_MSG_HEADER_SIZE;
  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      size += m_message.hello.Deserialize (i, m_messageSize - BACKPRESSURE_MSG_HEADER_SIZE);
      break;
    default:
      NS_ASSERT (false);
    }
  return size;
}

// ---------------- BACKPRESSURE HELLO Message -------------------------------

uint32_t 
MessageHeader::Hello::GetSerializedSize (void) const
{
  //uint32_t size = 8;
  //uint32_t size = 24;
  //uint32_t size = 26;
  
  //uint32_t size = 26 + IPV4_ADDRESS_SIZE;
  //uint32_t size = 27 + IPV4_ADDRESS_SIZE; //due to the adding of one extra flags: n_interfaces
  uint32_t size = 27 + n_interfaces + IPV4_ADDRESS_SIZE;
  
  for (std::vector<LinkMessage>::const_iterator iter = this->linkMessages.begin ();
       iter != this->linkMessages.end (); iter++)
    {
      const LinkMessage &lm = *iter;
      size += 4;
      size += IPV4_ADDRESS_SIZE * lm.neighborInterfaceAddresses.size ();
    }
  return size;
}

void 
MessageHeader::Hello::Print (std::ostream &os) const
{
  // TODO
}
/*
void
WriteDouble (double v)
{
  uint8_t *buf = (uint8_t *)&v;
  for (uint32_t i = 0; i < sizeof (double); ++i, ++buf)
    {
      j.WriteU8 (*buf);
    }
  
}
*/

double
ReadDouble (Buffer::Iterator start)
{
  double v;
  Buffer::Iterator j = start;
  uint8_t *buf = (uint8_t *)&v;
  buf[7] = j.ReadU8 ();
  buf[6] = j.ReadU8 ();
  buf[5] = j.ReadU8 ();
  buf[4] = j.ReadU8 ();
  buf[3] = j.ReadU8 ();
  buf[2] = j.ReadU8 ();
  buf[1] = j.ReadU8 ();
  buf[0] = j.ReadU8 ();
  /*for (uint32_t i = 0; i < sizeof (double); ++i, ++buf)
    {
      *buf = j.ReadU8 ();
    }*/
NS_LOG_UNCOND ("value is "<<v);
  return v;
}

void
MessageHeader::Hello::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU16 (0); // Reserved
  i.WriteU8 (this->hTime);
  i.WriteU8 (this->willingness);
  i.WriteHtonU32 (this->qLength);
  //coordinates two doubles
  
  //reverse buffer1 and buffer2
  uint64_t *a = (uint64_t *)&(this->x);
  uint64_t *b = (uint64_t *)&(this->y);

  i.WriteU64(*a);//serialize x pos
  i.WriteU64(*b);//serialize y pos

  i.WriteHtonU16(this->pdr);//serialize packet delivery ratio
  i.WriteHtonU32(this->addr.Get ());//send addr associated to the previous pdr 
  
  i.WriteU8(this->n_interfaces);
  for (uint32_t j=0; j< this->n_interfaces; j++)
  {
    i.WriteU8(this->qLength_eth[j]);
  }
    
  for (std::vector<LinkMessage>::const_iterator iter = this->linkMessages.begin ();
       iter != this->linkMessages.end (); iter++)
    {
      const LinkMessage &lm = *iter;

      i.WriteU8 (lm.linkCode);
      i.WriteU8 (0); // Reserved

      // The size of the link message, counted in bytes and measured
      // from the beginning of the "Link Code" field and until the
      // next "Link Code" field (or - if there are no more link types
      // - the end of the message).
      i.WriteHtonU16 (4 + lm.neighborInterfaceAddresses.size () * IPV4_ADDRESS_SIZE);
      
      for (std::vector<Ipv4Address>::const_iterator neigh_iter = lm.neighborInterfaceAddresses.begin ();
           neigh_iter != lm.neighborInterfaceAddresses.end (); neigh_iter++)
        {
          i.WriteHtonU32 (neigh_iter->Get ());
        }
    }
}

uint32_t
MessageHeader::Hello::Deserialize (Buffer::Iterator start, uint32_t messageSize)
{
  Buffer::Iterator i = start;

  //NS_ASSERT (messageSize >= 8); add qlength 
  //NS_ASSERT (messageSize >= 24); add coordinates
  //NS_ASSERT (messageSize >= 26); add pdr
  //NS_ASSERT (messageSize >= 28);
  //NS_ASSERT (messageSize >= 26+IPV4_ADDRESS_SIZE);//add address
  NS_ASSERT (messageSize >= 27+IPV4_ADDRESS_SIZE);//add address
  this->linkMessages.clear ();
  
  uint16_t helloSizeLeft = messageSize;
  
  i.ReadNtohU16 (); // Reserved
  this->hTime = i.ReadU8 ();
  this->willingness = i.ReadU8 ();
  this->qLength= i.ReadNtohU32 ();
  /*char buffer1[sizeof(double)];
  char buffer2[sizeof(double)];
  for (uint16_t k=7; k>=0; k--){
    buffer1[k]=i.ReadU8 ();
  }
  for (uint16_t k=7; k>=0; k--){
    buffer2[k]=i.ReadU8 ();
  }*/
  //double a,b;
 // uint8_t *buf = (uint8_t *)&b;
  uint64_t *a = (uint64_t *)&(this->x);
  uint64_t *b = (uint64_t *)&(this->y);
  *a = i.ReadU64();
  *b = i.ReadU64();
  this->pdr  = i.ReadNtohU16();
  this->addr = Ipv4Address (i.ReadNtohU32 ());
  
  this->n_interfaces= i.ReadU8();
  for (uint32_t j=0; j<this->n_interfaces; j++)
    {
      this->qLength_eth.push_back(i.ReadU8());
    }
  //helloSizeLeft -= 8;
  //helloSizeLeft -= 24;
  //helloSizeLeft -= 26;
  
  //helloSizeLeft -= 30;
  helloSizeLeft -=(31+ (uint16_t) this->n_interfaces);

  while (helloSizeLeft)
    {
      LinkMessage lm;
      NS_ASSERT (helloSizeLeft >= 4);
      lm.linkCode = i.ReadU8 ();
      i.ReadU8 (); // Reserved
      uint16_t lmSize = i.ReadNtohU16 ();
      NS_ASSERT ((lmSize - 4) % IPV4_ADDRESS_SIZE == 0);
      for (int n = (lmSize - 4) / IPV4_ADDRESS_SIZE; n; --n)
        {
          lm.neighborInterfaceAddresses.push_back (Ipv4Address (i.ReadNtohU32 ()));
        }
      helloSizeLeft -= lmSize;
      this->linkMessages.push_back (lm);
    }

  return messageSize;
}

} // namespace backpressure
} // namespace ns3
