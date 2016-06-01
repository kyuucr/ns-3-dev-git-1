/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 CTTC
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
 * Author: José Núñez <jnunez@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/config.h"
#include "ns3/seqtag.h"
#include "backpressure-tracing-helper.h"
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#define MAX_PACKETS 10000
#define MAX_INC 100
#define MAX_INTERFACES 100

NS_LOG_COMPONENT_DEFINE("BackpressureTracingHelper");


namespace ns3 {

BackpressureTracingHelper::BackpressureTracingHelper ()
  : m_interval (Seconds (1.0))
{
}

void
BackpressureTracingHelper::EnableBackpressure (std::string filename,  uint32_t nodeid, uint32_t deviceid)
{
  Ptr<BackpressureWifiTraceSink> backpressureStats = CreateObject<BackpressureWifiTraceSink> ();
  backpressureStats->SetDeviceId(deviceid);

  std::ostringstream oss;
  oss << filename
      << "_" << std::setfill ('0') << std::setw (3) << std::right <<  nodeid 
      << "_" << std::setfill ('0') << std::setw (3) << std::right << deviceid;
  backpressureStats->OpenStats (oss.str ());

  oss.str ("");
  oss << "/NodeList/" << nodeid;
  std::string devicepath = oss.str ();

  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/QLength", MakeCallback (&BackpressureWifiTraceSink::IncQueue, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/Vvariable", MakeCallback (&BackpressureWifiTraceSink::printV, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/QOverflow", MakeCallback (&BackpressureWifiTraceSink::IncQueueOverflow, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/TTL", MakeCallback (&BackpressureWifiTraceSink::IncExpiredTTL, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/TxData", MakeCallback (&BackpressureWifiTraceSink::IncTx, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/AvgQueue", MakeCallback (&BackpressureWifiTraceSink::AvgQueue, backpressureStats));
}

void
BackpressureTracingHelper::EnableBackpressure (std::string filename,  uint32_t nodeid, uint32_t deviceid, Ipv4Address srcIp, Ipv4Address dstIp)
{
  Ptr<BackpressureWifiTraceSink> backpressureStats = CreateObject<BackpressureWifiTraceSink> ();
  backpressureStats->SetDeviceId(deviceid);
  backpressureStats->SetSource(srcIp);
  backpressureStats->SetDst(dstIp);
  backpressureStats->SetNode(nodeid);

  std::ostringstream oss;
  oss << filename
      << "_" << std::setfill ('0') << std::setw (3) << std::right <<  nodeid 
      << "_" << std::setfill ('0') << std::setw (3) << std::right << deviceid;
  backpressureStats->OpenStats (oss.str ());

  oss.str("");
  oss << filename
      << "_packet"
      << "_" << std::setfill ('0') << std::setw (3) << std::right <<  nodeid 
      << "_" << std::setfill ('0') << std::setw (3) << std::right << deviceid;
  
  if (backpressureStats->GetDoPktStats())
  {
    backpressureStats->OpenPacketStats (oss.str ());
  }

  //NS_LOG_UNCOND("node id is "<<nodeid);
  oss.str ("");
  oss << "/NodeList/" << nodeid;
  std::string devicepath = oss.str ();

  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/QLength", MakeCallback (&BackpressureWifiTraceSink::IncQueue, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/Vvariable", MakeCallback (&BackpressureWifiTraceSink::printV, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/QOverflow", MakeCallback (&BackpressureWifiTraceSink::IncQueueOverflow, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/TTL", MakeCallback (&BackpressureWifiTraceSink::IncExpiredTTL, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/TxData", MakeCallback (&BackpressureWifiTraceSink::IncTx, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/AvgQueue", MakeCallback (&BackpressureWifiTraceSink::AvgQueue, backpressureStats));
}

void
BackpressureTracingHelper::EnableBackpressure (std::string filename,  uint32_t nodeid, uint32_t deviceid, Ipv4Address dstIp)
{
  Ptr<BackpressureWifiTraceSink> backpressureStats = CreateObject<BackpressureWifiTraceSink> ();
  backpressureStats->SetDeviceId(deviceid);
  backpressureStats->SetSource("0.0.0.0");
  backpressureStats->SetDst(dstIp);
  backpressureStats->SetNode(nodeid);

  std::ostringstream oss;
  oss << filename
      << "_" << std::setfill ('0') << std::setw (3) << std::right <<  nodeid 
      << "_" << std::setfill ('0') << std::setw (3) << std::right << deviceid;
  backpressureStats->OpenStats (oss.str ());

  oss.str("");
  oss << filename
      << "_packet"
      << "_" << std::setfill ('0') << std::setw (3) << std::right <<  nodeid 
      << "_" << std::setfill ('0') << std::setw (3) << std::right << deviceid;
  if (backpressureStats->GetDoPktStats())
  {
    backpressureStats->OpenPacketStats (oss.str ());
  }
  //NS_LOG_UNCOND("node id is "<<nodeid);
  oss.str ("");
  oss << "/NodeList/" << nodeid;
  std::string devicepath = oss.str ();

  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/QLength", MakeCallback (&BackpressureWifiTraceSink::IncQueue, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/Vvariable", MakeCallback (&BackpressureWifiTraceSink::printV, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/QOverflow", MakeCallback (&BackpressureWifiTraceSink::IncQueueOverflow, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/TTL", MakeCallback (&BackpressureWifiTraceSink::IncExpiredTTL, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/TxData", MakeCallback (&BackpressureWifiTraceSink::IncTx, backpressureStats));
  Config::Connect (devicepath + "/$ns3::backpressure::RoutingProtocol/AvgQueue", MakeCallback (&BackpressureWifiTraceSink::AvgQueue, backpressureStats));
}

void
BackpressureTracingHelper::EnableBackpressure (std::string filename, Ptr<NetDevice> nd)
{
  EnableBackpressure (filename, nd->GetNode ()->GetId (), nd->GetIfIndex ());
}

void 
BackpressureTracingHelper::EnableBackpressure (std::string filename, NetDeviceContainer d)
{
  for (NetDeviceContainer::Iterator i = d.Begin (); i != d.End (); ++i)
    {
      Ptr<NetDevice> dev = *i;
      EnableBackpressure (filename, dev->GetNode ()->GetId (), dev->GetIfIndex ());
    }
}

void 
BackpressureTracingHelper::EnableBackpressure (std::string filename, NetDeviceContainer d, Ipv4Address srcIp, Ipv4Address dstIp)
{
  for (NetDeviceContainer::Iterator i = d.Begin (); i != d.End (); ++i)
    {
      Ptr<NetDevice> dev = *i;
      EnableBackpressure (filename, dev->GetNode ()->GetId (), dev->GetIfIndex (), srcIp, dstIp);
    }
}

void 
BackpressureTracingHelper::EnableBackpressure (std::string filename, NetDeviceContainer d, Ipv4Address dstIp)
{
  for (NetDeviceContainer::Iterator i = d.Begin (); i != d.End (); ++i)
    {
      Ptr<NetDevice> dev = *i;
      EnableBackpressure (filename, dev->GetNode ()->GetId (), dev->GetIfIndex (), dstIp);
    }
}

void
BackpressureTracingHelper::EnableBackpressure (std::string filename, NodeContainer n)
{
  NetDeviceContainer devs;
  for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
    {
      Ptr<Node> node = *i;
      for (uint32_t j = 0; j < node->GetNDevices (); ++j)
        {
          devs.Add (node->GetDevice (j));
        }
    }
  EnableBackpressure (filename, devs);
}

PacketStats::PacketStats()
{
}

uint64_t 
PacketStats::GetPacketId()
{
  return m_idPacket;
}

uint64_t
PacketStats::GetTime()
{
  return m_arrivalTime;
}

void
PacketStats::SetTime(Time theTime)
{
  m_arrivalTime = theTime.GetNanoSeconds();
}

void
PacketStats::SetPacketId(uint64_t theSeq)
{
  m_idPacket = theSeq;
}

void
PacketStats::SetSource(const Ipv4Address theAddr)
{
  m_addr = theAddr;
}

const Ipv4Address
PacketStats::GetSource()
{
  return m_addr;
}

NS_OBJECT_ENSURE_REGISTERED (BackpressureWifiTraceSink);

TypeId 
BackpressureWifiTraceSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BackpressureWifiTraceSink")
    .SetParent<Object> ()
    .AddConstructor<BackpressureWifiTraceSink> ()
    .AddAttribute ("Interval",
                   "Time interval between reports",
                   TimeValue (Seconds(0.1)),
                   MakeTimeAccessor (&BackpressureWifiTraceSink::m_interval),
                   MakeTimeChecker ())
    ;
  return tid;
}

BackpressureWifiTraceSink::BackpressureWifiTraceSink ()
  :   m_qLength (0),
      m_ifaces(0),
      m_v (0),
      m_qOflow(0),
      m_expTTL(0),
      //m_TxData(0),
      m_queueAvg(0),
      m_do_pktStats(false),
      m_Awriter(0),
      m_Anotherwriter(0),
      m_size(MAX_PACKETS),
      m_seconds_file(0) 
{
  
  for (uint32_t i=0; i<MAX_INTERFACES; i++)
  {
    m_TxData.push_back(0);
    m_qLength_ifaces.push_back(0);
  }
  
  if (m_do_pktStats)
  {
    m_packetStats.reserve(MAX_PACKETS); //conservative to be updated
  }
  Simulator::ScheduleNow (&BackpressureWifiTraceSink::WriteStats, this);
}

BackpressureWifiTraceSink::~BackpressureWifiTraceSink ()
{
  NS_LOG_FUNCTION (this);
  if ( (m_Awriter != 0) && (m_Anotherwriter !=0) )
  {
      NS_LOG_LOGIC (" nonzero " << m_Awriter);
      if (m_Awriter->is_open ())
      {
          NS_LOG_LOGIC (" opened.  Closing " << m_Awriter);
          m_Awriter->close ();
      }
      if (m_Anotherwriter->is_open ())
      {
          NS_LOG_LOGIC (" opened.  Closing " << m_Anotherwriter);
          m_Anotherwriter->close ();
      }
      NS_LOG_LOGIC ("Deleting writer " << m_Awriter);
      NS_LOG_LOGIC ("Deleting writer " << m_Anotherwriter);
      delete m_Awriter;
      delete m_Anotherwriter;
      NS_LOG_LOGIC ("m_Awriter = 0");
      NS_LOG_LOGIC ("m_Anotherwriter = 0");
      m_Awriter = 0;
      m_Anotherwriter = 0;
      if (m_do_pktStats==true)
      {				
        m_packetStats.clear();	//clear vector of packet statistics
      }
  }
  else
  {
    NS_LOG_LOGIC ("m_Anotherwriter == 0 and m_Awriter == 0");
  }
}

void    
BackpressureWifiTraceSink::ResetCounters ()
{
  //m_qLength = 0;
  m_qOflow = 0;
  //m_TxData = 0;
  for(std::vector<uint32_t>::iterator it = m_TxData.begin(); it != m_TxData.end(); ++it) 
      *it = 0 ;
  m_expTTL=0;
  //m_txCount = 0;
  if (m_do_pktStats==true)
  {				
    m_packetStats.erase ( m_packetStats.begin(), m_packetStats.end() );    // erasing by range
  }
}

void
BackpressureWifiTraceSink::IncQueue (std::string context, uint32_t length, std::vector<uint32_t> IfacesSizes, uint32_t n_interfaces)
{
  NS_LOG_FUNCTION (this << context <<length);
  m_qLength = length;
  m_ifaces = n_interfaces;
  for (uint32_t i=0; i<m_ifaces; i++)
  {
    m_qLength_ifaces[i]= IfacesSizes[i];
  }
}

void
BackpressureWifiTraceSink::AvgQueue (std::string context, uint32_t length)
{
  NS_LOG_FUNCTION (this << context << length);
  m_queueAvg = length;
}

void
BackpressureWifiTraceSink::printV( std::string context, uint32_t v)
{
  NS_LOG_FUNCTION (this << context <<v);
  m_v = v;
}

void
BackpressureWifiTraceSink::IncQueueOverflow (std::string context, uint32_t length)
{
  NS_LOG_FUNCTION (this << context <<length);
  m_qOflow++;
}

void
BackpressureWifiTraceSink::IncExpiredTTL (std::string context, uint32_t expired)
{
  NS_LOG_FUNCTION (this << context <<expired);
  m_expTTL++;
}

void
BackpressureWifiTraceSink::IncTx (std::string context, const Ipv4Header &h, Ptr<const Packet> p, uint32_t deviceid)
{
  NS_LOG_FUNCTION (this << context <<p);
  //m_TxData++;
  m_TxData[deviceid]++;
  uint32_t sum_m_TxData=0;
  SequenceTag seqTag;
  bool foundSeq = p->FindFirstMatchingByteTag(seqTag);
  if ( foundSeq )
  {
    if (m_do_pktStats)
    {
      //NS_LOG_INFO("[IncTx]: using packet stats");
      for (std::vector<uint32_t>::iterator it = m_TxData.begin(); it != m_TxData.end(); ++it)
	sum_m_TxData = sum_m_TxData + *it;
      if ( sum_m_TxData < m_size )
      {
        //add more stats
        PacketStats aPacketStats;
        aPacketStats.SetTime(Simulator::Now());
        aPacketStats.SetPacketId(seqTag.GetSequence());
        aPacketStats.SetSource(h.GetSource());
        m_packetStats.push_back(aPacketStats);         //conservative to be updated
      } 
      else
      {
        //NS_LOG_UNCOND("resizing on node"); 
        m_packetStats.resize(MAX_INC);
        m_size = m_size + MAX_INC;
      }
    }
  }
}

void
BackpressureWifiTraceSink::SetSource (Ipv4Address src)
{
  m_src= src;
}

void
BackpressureWifiTraceSink::SetDst (Ipv4Address dst)
{
  m_dst = dst;
}

void
BackpressureWifiTraceSink::SetNode (uint32_t nodeId)
{
  m_idNode = nodeId;
}

void BackpressureWifiTraceSink::SetDeviceId (uint32_t deviceId)
{
  m_deviceId = deviceId;
}

bool
BackpressureWifiTraceSink::GetDoPktStats ()
{
  return m_do_pktStats;
}


void
BackpressureWifiTraceSink::OpenStats (std::string const &name)
{
  NS_LOG_FUNCTION (this << name);
  NS_ABORT_MSG_UNLESS ( m_Awriter == 0, "BackpressureWifiTraceSink::OpenStats (): m_Awriter already allocated (std::ofstream leak detected)");

  m_Awriter = new std::ofstream ();
  NS_ABORT_MSG_UNLESS (m_Awriter, "BackpressureWifiTraceSink::OpenStats (): Cannot allocate m_Awriter");

  NS_LOG_INFO (this << " [OpenStats]: Created writer");

  m_Awriter->open (name.c_str (), std::ios_base::binary | std::ios_base::out);
  NS_ABORT_MSG_IF (m_Awriter->fail (), "BackpressureWifiTraceSink::OpenStats (): m_Awriter->open (" << name.c_str () << ") failed");

  NS_ASSERT_MSG (m_Awriter->is_open (), "BackpressureWifiTraceSink::OpenStats (): m_Awriter not open");
  NS_LOG_LOGIC ("Writer opened successfully");
}

void
BackpressureWifiTraceSink::OpenPacketStats (std::string const &name)
{
  NS_LOG_FUNCTION (this << name);
  NS_ABORT_MSG_UNLESS ( m_Anotherwriter == 0, "BackpressureWifiTraceSink::OpenPacketStats (): m_Awriter already allocated (std::ofstream leak detected)");

  m_Anotherwriter = new std::ofstream ();
  NS_ABORT_MSG_UNLESS (m_Anotherwriter, "BackpressureWifiTraceSink::OpenPacketStats (): Cannot allocate m_Awriter");

  //NS_LOG_UNCOND ("Created writer " << );

  m_Anotherwriter->open (name.c_str (), std::ios_base::binary | std::ios_base::out);
  NS_ABORT_MSG_IF (m_Anotherwriter->fail (), "BackpressureWifiTraceSink::OpenPacketStats (): m_Anotherwriter->open (" << name.c_str () << ") failed");

  NS_ASSERT_MSG (m_Anotherwriter->is_open (), "BackpressureWifiTraceSink::OpenPacketStats (): m_Anotherwriter not open");
  NS_LOG_LOGIC ("Writer opened successfully");
}

void
BackpressureWifiTraceSink::WriteStats ()
{
  NS_LOG_LOGIC ("[BackpressureWifiTraceSink]: Writing Stats now=" <<Simulator::Now() );

  if (m_do_pktStats)
  {
    if (m_Anotherwriter)
    {
      for ( std::vector< PacketStats >::iterator it = m_packetStats.begin(); it!=m_packetStats.end(); ++it )
      {      
      //NS_LOG_UNCOND(" Id is "<< it->GetId()<<" Time "<<it->GetTime());
        *m_Anotherwriter << m_idNode
                         << " " << it->GetSource()
                         << " " << m_dst
                         << " " << it->GetPacketId()
                         << " " << it->GetTime()
                         << std::endl;
      }
    }
  }
  if (m_Awriter)
  {
    *m_Awriter << m_qLength
               << " "  << m_qOflow
               << " "  << m_TxData[m_deviceId]
               << " "  << m_expTTL
               << " "  << m_v
               //<< " "  << m_queueAvg
	       << " 	   "  << m_seconds_file;
for (uint32_t i=0; i<m_ifaces; i++)
     {
       *m_Awriter << " " << m_qLength_ifaces[i];
     }
	*m_Awriter << std::endl;
	 m_seconds_file = m_seconds_file +1;
    ResetCounters ();
    Simulator::Schedule (m_interval, &BackpressureWifiTraceSink::WriteStats, this);
  }
}

} // namespace ns3
