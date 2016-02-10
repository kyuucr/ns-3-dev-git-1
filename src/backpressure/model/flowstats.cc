#include "flowstats.h"

FlowStats::FlowStats()
{
  m_txTime = 0.0;
}

FlowStats::~FlowStats ()
{
}

void FlowStats::SetTsSource(Time time)
{
  m_tsSource = time.GetNanoSeconds(); 
}

void FlowStats::SetTsDest(Time time)
{
  m_tsDest = time.GetNanoSeconds(); 
}

void FlowStats::SetTxTime(float txtime)
{
  m_txTime = m_txTime + txtime; 
}

void FlowStats::SetDelay(Time time)
{
  m_delay = time.GetNanoSeconds();
}

void FlowStats::SetSeqNumber(uint32_t theSeq)
{
  m_seq = theSeq;
}

void FlowStats::SetDestinationIp(const Ipv4Address addr)
{
  m_dstIp=addr;
}


void FlowStats::SetSourceIp(const Ipv4Address addr)
{
  m_srcIp=addr;
}

void FlowStats::SetHops(uint32_t hops)
{
  m_hops=hops;
}

void FlowStats::SetSize(uint32_t size)
{
  m_size=size;
}

Time FlowStats::GetTsSource (void) const
{
  return NanoSeconds(m_tsSource);
}

Time FlowStats::GetTsDest (void) const
{
  return NanoSeconds(m_tsDest);
}

Time FlowStats::GetTxTime (void) const
{
  uint64_t tmp = uint64_t (m_txTime*1e9);
  return NanoSeconds(tmp);
}

float FlowStats::GetTxTimeF (void) const
{
  return m_txTime;
}

Time FlowStats::GetDelay   (void) const
{
  return NanoSeconds(m_delay);
}

uint32_t FlowStats::GetSeqNumber(void) const
{
  return (m_seq);
}

const Ipv4Address FlowStats::GetDestinationIp(){
  return m_dstIp;
}

const Ipv4Address FlowStats::GetSourceIp(){
  return m_srcIp;
}

uint32_t FlowStats::GetHops(void) const
{
  return m_hops;
}

uint32_t FlowStats::GetSize(void) const
{
  return m_size;
}
