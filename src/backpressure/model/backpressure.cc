/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Jose Núñez
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
 * Author:  José Núñez  <jose.nunez@cttc.cat>
 *       
 */

///
/// \file	backpressure-routing-protocol.cc
/// \brief	Implementation of BACKPRESSURE agent and related classes.
///
/// This is the main file of this software because %BACKPRESSURE's behaviour is
/// implemented here.
///

#define NS_LOG_APPEND_CONTEXT                                   \
  if (GetObject<Node> ()) { std::clog << "[node " << GetObject<Node> ()->GetId () << "] "; }

#include "backpressure.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/callback.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
// new headers
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
//GTP header
#include "ns3/epc-gtpu-header.h"
// end new headers
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/location-list.h"
#include "ns3/ipv4-route.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/dca-txop.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/queue.h"
#include "ns3/node-list.h"

#include "ns3/seqtag.h"

#include <algorithm>
#include <map>

/********** Useful macros **********/

///
/// \brief Gets the delay between a given time and the current time.
///
/// If given time is previous to the current one, then this macro returns
/// a number close to 0. This is used for scheduling events at a certain moment.
///
#define DELAY(time) (((time) < (Simulator::Now ())) ? Seconds (0.000001) : \
                     (time - Simulator::Now () + Seconds (0.000001)))

/********** Holding times **********/
/// Neighbor time.
#define BACKPRESSURE_NEIGHB_VALID Seconds(m_helloInterval.GetSeconds()*3)

/// Neighbor holding time.
#define BACKPRESSURE_NEIGHB_HOLD_TIME Seconds (30)


/********** Willingness **********/  //IS WILLINGNESS NECESSARY FOR HELLO MESSAGES

/// Willingness for forwarding packets from other nodes: never.
#define BACKPRESSURE_WILL_NEVER		0
/// Willingness for forwarding packets from other nodes: low.
#define BACKPRESSURE_WILL_LOW		1
/// Willingness for forwarding packets from other nodes: medium.
#define BACKPRESSURE_WILL_DEFAULT	3
/// Willingness for forwarding packets from other nodes: high.
#define BACKPRESSURE_WILL_HIGH		6
/// Willingness for forwarding packets from other nodes: always.
#define BACKPRESSURE_WILL_ALWAYS	7

/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define BACKPRESSURE_MAXJITTER		(m_helloInterval.GetSeconds ())
/// Maximum allowed sequence number.
#define BACKPRESSURE_MAX_SEQ_NUM	65535
/// Random number between [0-BACKPRESSURE_MAXJITTER] used to jitter OLSR packet transmission.
//#define JITTER (Seconds (UniformRandomVariable().GetValue (0, BACKPRESSURE_MAXJITTER/4)))
#define JITTER (Seconds (m_uniformRandomVariable->GetValue (0, BACKPRESSURE_MAXJITTER/4)))

#define BACKPRESSURE_PORT_NUMBER 698 		// Wireshark protocol number to be updated for a backpressure; dissector for wireshark
/// Maximum number of messages per packet.
#define BACKPRESSURE_MAX_MSGS		64

// Direction determination in the ring topology
#define CLOCKWISE	0
#define COUNTER_CLOCKWISE	1

namespace ns3 {
namespace backpressure {

NS_LOG_COMPONENT_DEFINE ("BackpressureAgent");


/********** BACKPRESSURE class **********/

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

TypeId 
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::backpressure::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&RoutingProtocol::m_helloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("VcalInterval", "V recalculation interval.",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&RoutingProtocol::m_vcalcInterval),
                   MakeTimeChecker ())
    .AddAttribute ("QueueAvgInterval", "Sampling queue to compute avg. occupation.",
		   TimeValue (Seconds (0.1)),
		   MakeTimeAccessor (&RoutingProtocol::m_queueAvgInterval),
		   MakeTimeChecker ())
    .AddAttribute ("Willingness", "Willingness of a node to carry and forward traffic for other nodes.",
                   EnumValue (BACKPRESSURE_WILL_DEFAULT),
                   MakeEnumAccessor (&RoutingProtocol::m_willingness),
                   MakeEnumChecker (BACKPRESSURE_WILL_NEVER, "never",
                                    BACKPRESSURE_WILL_LOW, "low",
                                    BACKPRESSURE_WILL_DEFAULT, "default",
                                    BACKPRESSURE_WILL_HIGH, "high",
                                    BACKPRESSURE_WILL_ALWAYS, "always"))
    .AddAttribute ("VStrategy", "V parameter policy",
                   EnumValue (FIXED_V),
                   MakeEnumAccessor (&RoutingProtocol::m_vpolicy),
                   MakeEnumChecker (FIXED_V, "fixed",
                                    VARIABLE_V_SINGLE_RADIO, "variable-sradio",
                                    VARIABLE_V_MULTI_RADIO, "variable-mradio"))
    .AddAttribute ("LocalStrategy", "Backpressure Routing policy.",
                   EnumValue (BACKPRESSURE_CENTRALIZED_FLOW),
                   MakeEnumAccessor (&RoutingProtocol::m_variant),
                   MakeEnumChecker( BACKPRESSURE_CENTRALIZED_PACKET, "centralized_packet",
                                    BACKPRESSURE_CENTRALIZED_FLOW, "centralized_flow",
                                    BACKPRESSURE_DISTRIBUTED_PENA, "distributed_a",
                                    BACKPRESSURE_DISTRIBUTED_PENB, "distributed_b",
                                    BACKPRESSURE_DISTRIBUTED_PENC, "distributed_c",
                                    GREEDY_FORWARDING, "greedy",
				     MP_FORWARDING, "mp_greedy",
 				     BACKPRESSURE_PENALTY_RING, "penaltyring"))
    .AddAttribute ("V", "weight of the penalty function",
		   UintegerValue(0),
		   MakeUintegerAccessor(&RoutingProtocol::m_weightV),
   		   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QPolicy", "Data queue policy",
                   EnumValue (FIFO_RQUEUE),
                   MakeEnumAccessor (&RoutingProtocol::m_queuePolicy),
                   MakeEnumChecker (FIFO_RQUEUE, "fifo",
                                    LIFO_RQUEUE, "lifo",
                                    EDF_RQUEUE, "edf",
				     RING_RQUEUE, "ring",
				     GRID_RQUEUE, "grid"))
    .AddAttribute ("MaxQueueLen", "Size of the queue",
                   UintegerValue(0/*0 means infinite queues*/),
                   MakeUintegerAccessor (&RoutingProtocol::m_maxQueueLen),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RowNodes", "Number of nodes in a row",
		    UintegerValue(5),
		    MakeUintegerAccessor (&RoutingProtocol::m_nodesRow),
		    MakeUintegerChecker<uint32_t>())
    .AddAttribute ("HybridNetwork", "Set wether the network is hybrid or not",
		    UintegerValue(0),
		    MakeUintegerAccessor (&RoutingProtocol::m_hybrid),
		    MakeUintegerChecker<uint32_t>())
    .AddTraceSource ("QLength", "Queue Length.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_qLengthTrace),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("Vvariable", "V parameter.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_vTrace),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("QOverflow", "Number of times a control packet is discarded due to queue overflow",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_qOverflowTrace),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("TTL", "Number of times a control packet is discarded due to exceded TTL",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_expTTL),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("AvgQueue", "Average number of elements in the queue during the last second",
		     MakeTraceSourceAccessor (&RoutingProtocol::m_queueAvg),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("TxBeginData", "Number of packets originated at the node",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_TxBeginData),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("TxEnd", "Packet received by this node",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_TxEnd),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("TxData", "Number of times a control packet is discarded due to queue overflow",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_TxData),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    .AddTraceSource ("Rx", "Receive HELLO packet.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_rxPacketTrace),
                     "ns3::backpressure::RoutingProtocol::PacketTxRxTracedCallback")
    .AddTraceSource ("Tx", "Send HELLO packet.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_txPacketTrace),
		     "ns3::backpressure::RoutingProtocol::PacketTxRxTracedCallback")
//    .AddTraceSource ("RoutingTableChanged", "The BP routing table has changed.",
//		     MakeTraceSourceAccessor (&RoutingProtocol::m_routingTableChanged),
//			)
    .AddTraceSource ("TxDataAir", "Time to transmit the packet over the air interface.",
		     MakeTraceSourceAccessor(&RoutingProtocol::m_TxDataAir),
		     "ns3::backpressure::RoutingProtocol::TraceBackpressureParams")
    ;
  return tid;
}


RoutingProtocol::RoutingProtocol ()
  : m_ipv4 (0),
    m_helloTimer (Timer::CANCEL_ON_DESTROY),
    m_vcalcTimer (Timer::CANCEL_ON_DESTROY),
    m_avgqueueTimer (Timer::CANCEL_ON_DESTROY),
    m_reconfTimer (Timer::CANCEL_ON_DESTROY),
    m_queuedMessagesTimer (Timer::CANCEL_ON_DESTROY)    
{
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
}

RoutingProtocol::~RoutingProtocol ()
{
}

//a packet should be dequeued

void
RoutingProtocol::TxOpportunity (Address MAC_Address) //one interface asking to tx
{ ///callback function for wifi/ppp interface multi-radio multi-queue
  NS_ASSERT(m_ipv4);
  if (m_reconfState)
    { //we cannot wait until we change this flag
      return;
    }
  //create the route entry
  //std::cout<<"La mac address es: "<<MAC_Address<<" ,tiempo:"<<Simulator::Now()<<std::endl;
  Ptr<Ipv4Route> rtentry;
  Ipv4Address the_nh("127.0.0.1");
  EntryRoutingPacketQueue anEntry; 
  rtentry = Create<Ipv4Route> ();
  uint32_t woke_interface;
  uint32_t iface;
  MapMACAddressToIPinterface(MAC_Address, woke_interface);
  uint32_t queue_woke_interface = woke_interface;
  if (!m_queuedData->ReadEntry(anEntry,queue_woke_interface))
    {
        //there are not remaining packets in the queue
      return;
    }
  Ipv4Address dstAddr= anEntry.GetHeader().GetDestination();
  Ipv4Address srcAddr= anEntry.GetHeader().GetSource();
  Mac48Address hwAddr = anEntry.GetHwAddres();
  Ipv4InterfaceAddress ifAddr;
  ///variables not used for the moment
  /*bool centraliced=false;
  if (m_requeue_loop)
    {
      centraliced=true;
    }
  uint32_t qlen_max;*/
  //we give priority to the requeued packet to go out through this interface
  Ptr<NetDevice> aDev = m_ipv4->GetNetDevice (woke_interface);
  Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
  
  //aqui lo mismo: mirar si el paquete tiene regla y si no tiene buscar una y actualizar. En teoria requeued no podria ser 1, pq ya se le habría buscado una regla antes.
  //entonces si los paquetes tienen que salir por otra interfaz, pues meterlos en la nueva interfaz
  
  if (aWifiDev == NULL)
  {
    //it is a PtoP device
    //this code is valid in the multi-radio SANSA context. 
    //the requeue flag will not be set different to zero before determining its next_hop
    if((anEntry.requeued==1) || (anEntry.requeued==2 ))
    {
      //std::cout<<"UL TRAFFIC callback requeued in node"<<m_ipv4->GetObject<Node>()->GetId()<<"!!"<<std::endl;
      //this packet is derived to the other interface because there is overloading in the queue, so we should sent it
      the_nh=m_state.FindtheNeighbourAtThisInterface(woke_interface);
      rtentry->SetGateway (the_nh);
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (woke_interface, 0);
      rtentry->SetSource (oifAddr.GetLocal ());
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (woke_interface));
      SendQueuedData(rtentry);
      return;
    }
  }
  ifAddr.SetLocal(m_mainAddress);
  
  ///Determining the next-hop and the iface
  Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
  uint8_t ttl = anEntry.GetHeader().GetTtl();
  uint32_t v=200;
  VcalcTimerExpire (woke_interface, m_requeue_loop);
  v=m_weightV;
  if (m_variant == BACKPRESSURE_CENTRALIZED_PACKET || m_variant == BACKPRESSURE_CENTRALIZED_FLOW) 
    {
      Ptr<Packet> currentPacket = anEntry.GetPacket()->Copy();
      Ipv4Header tmp_header = anEntry.GetHeader();
      uint16_t s_port=999, d_port=999;
      GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);
      bool SatFlow = false;
      
      if (LocationList::m_SatGws.size()>0)
	{
	  //there are Satellite gws in the network
	  uint16_t ref_port=1; //by default through the terrestrial
	  if (d_port > 10000)
	    {
	      ref_port = s_port;
	    }
	  else
	    {
	      ref_port = d_port;
	     }
	  bool reset = LocationList::GetResetLocationList();
	  //bool reset = true;
	  if (!reset)
	    {
	      if (ref_port%2==0)
	        {
		  SatFlow = true;
	        }
	    }
	  else
	    { //m_reset true, odd ports go thorough satellite
	      if (ref_port%2!=0)
		SatFlow = true;
	    } 
    /*	        ///not to force satflows
	        SatFlow = false;*/

	}
	
	bool erased = false; //this is flag in case we want to keep a path
	Ipv4Address prev_nh("127.0.0.1");
	if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	  {
	    the_nh = m_state.FindNextHopInFlowTable(m_ipv4, srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, iface, erased, prev_nh, tmp_header.GetTtl());
	    //check that the interface is available: if not, force to find another path
	    if (!the_nh.IsEqual("127.0.0.1") && !m_ipv4->IsUp(iface) && !erased)
	      {
	        //we erase the rule if the selected iface not available. 
	        the_nh.Set("127.0.0.1");
	        m_state.EraseFlowTableEntry(srcAddr, dstAddr, s_port, d_port);
	      }
	  }
	if (the_nh.IsEqual("127.0.0.1"))
	  {
	    //sansa-lena
	    if (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2") )
	      { //the packet is in EPC and has been deencapsulated
	        //std::cout<<"UL TRAFFIC  callback in node"<<m_ipv4->GetObject<Node>()->GetId()<<"!!"<<std::endl;
	        the_nh = dstAddr;
		uint8_t buf[4];
		the_nh.Serialize(buf);
		buf[3]=buf[3]-1;
		Ipv4Address tmp_addr= Ipv4Address::Deserialize(buf);
		iface = m_ipv4->GetInterfaceForAddress(tmp_addr);
	      }
	   else
	      {
		if (LocationList::m_SatGws.size()>0)
		  {
		    Ptr<Node> node = NodeList::GetNode(0);
		    Ipv4Address tmp_addr = node->GetObject<Ipv4>()->GetAddress(3,0).GetLocal();
		    if (dstAddr.IsEqual(tmp_addr))
		      {  //these cases are to force the marked SatFlows to go through the satellite
		        //std::cout<<"UL TRAFFIC callback in node"<<m_ipv4->GetObject<Node>()->GetId()<<"!!"<<std::endl;
		        if (m_ipv4->GetObject<Node>()->GetId() == (NodeList::GetNNodes()-1) || !(SatFlow) )
		          { //we are in the node that connects the satellite link with the EPC (the extraEnodeB)
		            ///new function to determine next hop in multi-radio
		            ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);    
			    the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, woke_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		          }
		        else
		          { //this is a satellite flow and we are in the terrestrial backhaul. We have to redirect the flow to a sat gw
		            Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
		            Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
		            Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		            //std::cout<<"La ip address del sat Node es: "<<dstAddr<<std::endl;
		            ///new function to determine next hop in multi-radio
		            ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			    the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, woke_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		          }
		      }
		    else
		      {
			//std::cout<<"DL TRAFFIC callback in node"<<m_ipv4->GetObject<Node>()->GetId()<<"!!"<<std::endl;
			if (m_ipv4->GetObject<Node>()->GetId() == 0 && SatFlow)
			  { //this case is to force that a SatFlow goes through the satellite 
			    Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
			    Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
			    Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
			    ///new function to determine next hop in multi-radio
		            ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			    the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, woke_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			  }
			else
			  {
			    ///new function to determine next hop in multi-radio
			    ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			    the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, woke_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			  } 
		      }
		  }
		else
		  {
		    //there are no satellite gw's.
		    ///new function to determine next hop in multi-radio
		    ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		    //std::cout<<"DL TRAFFIC callback in node"<<m_ipv4->GetObject<Node>()->GetId()<<"!!"<<std::endl;
		    the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, woke_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		    //std::cout<<"El nexthop es: "<<the_nh<<" y la iface: "<<iface<<std::endl;
		  }
              }    
	    if (!the_nh.IsEqual("127.0.0.1") && m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	      {
		//the flow table is only updated when we have a neighbor, otherwise we can create a wrong rule, creating a loop
	        m_state.UpdateFlowTableEntry(srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, the_nh, iface, ttl);
		if (m_ipv4->GetObject<Node>()->GetId() == 0)
		  std::cout<<"the nexthop en EPC "<<m_ipv4->GetObject<Node>()->GetId()<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time: "<<Simulator::Now()<<" destination port: "<<d_port<<" source port: "<<s_port<<std::endl;
		else
		  std::cout<<"the nexthop en eNodeB "<<m_ipv4->GetObject<Node>()->GetId()-1<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time "<<Simulator::Now()<<" y destination port: "<<d_port<<" source port: "<<s_port<<std::endl;
	      }  
	  } //end the_nh==127.0.0.1      
    } //end m_variant
  
 uint32_t output_interface=woke_interface; 
///Deciding where to send the packet   
    NS_LOG_DEBUG("[TxOpportunityCallback]: the next hop is "<<the_nh);
    if (iface == 0) //packet cannot be forwarded
      {
        //packet cannot be transmitted for any interface
        return;
      }
    if ( !the_nh.IsEqual("127.0.0.1") && m_ipv4->IsUp(iface)) 
      { //there is a next hop and the interface is available
        if (iface == output_interface)
	  {
	    //packet sent through this interface
            Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
            rtentry->SetSource (oifAddr.GetLocal ());
	    rtentry->SetGateway (the_nh);
	    rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
	    SendQueuedData(rtentry);
	    return;
	  }
	else
	  {
	    //iface !=woke_interface: the packet was not enqueued properly
            //an enqueue, prior to make the SendQueuedData operation
	    if (anEntry.requeued!=0 && m_ipv4->IsUp(output_interface))
              { //in theory this should never happen as when changing interfaces
                //we empty the queues
	        //this packet has been treated before, so we can send it
                Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (output_interface, 0);
                rtentry->SetSource (oifAddr.GetLocal ());
	        rtentry->SetGateway (the_nh);
	        rtentry->SetOutputDevice (m_ipv4->GetNetDevice (output_interface));
	        SendQueuedData(rtentry);
	        return;
              }
            else
              { //entry requeued ==0, so we dequeue from output_interface and we send through interface
	        uint32_t tmp = output_interface;
	        bool ret = m_queuedData->Dequeue(anEntry, tmp);
	        if (ret)
	          {
	            anEntry.requeued = 1;
	            QueueData(anEntry.GetPacket(), anEntry.GetHeader(), hwAddr, anEntry.GetUcb(), iface, ret, anEntry.GetTstamp());
	          }
	        else
	          return;
              } 
            Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
            rtentry->SetSource (oifAddr.GetLocal ());
            rtentry->SetGateway (the_nh);
	    rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
	    SendQueuedData(rtentry);   
	    return;     
	  }
      }
  return;
  
  ///Deciding where to send the packet
//   NS_LOG_DEBUG("[TxOpportunityCallback]: the next hop is "<<the_nh);
//   if (iface == 0)
//     {
//       //packet cannot be transmitted for any interface
//       return;
//     }
//   if ( !the_nh.IsEqual("127.0.0.1") && m_ipv4->IsUp(iface))
//   {
//     if (iface == woke_interface)
//       {
// 	//the packet can be sent for the eth that requested the opportunity
//         rtentry->SetGateway (the_nh);
//         Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
//         Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (woke_interface, 0);
//         rtentry->SetSource (oifAddr.GetLocal ());
//         rtentry->SetOutputDevice (m_ipv4->GetNetDevice (woke_interface));
//         SendQueuedData(rtentry);
//       }
//     else
//       {// iface != woke_interface: the packet was not enqueued properly
//         rtentry->SetGateway (the_nh);
//         Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
//         Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
//         rtentry->SetSource (oifAddr.GetLocal ());
//         rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
//         Ptr<NetDevice> aDev = rtentry->GetOutputDevice();
// 	bool serviced = (GetMacQueueSize(aDev) < 2);
// 	if (serviced)
//           { //let's transmit through the other interface
//             bool canItx = m_queuedData->Dequeue(anEntry, queue_woke_interface);
//             if (canItx)
//               {
// 	        //std::cout<<"TxOpportunity changes the interface and transmit through other iface"<<std::endl;
// 	        UnicastForwardCallback qucb;
//                 Ptr<const Packet> qPacket;
//                 Ipv4Header qHeader;
// 	        qHeader = anEntry.GetHeader();
//                 qucb = anEntry.GetUcb();
//                 qPacket = anEntry.GetPacket(); 
// 	        Ptr<PointToPointNetDevice> aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
//                 Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
// 	        uint64_t rate_bps;
// 	        int32_t mode;
// 	        if (aWifiDev == NULL)
// 	          { //ppp device
// 	            rate_bps = aPtopDev->GetRatebps();
// 		    mode = 2;
// 	          }
// 	        else
// 	          {
// 		    rate_bps = aWifiDev->GetRatebps();
// 		    mode = 1;
// 	          }
// 	        m_TxData(qHeader, qPacket, aDev->GetIfIndex ()); 
// 	        m_TxDataAir(qPacket, qHeader,rate_bps, mode);	  
//                 rtentry->SetSource(qHeader.GetSource());
//                 rtentry->SetDestination(qHeader.GetDestination());
//                 qucb (rtentry, qPacket, qHeader);
// 		
// 	      }
// 	    else
// 	      {
// 	        std::cout<<"Desencolo para nada"<<std::endl;
// 	      }
//           }
//         else
//           {
// 	    //if not, we will enqueue this packet at the beginning of the other queue
// 	    //std::cout<<"TxOpportunity, la ruta se cambia y se tx por el otro interfaz"<<std::endl;
// 	    if (anEntry.requeued==0 || anEntry.requeued==2)
// 	      {
// 	        bool requeue_the_packet = m_queuedData->Dequeue(anEntry, queue_woke_interface);
// 	        if (requeue_the_packet)
// 	        {
// 	          anEntry.requeued = 1;
// 	          QueueData(anEntry.GetPacket(), anEntry.GetHeader(),hwAddr, anEntry.GetUcb(), iface, requeue_the_packet, anEntry.GetTstamp()); 
// 	        }  
// 	      else
// 	        return;
// 	      }
// 	    else
// 	      return;	    
//           }
//       }        
//   } //end if !the_nh.IsEqual("127.0.0.1")
}

void
RoutingProtocol::FireTxOpportunities (uint32_t iface)
{
  uint32_t nodeId = m_ipv4->GetObject<Node>()->GetId();
  uint16_t j = 1;
  if (nodeId== 0)
    j= 2;
  for (uint16_t i=j; i < m_queuedData->GetNInterfaces(); i++)
    {
      if (i != iface)
      {
        Ptr<PointToPointNetDevice> dev = m_ipv4->GetNetDevice(i)->GetObject<PointToPointNetDevice>();
        if (dev && dev->IsReadyTx())
	//if (dev)
          {
	    Address mac = m_ipv4->GetNetDevice(i)->GetAddress();
	    TxOpportunity(mac);
          }
      }
    }
}


//!!!QUEUETX function for PointToPointDevice (the only change with the Wifi counterpart function is the inclusion of the MAC address parameter)
void
RoutingProtocol::QueueTxPtoP (Address MAC_Address) //one interface asking to tx
{ //callback function for point-to-point link multi-radio single queue
  NS_ASSERT (m_ipv4);
  if (m_reconfState)
    { //we cannot wait until we change this flag
      return;
    }
  //create the route entry
  Ptr<Ipv4Route> rtentry;
  rtentry = Create<Ipv4Route> ();
  uint32_t numOifAddresses = m_ipv4->GetNAddresses (1);
  EntryRoutingPacketQueue anEntry;
  if (!m_queuedData->ReadEntry(anEntry))
    {
      return;
    }
  Ipv4Address dstAddr = anEntry.GetHeader().GetDestination();
  Ipv4Address srcAddr = anEntry.GetHeader().GetSource();
  Mac48Address hwAddr = anEntry.GetHwAddres();
  Ipv4InterfaceAddress ifAddr;
  if (numOifAddresses == 1) 
    {
    //assuming just one interface;
    ifAddr = m_ipv4->GetAddress (1, 0);
    }
  else 
    {
      NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing in Backpressure");
    }
  //rtentry->SetSource (ifAddr.GetLocal ());
  Ipv4Address the_nh("127.0.0.1");
  uint32_t iface;
  if ( m_variant == GREEDY_FORWARDING)
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      the_nh = m_state.FindNextHopGreedyForwardingSinglePath(ifAddr.GetLocal(), dstAddr, pos, iface);
    }
  else if ( m_variant == MP_FORWARDING)
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      the_nh = m_state.FindNextHopGreedyForwardingMultiPath(ifAddr.GetLocal(), dstAddr, pos, iface);
    }
  else // GEOBACKPRESSURE with VARIABLE_V 
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      uint32_t v;
      uint8_t ttl = anEntry.GetHeader().GetTtl(); 
      VcalcTimerExpire ();
      if ( (m_vpolicy==VARIABLE_V_SINGLE_RADIO) || (m_vpolicy==VARIABLE_V_MULTI_RADIO) )
        {
          if ( ((ttl)<40) )
            {
              float increment=((float)(63-anEntry.GetHeader().GetTtl())/(63))*(float)(m_queuedData->GetMaxSize()-m_weightV);
              if ( (m_weightV+(int)increment)<(m_queuedData->GetMaxSize()) )
                {
                  v = m_weightV+(int)increment;
                }
              else 
                {
                  v = m_queuedData->GetMaxSize();
                }
            }
          else
            {
              v = m_weightV;
            }
        }
      else 
        {
          v = m_weightV;
        }
    if (m_variant == BACKPRESSURE_CENTRALIZED_PACKET || m_variant == BACKPRESSURE_CENTRALIZED_FLOW) 
      //for any topology calculate queue backlogs on a centralized way
      {	
	  Ptr<Packet> currentPacket = anEntry.GetPacket()->Copy();
	  Ipv4Header tmp_header = anEntry.GetHeader();
          uint16_t s_port=999, d_port=999;
          GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);
	  bool SatFlow = false;
          //initial simple traffic classification
	  /*if (d_port%2 ==0 && LocationList::m_SatGws.size()>0)
	    SatFlow = true;*/
	  	  
	  if (LocationList::m_SatGws.size()>0)
	    {
	      //there are Satellite gws in the network
	      uint16_t ref_port=1; //by default through the terrestrial
	      if (d_port > 10000)
	        {
		  ref_port = s_port;
	        }
	      else
		{
		  ref_port = d_port;
		}
		
	      /*if (ref_port%2==0)
	        {
		  SatFlow = true;
	        }*/
	      bool reset = LocationList::GetResetLocationList();
	      //bool reset = true;
	      if (!reset)
	      {
	         if (ref_port%2==0)
	          {
		    SatFlow = true;
	          }
	      }
	      else
	      { //m_reset true, odd ports go thorough satellite
		if (ref_port%2!=0)
		  SatFlow = true;
	      }
	      
	    }
	  
	  bool erased =false; //this is flag in case
	  Ipv4Address prev_nh("127.0.0.1");
	  if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	  {
	    the_nh = m_state.FindNextHopInFlowTable(m_ipv4, srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, iface, erased, prev_nh, tmp_header.GetTtl());
	    //check that the interface is available: if not, force to find another path
	    if (!the_nh.IsEqual("127.0.0.1") && !m_ipv4->IsUp(iface) && !erased)
	      {
	        //we erase the rule if the selected iface not available. 
	        the_nh.Set("127.0.0.1");
	        m_state.EraseFlowTableEntry(srcAddr, dstAddr, s_port, d_port);
	      }
	  }
	      
	  //NS_LOG_UNCOND(" sport:"<<s_port<<" dport:"<<d_port<<" type:"<<(int)header.GetProtocol());
          if (the_nh.IsEqual("127.0.0.1"))
            {
              //paper BP-SDN
	      //the_nh = m_state.FindNextHopBackpressureCentralized(m_variant, ifAddr.GetLocal(), srcAddr, dstAddr, ttl, m_queuedData->GetSize(), m_ipv4->GetObject<Node> ()->GetId(), pos, v, iface, hwAddr, m_nodesRow, m_ipv4);
	      //sansa
	      //the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSAv2(ifAddr.GetLocal(), srcAddr, dstAddr, m_ipv4->GetObject<Node>()->GetId(), v, iface, m_ipv4, m_nodesRow, hwAddr, erased, prev_nh);
	      //sansa-lena
	      if (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2") )
	        {
		   the_nh = dstAddr;
		   uint8_t buf[4];
		   the_nh.Serialize(buf);
		   buf[3] = buf[3]-1;
		   Ipv4Address tmp_addr= Ipv4Address::Deserialize(buf);
		   iface = m_ipv4->GetInterfaceForAddress(tmp_addr);
	         }
	      else
	        {
		  if (LocationList::m_SatGws.size()>0)
		    {
		      Ptr<Node> node = NodeList::GetNode(0);
		      Ipv4Address tmp_addr = node->GetObject<Ipv4>()->GetAddress(3,0).GetLocal();
		      if (dstAddr.IsEqual(tmp_addr))
		        {
			  //std::cout<<"UL TRAFFIC callback!!"<<std::endl;
			  if (m_ipv4->GetObject<Node>()->GetId() == (NodeList::GetNNodes()-1) || !(SatFlow) )
		             { //we are in the node that connects the satellite link with the EPC (the extraEnodeB)
		               the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		             }
		           else
		             { //this is a satellite flow and we are in the terrestrial backhaul. We have to redirect the flow to a sat gw
		               Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
		               Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
		               Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		               //std::cout<<"La ip address del sat Node es: "<<dstAddr<<std::endl;
		               the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		             }
			}
		      else
		        {
			    //std::cout<<"DL TRAFFIC callback!!"<<std::endl;
			    if (m_ipv4->GetObject<Node>()->GetId() == 0 && SatFlow)
			      { //this case is to force that a SatFlow goes through the satellite 
			        Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
			        Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
			        Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		                the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      }
			    else
			      {
				the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      }  
		        }
		    }
		  else
		    {
		      //there are no satellite gw's.
		      the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		    }
		 }       
	       
	      if (!the_nh.IsEqual("127.0.0.1") && m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	        { //the value of iface will only be 0 when returning loopback
		  m_state.UpdateFlowTableEntry(srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, the_nh, iface,ttl);
		  if (m_ipv4->GetObject<Node>()->GetId() ==0)
		   std::cout<<"the nexthop en EPC debido al callback "<<m_ipv4->GetObject<Node>()->GetId()<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time: "<<Simulator::Now()<<" y destination port: "<<d_port<<std::endl;
		 else
		   std::cout<<"the nexthop en eNodeB debido al callback "<<m_ipv4->GetObject<Node>()->GetId()-1<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time "<<Simulator::Now()<<" y destination port: "<<d_port<<std::endl;
	        }	       
             }
      }
    else 
      {						    //trusting on HELLO messages for getting queue backlog information
        the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSA(ifAddr.GetLocal(), srcAddr, dstAddr, m_ipv4->GetObject<Node>()->GetId(), v, iface, m_ipv4, m_nodesRow, hwAddr);
	//paper BP-SDN
        //the_nh = m_state.FindNextHopBackpressureDistributed(m_variant, ifAddr.GetLocal(), srcAddr, dstAddr, anEntry.GetHeader().GetTtl(), m_queuedData->GetSize(), m_ipv4->GetObject<Node> ()->GetId(), pos, v, iface, hwAddr, m_nodesRow);
      }  
    }
  if (!the_nh.IsEqual("127.0.0.1") && m_ipv4->IsUp(iface))
    {
      rtentry->SetGateway (the_nh);
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
      rtentry->SetSource (oifAddr.GetLocal ());
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
      SendQueuedData(rtentry);
    }
}

//!!!QUEUETX function for WifiNetDevice
void
RoutingProtocol::QueueTx()
{ //callback function for wifi net device no multi-queue
  NS_ASSERT (m_ipv4);
  //create the route entry
  Ptr<Ipv4Route> rtentry;
  rtentry = Create<Ipv4Route> ();
  uint32_t numOifAddresses = m_ipv4->GetNAddresses (1);
  EntryRoutingPacketQueue anEntry;
  if (!m_queuedData->ReadEntry(anEntry))
    {
      return;
    }
  Ipv4Address dstAddr = anEntry.GetHeader().GetDestination();
  Ipv4Address srcAddr = anEntry.GetHeader().GetSource();
  Mac48Address hwAddr = anEntry.GetHwAddres();
  Ipv4InterfaceAddress ifAddr;
  if (numOifAddresses == 1) 
    {
    //assuming just one interface;
    ifAddr = m_ipv4->GetAddress (1, 0);
    }
  else 
    {
      NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing in Backpressure");
    }
  
  Ipv4Address the_nh("127.0.0.1");
  uint32_t iface;
  if ( m_variant == GREEDY_FORWARDING)
    {
      
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      the_nh = m_state.FindNextHopGreedyForwardingSinglePath(ifAddr.GetLocal(), dstAddr, pos, iface);
      
      /*BP-SDN CODE: we did multi-radio but single queue*/
      //this code can be recover from commites on sansa sharedka previous to 14/05/15
    }
  else if ( m_variant == MP_FORWARDING)
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      the_nh = m_state.FindNextHopGreedyForwardingMultiPath(ifAddr.GetLocal(), dstAddr, pos, iface);
    }
  else // GEOBACKPRESSURE with VARIABLE_V 
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      uint32_t v;
      uint8_t ttl = anEntry.GetHeader().GetTtl();
      VcalcTimerExpire ();
      if ( (m_vpolicy==VARIABLE_V_SINGLE_RADIO) || (m_vpolicy==VARIABLE_V_MULTI_RADIO) )
        {
          if ( ((ttl)<0) )
            {
              float increment=((float)(63-anEntry.GetHeader().GetTtl())/(63))*(float)(m_queuedData->GetMaxSize()-m_weightV);
              if ( (m_weightV+(int)increment)<(m_queuedData->GetMaxSize()) )
                {
                  v = m_weightV+(int)increment;
                }
              else 
                {
                  v = m_queuedData->GetMaxSize();
                }
            }
          else
            {
              v = m_weightV;
            }
        }
      else 
        {
          v = m_weightV;
        }
    if (m_variant == BACKPRESSURE_CENTRALIZED_PACKET || m_variant == BACKPRESSURE_CENTRALIZED_FLOW) //for any topology calculate queue backlogs on a centralized way
      {	
	  Ptr<Packet> currentPacket = anEntry.GetPacket()->Copy();
	  Ipv4Header tmp_header= anEntry.GetHeader();
          uint16_t s_port=999, d_port=999;
	  GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);
	  
	  //if it is present in the flow table or flow table entry expired then compute weights
          the_nh = m_state.FindNextHopInFlowTable(srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, iface);
	  //NS_LOG_UNCOND(" sport:"<<s_port<<" dport:"<<d_port<<" type:"<<(int)header.GetProtocol());
          if (the_nh.IsEqual("127.0.0.1"))
            {
              the_nh = m_state.FindNextHopBackpressureCentralized(m_variant, ifAddr.GetLocal(), srcAddr, dstAddr, anEntry.GetHeader().GetTtl(), m_queuedData->GetSize(), m_ipv4->GetObject<Node> ()->GetId(), pos, v, iface, hwAddr, m_nodesRow, m_ipv4);
	      m_state.UpdateFlowTableEntry(srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, the_nh, iface, ttl);
            }
      }
    else 
      {						    //trusting on HELLO messages for getting queue backlog information
        the_nh = m_state.FindNextHopBackpressureDistributed(m_variant, ifAddr.GetLocal(), srcAddr, dstAddr, anEntry.GetHeader().GetTtl(), m_queuedData->GetSize(), m_ipv4->GetObject<Node> ()->GetId(), pos, v, iface, hwAddr, m_nodesRow);
      }  
    }
  if (!the_nh.IsEqual("127.0.0.1"))
    {
      rtentry->SetGateway (the_nh);
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
      rtentry->SetSource (oifAddr.GetLocal ());
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
      SendQueuedData(rtentry);
    }
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  //NS_LOG_UNCOND ("Created backpressure routing protocol");
  m_helloTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
  //assign the function to change the flag after a reconfiguration
  m_reconfTimer.SetFunction (&RoutingProtocol::ReconfTimerExpire, this);
  if (m_queuePolicy == FIFO_RQUEUE)
     { // First In First Out
       NS_LOG_DEBUG("FIFO routing policy");
       m_queuedData = CreateObject<FifoRoutingPacketQueue> ();
     }
   else if (m_queuePolicy == LIFO_RQUEUE)
     { // Last In First Out
       NS_LOG_UNCOND("LIFO routing policy");
       m_queuedData = CreateObject<LifoRoutingPacketQueue> ();
     }
   else if (m_queuePolicy == EDF_RQUEUE)
     { // Earliest Deadline First
       NS_LOG_UNCOND("EDF routing policy and V is "<<(int)m_weightV);
       m_queuedData = CreateObject<PriorityRoutingPacketQueue> ();
     }
   else if (m_queuePolicy == RING_RQUEUE)
    { // Ring queue discipline
       //NS_LOG_UNCOND("Ring routing policy");
       m_queuedData = CreateObject<RingRoutingPacketQueue> ();
    }
   else
    { // GRID_RQUEUE: Grid FIFO queue discipline: separate queues for each iface
      //NS_LOG_UNCOND("Grid routing policy");
      m_queuedData = CreateObject<GridRoutingPacketQueue> ();
    }
       
  //m_requeueing = 0;
  m_requeue_loop = false;
  
  //initially we do not need to wait for a reconfiguration
  m_reconfState = false;
  
  m_queuedMessagesTimer.SetFunction (&RoutingProtocol::SendQueuedMessages, this);
  m_packetSequenceNumber = BACKPRESSURE_MAX_SEQ_NUM;
  m_messageSequenceNumber = BACKPRESSURE_MAX_SEQ_NUM;

  m_linkTupleTimerFirstTime = true;

  m_ipv4 = ipv4;
  
  NS_LOG_DEBUG("Max Queue Length:"<<m_maxQueueLen);
  if (m_ipv4->GetObject<Node>()->GetId() == 0)
    {
      m_maxQueueLen = (m_maxQueueLen*3)/2;
    }
  m_queuedData->SetMaxSize(m_maxQueueLen);
  
  
  m_queueAvg(m_queuedData->GetSize());
  //m_queueSizes.push_back(m_queuedData->GetSize());
  m_avgqueueTimer.SetFunction(&RoutingProtocol::AvgQueueTimerExpire, this);
  m_avgqueueTimer.Schedule (m_queueAvgInterval);
  
  //for transmitting packets
  // Create lo route. It is asserted that the only one interface up for now is loopback
  //NS_ASSERT (m_ipv4->GetNInterfaces () == 1 && m_ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
  //we remove this assert due to the inclusion of the backpressure agent in the EPC node
  m_lo = m_ipv4->GetNetDevice (0);
  NS_ASSERT (m_lo != 0);

  //jnunez assign routing metric to the Routing Protocol
  //now the routing metric is statically mapped to the etx-metric
  //effectively EtxMetric is in charge of SendingHellos
  NS_LOG_DEBUG ("Created object etx metric");
  //COMMENTING CODE associated to link Metric
  //m_rtMetric = CreateObject <EtxMetric> ();
  //m_rtMetric->SetWirelessIpv4RoutingProtocol(this);
  //m_rtMetric->SetHelloInterval(m_helloInterval);
  //m_rtMetric->SetIpv4(m_ipv4);
  //m_rtMetric->Start();
  //END COMMENTED CODE
}

void RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;
  for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator iter = m_socketAddresses.begin ();
       iter != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();

  WirelessIpv4RoutingProtocol::DoDispose ();
}
void RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const
{
//TODO Print Routing Table (i.e., Neighbor Table)
}

int RoutingProtocol::GetMacQueueSize(Ptr<NetDevice> aDev)
{
  Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
  if (aWifiDev!=NULL)
    {
      Ptr<WifiMac> aMac = aWifiDev->GetMac();
      if (aMac!=NULL)
        {
	  Ptr<RegularWifiMac> aAdhocMac = aMac->GetObject<RegularWifiMac> ();
	  if (aAdhocMac!=NULL)
	    {
	      #ifdef MYWIFIQUEUE
	      Ptr<DcaTxop> aDca = aAdhocMac->GetDcaTxop();
	      return aDca->GetQueueSize();
              #else
              return 0;
              #endif
	    }
          else
            {
	      NS_FATAL_ERROR("RegularWifiMac without WifiMac");
            }
	}
      else
	{
	  NS_FATAL_ERROR("WifiNetDevice without WifiMac");
	}
    }
  else //E-BAND
    { 
      Ptr<PointToPointNetDevice> aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
      if (aPtopDev!=NULL)
        {
          Ptr<Queue> aQueue = aPtopDev->GetQueue ();
          if (aQueue!=NULL)
            {
	      return aQueue->GetNPackets();
            }
          else
            {
   	      NS_FATAL_ERROR("PointtoPointNetDevice without queue");
            }
        }
      else
        {
	  NS_FATAL_ERROR("Unknown L2 device");
        }
    }
}

Ptr<WifiNetDevice> RoutingProtocol::IsWifi(Ptr<NetDevice> dev, uint32_t i)
{
  Ptr<WifiNetDevice> aWifiDev = dev->GetObject<WifiNetDevice> ();
  if (aWifiDev == 0)
    {
      NS_LOG_DEBUG("WiFi "<<i<<" does not exist."<<" Try another device type.");
      return NULL;
    }
  return aWifiDev;
}

Ptr<WifiMacQueue> RoutingProtocol::GetWifiQueue(Ptr<NetDevice> dev, uint32_t i)
{
  #ifdef MYWIFIQUEUE
  Ptr<WifiNetDevice> aWifiDev = dev->GetObject<WifiNetDevice> ();
  if (aWifiDev == 0)
    {
      NS_LOG_DEBUG("WiFi "<<i<<" does not exist."<<" Try another device type.");
      return 0;
    }
  Address mac_queue = aWifiDev->GetAddress();
  Ptr<WifiMac> aMac = aWifiDev->GetMac();
  if (aMac == 0)
    {
      NS_FATAL_ERROR("WiFiNetDevice present but no WiFiMAC");
    }
  Ptr<RegularWifiMac> aAdhocMac = aMac->GetObject<RegularWifiMac> ();
  if (aAdhocMac == 0)
    {
      NS_FATAL_ERROR("WiFiNetDevice present, WiFiMAC present but not in adhoc mode");
    }
  Ptr<DcaTxop> aDca = aAdhocMac->GetDcaTxop();
  if (aDca == 0)
    {
      NS_FATAL_ERROR("WiFiNetDevice present, WiFiMAC present in adhoc mode, but not DcaTxop");
    }
  Ptr<WifiMacQueue> aWifiQueue = aDca->GetQueue();
  if (aWifiQueue == 0)
    {
      NS_FATAL_ERROR("WiFiNetDevice present, WiFiMAC present in adhoc mode, DcaTxop present but no WiFiMacQueue");
    }
   aWifiQueue->SetMacAddress(mac_queue);
  return aWifiQueue; 
  #endif
  return NULL;
}

void RoutingProtocol::DoInitialize ()
{
  /*std::cout<<"Info interfaces at node: "<<m_ipv4->GetObject<Node>()->GetId()<<std::endl;
  for (std::multimap<int, int>::iterator it = m_infoInterfaces.begin(); it!=m_infoInterfaces.end();++it)
    {
      std::cout<<(*it).first<<" "<<(*it).second<<std::endl;
    }*/
    
  std::map<Ipv4Address,uint32_t>::iterator it;
  it = LocationList::m_SatGws.find(m_ipv4->GetAddress (1, 0).GetLocal ());
  if (it != LocationList::m_SatGws.end())
    {
      m_state.SetSatNodeValue(true);      
    }
  else
    { 
      m_state.SetSatNodeValue(false);
    }
  
  it = LocationList::m_Gws.find(m_ipv4->GetAddress (1,0).GetLocal());
  if (it != LocationList::m_Gws.end())
    {
      m_state.SetTerrGwNodeValue (true);
    }
  else
    {
      m_state.SetTerrGwNodeValue (false);
    }
  
  m_arrays=0;
  //SANSA multiradio: according to the assumption done in the design of multiradio
  //each node will have a queue per each interface, we are also counting the loopback
  //interface to avoid problems with indexing. So you will never be able to insert packets
  //in the 0 queue. Be careful with the EPC node where iface0: loopback, iface1:LTE,
  //iface2:link with remote host and iface3...: the backhauling ones
  uint32_t interfaces;
#ifdef GRID_MULTIRADIO_MULTIQUEUE
  interfaces = m_ipv4->GetNInterfaces();
#else
  interfaces = m_ipv4->GetNInterfaces()-1;
#endif
  
  m_queuedData->SetInterfaces(interfaces);
  for (uint32_t i = 0; i < interfaces; i++)
    m_histeresis.push_back(false);
  // 9/10/15: m_arrays is a variable we do not use
  for (uint32_t i = 1; i < m_ipv4->GetNInterfaces(); i++)
    {
      if (m_ipv4->IsUp(i))
        m_arrays++;
    }
  //std::cout<<"Node "<<m_ipv4->GetObject<Node>()->GetId()<<" has "<<m_arrays<<" arrays"<<std::endl;
#ifndef GRID_MULTIRADIO_MULTIQUEUE
  m_queuedData->SetMaxSize(m_maxQueueLen*interfaces);
  //test multiradio wifi-SI-ad-hoc (SI:special issue). In SANSA, only one queue per node 
  //  m_queuedData->SetMaxSize(m_maxQueueLen);
#endif
     
  if (m_mainAddress == Ipv4Address ())
    {
      Ipv4Address loopback ("127.0.0.1");
      for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {
          // Use primary address, if multiple
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
	  //for the EPC, we need the mainAddr to be the address of the P2P link
	  //comment these two lines to make sansa-grid work
	  if (m_ipv4->GetObject<Node>()->GetId()==0 && (i==1 || i==2))
	    continue;
          if (addr != loopback)
            {
              m_mainAddress = addr;
              break;
            }
        }

      NS_ASSERT (m_mainAddress != Ipv4Address ());
    }

  NS_LOG_DEBUG ("Starting Backpressure on node " << m_mainAddress);
  
  Ipv4Address loopback ("127.0.0.1");
  //SANSA + LENA
  //remoteHost address is the Ipv4address of the EPC node connecting with internet
  Ipv4Address remoteHost ("1.0.0.1");
  Ipv4Address LteEpcHost ("7.0.0.1");
  
  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
      if (addr == loopback || addr == remoteHost || addr == LteEpcHost)
      {
        continue;
      }

      if (addr != m_mainAddress)
        {
          // Create never expiring interface association tuple entries for our
          // own network interfaces, so that GetMainAddress () works to
          // translate the node's own interface addresses into the main address.
          IfaceAssocTuple tuple;
          tuple.ifaceAddr = addr;
          tuple.mainAddr = m_mainAddress;
          AddIfaceAssocTuple (tuple);
          NS_ASSERT (GetMainAddress (addr) == m_mainAddress);
        }

      if (m_interfaceExclusions.find (i) != m_interfaceExclusions.end ())
          continue;

      // Create a socket to listen only on this interface
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (), 
                                                 UdpSocketFactory::GetTypeId()); 
      socket->SetAllowBroadcast (true);
      InetSocketAddress inetAddr (m_ipv4->GetAddress (i,0).GetLocal (), BACKPRESSURE_PORT_NUMBER);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvHelloBackpressure,  this));
      if (socket->Bind (inetAddr))
        {
          NS_FATAL_ERROR ("Failed to bind() Backpressure Routing socket");
        }
      socket->Connect (InetSocketAddress (Ipv4Address (0xffffffff), BACKPRESSURE_PORT_NUMBER));
      m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);
    }

  HelloTimerExpire ();
  m_state.SetNeighExpire(BACKPRESSURE_NEIGHB_VALID);
  Ipv4Address loopback1 ("127.0.0.1");
  Ipv4Address loopback2 ("192.168.0.1");
  for (uint32_t i=1; i < (m_ipv4->GetNInterfaces ()); i++)
    {
//NS_LOG_UNCOND("node id:"<<id<<" main address: "<<m_mainAddress<<" IFACE:"<<i<<" number of interfaces:"<<m_ipv4->GetNInterfaces()<<"iface address "<<m_ipv4->GetAddress(i,0).GetLocal());
      Ipv4Address addr = m_ipv4->GetAddress (i,0).GetLocal ();
      //if ( (addr == loopback1) || (addr == loopback2) || (addr == remoteHost) )
      if ( (addr == loopback1) || (addr == loopback2) )
        {
          continue;
        }
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (i);
      if (dev == 0)
        {
         NS_FATAL_ERROR("The device does not exist");
        }
      // get WiFi Queue
      #ifdef MYWIFIQUEUE
      Ptr<WifiMacQueue> aWifiQueue = GetWifiQueue(dev,i);
      if (aWifiQueue)		//WiFi interface WiFi Queue
        {
	    #ifdef GRID_MULTIRADIO_MULTIQUEUE
	     aWifiQueue->TraceConnectWithoutContext("MacDequeueGRID", MakeCallback(&RoutingProtocol::TxOpportunity, this));
	    #else
	     aWifiQueue->TraceConnectWithoutContext("MacDequeue", MakeCallback(&RoutingProtocol::QueueTx, this));
	    #endif	     
        } 
      #else
      Ptr<WifiNetDevice> aWifiDev = IsWifi(dev,i);
      if (aWifiDev)
        {//always defined MYWIFIQUEUE
          aWifiDev->TraceConnectWithoutContext("PhyTxBegin",MakeCallback(&RoutingProtocol::QueueTx, this));
        }
      #endif
      else 			//Get E-BAND MICROWAVE
        {
	  NS_LOG_DEBUG("Trace the Mac Dequeue event  4 E-BAND");
          Ptr<PointToPointNetDevice> aPtopDev = dev->GetObject<PointToPointNetDevice> ();
          if (aPtopDev!=NULL)
            {
              
	      #ifdef GRID_MULTIRADIO_MULTIQUEUE
                aPtopDev->TraceConnectWithoutContext("Dequeue", MakeCallback(&RoutingProtocol::TxOpportunity, this));
              #else
	        aPtopDev->TraceConnectWithoutContext("Dequeue", MakeCallback(&RoutingProtocol::QueueTxPtoP, this));
              #endif
            }
          else
            {
	      if (m_ipv4->GetObject<Node>()->GetId() !=0) //epc node
                NS_FATAL_ERROR("Not a PointToPointDevice");
            }
        }
    }
  NS_LOG_DEBUG ("Backpressure Routing on node " << m_mainAddress << " started");
}

void RoutingProtocol::SetMainInterface (uint32_t interface)
{
  m_mainAddress = m_ipv4->GetAddress (interface, 0).GetLocal ();
}

void RoutingProtocol::SetInterfaceExclusions (std::set<uint32_t> exceptions)
{
  m_interfaceExclusions = exceptions;
}

// \brief Processes an incoming %Backpressure packet; right now HELLO message
void
RoutingProtocol::RecvHelloBackpressure (Ptr<Socket> socket)
{
  Ptr<Packet> receivedPacket;
  Address sourceAddress;
  receivedPacket = socket->RecvFrom (sourceAddress);

  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address senderIfaceAddr = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiverIfaceAddr = m_socketAddresses[socket].GetLocal ();
  NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
  NS_LOG_DEBUG ("BACKPRESSURE node " << m_mainAddress << " received a HELLO packet from "
                << senderIfaceAddr << " to " << receiverIfaceAddr);
  
  // All routing messages are sent from and to port RT_PORT, so we check it.
  NS_ASSERT (inetSourceAddr.GetPort () == BACKPRESSURE_PORT_NUMBER);
  
  Ptr<Packet> packet = receivedPacket;

  backpressure::PacketHeader prob_olsrPacketHeader;
  packet->RemoveHeader (prob_olsrPacketHeader);
  NS_ASSERT (prob_olsrPacketHeader.GetPacketLength () >= prob_olsrPacketHeader.GetSerializedSize ());
  uint32_t sizeLeft = prob_olsrPacketHeader.GetPacketLength () - prob_olsrPacketHeader.GetSerializedSize ();

  MessageList messages;
  
  while (sizeLeft)
    {
      MessageHeader messageHeader;
      if (packet->RemoveHeader (messageHeader) == 0)
        NS_ASSERT (false);
      
      sizeLeft -= messageHeader.GetSerializedSize ();

      NS_LOG_DEBUG ("Backpressure Msg received with type "
                << std::dec << int (messageHeader.GetMessageType ())
                << " TTL=" << int (messageHeader.GetTimeToLive ())
                << " origAddr=" << messageHeader.GetOriginatorAddress ());
      messages.push_back (messageHeader);
    }

  m_rxPacketTrace (prob_olsrPacketHeader, messages);

  for (MessageList::const_iterator messageIter = messages.begin ();
       messageIter != messages.end (); messageIter++)
    {
      const MessageHeader &messageHeader = *messageIter;
      // If ttl is less than or equal to zero, or
      // the receiver is the same as the originator,
      // the message must be silently dropped
      if (messageHeader.GetTimeToLive () == 0
          || messageHeader.GetOriginatorAddress () == m_mainAddress)
        {
          packet->RemoveAtStart (messageHeader.GetSerializedSize ()
                                 - messageHeader.GetSerializedSize ());
          continue;
        }

      // If the message has been processed it must not be processed again
      switch (messageHeader.GetMessageType ())
        {
          case backpressure::MessageHeader::HELLO_MESSAGE:
            NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                            << "s BACKPRESSURE node " << m_mainAddress
                            << " received HELLO message of size " << messageHeader.GetSerializedSize ());
            ProcessHello (messageHeader, receiverIfaceAddr, senderIfaceAddr);
            break;
          default:
            NS_LOG_DEBUG ("BACKPRESSURE message type " <<
                        int (messageHeader.GetMessageType ()) <<
                        " not implemented");
        }
    }
}


///
/// \brief Gets the main address associated with a given interface address.
///
/// \param iface_addr the interface address.
/// \return the corresponding main address.
///
Ipv4Address
RoutingProtocol::GetMainAddress (Ipv4Address iface_addr) const
{
  const IfaceAssocTuple *tuple =
    m_state.FindIfaceAssocTuple (iface_addr);
  
  if (tuple != NULL)
    return tuple->mainAddr;
  else
    return iface_addr;
}

///
/// \brief Processes a HELLO message.
///
/// Link sensing
///
/// \param msg the %PROB_OLSR message which contains the HELLO message.
/// \param receiver_iface the address of the interface where the message was received from.
/// \param sender_iface the address of the interface where the message was sent from.
///
void
RoutingProtocol::ProcessHello (const backpressure::MessageHeader &msg,
                         const Ipv4Address &receiverIface,
                         const Ipv4Address &senderIface)
{
  const backpressure::MessageHeader::Hello &hello = msg.GetHello ();

  NS_LOG_DEBUG("[RoutingProtocol::ProcessHello]:: ip iface addr receiving: "<<receiverIface<<" sender ip iface addr: "<<senderIface);
  PopulateNeighborSet (receiverIface, senderIface, msg, hello);
  
    //passing received sequence number
  //Process LINK m_rtMetric
  /*if (hello.addr == receiverIface || hello.addr==Ipv4Address("0.0.0.0"))
    {					//the metric updates is for me
      m_rtMetric->RcvPacketProbing(senderIface, msg.GetMessageSequenceNumber (), hello.pdr );
    }*/
}

///
/// \brief Enques an %HELLO_BACKPRESSURE message which will be sent with a delay of (0, delay].
///
/// This buffering system is used in order to piggyback several %HELLO_BACKPRESSURE messages in
/// a same %HELLO_BACKPRESSURE packet.
///
/// \param msg the %HELLO_BACKPRESSURE message which must be sent.
/// \param delay maximum delay the %HELLO_BACKPRESSURE message is going to be buffered.
///

void
RoutingProtocol::QueueMessage (const backpressure::MessageHeader &message, Time delay)
{
  m_queuedMessages.push_back (message);
  if (not m_queuedMessagesTimer.IsRunning ())
    {
      m_queuedMessagesTimer.SetDelay (delay);
      m_queuedMessagesTimer.Schedule ();
    }
}

///
///
///  1. Insertion of a new packet in the data queue 
///  2. Stats maintenance of the Data Queue; if the queue is full report Overflow
///  TODO: Add Trace for keeping stats of packets generated by the node itself for admission control purposes
///
void 
RoutingProtocol::QueueData (Ptr<const Packet> packet, const Ipv4Header &header, const Mac48Address &from, UnicastForwardCallback ucb)
{
  if (not m_queuedData->Enqueue(packet, header, from, ucb))
    {
      if (header.GetTtl()!=64)   //the packet has not been generated by the node
        {
          m_qOverflowTrace(1);
        }
      else
        {			//the packet has been generated by the node
	  //Stats for future Admission control algorithm
          m_qOverflowTrace(1);
        }
    }
  else
    { // Update stats on the queue backlog
      //m_qLengthTrace(m_queuedData->GetSize());
      m_qLengthTrace(m_queuedData->GetSize(), m_queuedData->GetQueuesSize(), m_queuedData->GetNInterfaces());
    }
}

//Added function QueueData
void 
RoutingProtocol::QueueData (Ptr<const Packet> packet, const Ipv4Header &header, const Mac48Address &from, UnicastForwardCallback ucb, uint32_t direction, uint32_t requeued, Time time)
{
  
  if (not m_queuedData->Enqueue(packet,header, from, ucb, direction, requeued, time))
    {
      if (header.GetTtl()!=64)   //the packet has not been generated by the node
        {
          m_qOverflowTrace(1);
        }
      else
        {			//the packet has been generated by the node
	  //Stats for future Admission control algorithm
          m_qOverflowTrace(1);
        }
    }
  else
    { // Update stats on the queue backlog
       m_qLengthTrace(m_queuedData->GetSize(), m_queuedData->GetQueuesSize(), m_queuedData->GetNInterfaces());
    }
}

/////////////////////////////////


//for hybrid (nodes with wifi and ptop links) and only single netdevice (or wifi or ptop) sendPacket function
void
RoutingProtocol::SendPacket (Ptr<Packet> packet, 
                       const MessageList &containedMessages)
{
  NS_LOG_DEBUG (" node " << m_mainAddress << " sending a BACKPRESSURE_HELLO packet");
  // Add a header
  backpressure::PacketHeader header;
  header.SetPacketLength (header.GetSerializedSize () + packet->GetSize ());
  header.SetPacketSequenceNumber (GetPacketSequenceNumber ());
  
  // Trace it
  m_txPacketTrace (header, containedMessages);

  // Send it
  int32_t i=0;
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator ia = m_socketAddresses.begin (); ia != m_socketAddresses.end (); ia++)
    {
      Ptr<Socket> socket = ia->first; //the socket 
      Ipv4InterfaceAddress iface = ia->second; //the ifaceaddress
            
      if (m_hybrid)
        { //FOR THE HYBRID SCENARIO (PtoP + wifi), this is the way to identify PtP links. 
          //The first socket is the wifi, so the rest of sockets are PtP links.
          int32_t iface_socket = m_ipv4->GetInterfaceForAddress(iface.GetLocal()); //be careful with loopback interfaces
          Ptr<Packet> packet2 = packet->Copy();
          MessageHeader msg;
          packet2->RemoveHeader(msg);
          MessageHeader::Hello &hello = msg.GetHello();
          if (iface_socket !=1) //wifi should transmit always its queue_length
	    hello.qLength_eth[iface_socket-1]=255;
          packet2->AddHeader(msg);
          packet2->AddHeader(header);
          socket->SendTo (packet2, 0, InetSocketAddress (iface.GetBroadcast (), BACKPRESSURE_PORT_NUMBER));     
	}
      else
        {
#ifndef GRID_MULTIRADIO_MULTIQUEUE
	  Ptr<Packet> packet2 = packet->Copy();
	  packet2->AddHeader(header);
	  socket->SendTo (packet2, 0, InetSocketAddress (iface.GetBroadcast (), BACKPRESSURE_PORT_NUMBER));
#else
	  Ptr<Packet> packet2 = packet->Copy();
	  MessageHeader msg;
          packet2->RemoveHeader(msg);
          MessageHeader::Hello &hello = msg.GetHello();
	  hello.qLength_eth[i] = 255;
	  packet2->AddHeader(msg);
	  packet2->AddHeader(header);
	  socket->SendTo (packet2, 0, InetSocketAddress (iface.GetBroadcast (), BACKPRESSURE_PORT_NUMBER));
#endif	  
	  /*in the future... #ifndef GRID_MULTIRADIO_MULTIQUEUE
	  socket->SendTo (packet2, 0, InetSocketAddress (iface.GetBroadcast (), BACKPRESSURE_PORT_NUMBER));
	  #else
	  //grid_multiradio_multiqueue: tmb poner a 255 la propia interficie para indicar que es mi origen
	  //sockets are created only on backhaul interfaces 
	  #endif*/
        }     
        i++;
    }
}


///
/// \brief Creates as many %BACKPRESSURE_HELLO packets as needed in order to send all buffered
/// %BACKPRESSURE_HELLO messages.
///
/// Maximum number of messages which can be contained in an %BACKPRESSURE_HELLO packet is
/// dictated by BACKPRESSURE_MAX_MSGS constant.
///

void
RoutingProtocol::SendQueuedMessages ()
{
  Ptr<Packet> packet = Create<Packet> ();
  int numMessages = 0;

  NS_LOG_DEBUG ("node " << m_mainAddress << ": SendQueuedMessages");
  
  MessageList msglist;
  
  for (std::vector<backpressure::MessageHeader>::const_iterator message = m_queuedMessages.begin ();
       message != m_queuedMessages.end ();
       message++)
    {
      
      Ptr<Packet> p = Create<Packet> ();
      p->AddHeader (*message);
      packet->AddAtEnd (p);
      msglist.push_back (*message);
      
      if (++numMessages == BACKPRESSURE_MAX_MSGS)
        {
          SendPacket (packet, msglist);
          msglist.clear ();
          // Reset variables for next packet
          numMessages = 0;
          packet = Create<Packet> ();
        }
    }
        
  if (packet->GetSize ())
    {
      Ptr<NetDevice> aDev = m_ipv4->GetNetDevice (1);//this is the output device
      if (aDev==0)
        {
           SendPacket(packet, msglist);//emulation mode
	   NS_LOG_DEBUG("aDev does not exist");
        }
      else
        {
          Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
          if (aWifiDev!=0)
            {
              Ptr<WifiMac> aMac = aWifiDev->GetMac();
              if (aMac!=0)
                {
                  Ptr<RegularWifiMac> aAdhocMac = aMac->GetObject<RegularWifiMac> ();
                  Ptr<DcaTxop> aDca = aAdhocMac->GetDcaTxop();
                  #ifdef MYWIFIQUEUE
                  bool serviced = (aDca->GetQueueSize() < 10);
                  //bool serviced = true;
                  #else //TODO
                  bool serviced = true;
                  #endif
                  if (serviced)
                  SendPacket (packet, msglist);
                }
              else
                {
                  SendPacket (packet, msglist);
                }
            }
          else
            SendPacket (packet, msglist);
        }
    }
   m_queuedMessages.clear ();
}

///
/// send Data in the Queues
///
void
RoutingProtocol::SendQueuedData (Ptr<Ipv4Route> theRoute)
{
  Ptr<Packet> dataPacket;
  UnicastForwardCallback qucb;
  Ptr<const Packet> qPacket;
  Ipv4Header qHeader;
  EntryRoutingPacketQueue entry;
  bool serviced;

  //In multi-radio
  Ptr<NetDevice> aDev = theRoute->GetOutputDevice ();
  Ptr<PointToPointNetDevice> aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
  Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
  
  int64_t rate_bps;
  int32_t mode; //1 wifi, 2 ppp
  if (aWifiDev == NULL)
  { //ppp device
    rate_bps = aPtopDev->GetRatebps();
    mode = 2;
  }
  else
  { //wifi device
    rate_bps = aWifiDev->GetRatebps();
    mode = 1;
  }

  serviced = (GetMacQueueSize(aDev) < 2);
  if (serviced)
    {
#ifdef GRID_MULTIRADIO_MULTIQUEUE
      uint32_t direction;
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      direction= (uint32_t) m_ipv4->GetInterfaceForDevice(aDev);
      bool canItx = m_queuedData->Dequeue(entry, direction);
#else
      bool canItx = m_queuedData->Dequeue(entry);
#endif
      if (canItx)
        {
          //NS_LOG_UNCOND("[SendQueuedData]: el desti es "<<qHeader.GetDestination());
          qHeader=entry.GetHeader();
          qucb=entry.GetUcb();
          qPacket=entry.GetPacket(); 
          //NS_LOG_DEBUG("[SendQueuedData]: the packet size is "<<qPacket->GetSize());
          //NS_LOG_DEBUG("gateway is "<<theRoute->GetGateway()<<"  header src "<<theRoute->GetSource()<<"  and header dst "<<theRoute->GetDestination());
          //NS_LOG_DEBUG("[SendQueuedData]: unicast callback");
	  m_TxData(qHeader, qPacket, aDev->GetIfIndex ()); 
	  m_TxDataAir(qPacket, qHeader,rate_bps, mode);	  
          theRoute->SetSource(qHeader.GetSource());
          theRoute->SetDestination(qHeader.GetDestination());
          qucb (theRoute, qPacket, qHeader);
		  ///checking the histeresis
          if (m_histeresis[direction])
            {
              if (m_queuedData->GetIfaceSize(direction) < (5/10)*(m_queuedData->GetMaxSize()))
                {
                    m_histeresis[direction] = false;
                }
            }
        }
    }
  else
    {
      NS_LOG_DEBUG("MAC queue loaded size: ");
    }
}

///
/// \brief Creates a new %PROB_OLSR HELLO message which is buffered for being sent later on.
///
void
RoutingProtocol::SendHello ()
{
  NS_LOG_FUNCTION (this);
  m_qLengthTrace(m_queuedData->GetSize(), m_queuedData->GetQueuesSize(), m_queuedData->GetNInterfaces());
  backpressure::MessageHeader msg;
  msg.SetVTime (BACKPRESSURE_NEIGHB_HOLD_TIME);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (1);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
  backpressure::MessageHeader::Hello &hello = msg.GetHello ();
  hello.SetHTime (m_helloInterval);
  hello.willingness = m_willingness;
  hello.qLength = m_queuedData->GetSize();
  Ipv4Address addr;
  //COMMENTED m_rtMetric
  //hello.pdr = m_rtMetric->GetMetric4Hello(addr);//for simplicity in the serialize as DiPUMP does not use PDR
  NS_LOG_DEBUG("[EtxMetric::SendHello] Metric for hellos is "<<addr);
  hello.addr = addr;
  Ptr<Node> node = m_ipv4->GetObject<Node> ();
  Vector pos = node->GetObject<MobilityModel>()->GetPosition();
  hello.x= pos.x;
  hello.y= pos.y;
  //this variable should account the backhauling interfaces, which should be the size
  //of m_infoInterfaces variable
  //previous code: hello.n_interfaces = (uint8_t)m_queuedData->GetNInterfaces();
  hello.n_interfaces = (uint8_t)m_infoInterfaces.size();
  for (std::map<int,int>::iterator it=m_infoInterfaces.begin(); it!=m_infoInterfaces.end(); ++it)
    {
      if (m_ipv4->IsUp(it->first)) 
        {
	  //the interface is up
	  hello.qLength_eth.push_back((uint8_t)m_queuedData->GetIfaceSize(it->first));
        }
      else
        {
	  //the interface is down 
	  hello.qLength_eth.push_back(255);
        }
    }
  QueueMessage (msg, JITTER);
}

void
RoutingProtocol::PopulateNeighborSet (const Ipv4Address &receiverIface,
				      const Ipv4Address &senderIface,
                                      const backpressure::MessageHeader &msg,
                                      const backpressure::MessageHeader::Hello &hello)
{
  NeighborTuple *nb_tuple = m_state.FindNeighborTuple (senderIface);
  Time now = Simulator::Now ();
  //std::cout<<"recibo hello en nodo "<<m_ipv4->GetObject<Node>()->GetId()<<" ips: "<< receiverIface<<" "<<senderIface<<std::endl;
  if (nb_tuple != NULL)
    { // Update information of the neighbor
      nb_tuple->willingness = hello.willingness;
      nb_tuple->qPrev = now;
      nb_tuple->qLength = hello.qLength;
      nb_tuple->lastHello = now;
      //check if this link has been recovered, so we erase the flow table and recalculate the flow table
      m_state.CheckTerrestrialNeighbor (nb_tuple, now);
      nb_tuple->x = hello.x;
      nb_tuple->y = hello.y;     
      for (uint32_t i = 0; i< hello.n_interfaces; i++)
	nb_tuple->qLength_eth[i] = hello.qLength_eth[i];
    }
  else
    { // A new neighbor added to the list
      NeighborTuple anb_tuple;
      anb_tuple.neighborMainAddr = senderIface;	// neighbor ip interface
      anb_tuple.neighborhwAddr = m_state.GetHwAddress(msg.GetOriginatorAddress()); // hw Address
      NS_LOG_DEBUG("Mac address for neighbor "<< msg.GetOriginatorAddress() <<" is :"<< anb_tuple.neighborhwAddr);
      anb_tuple.theMainAddr =  msg.GetOriginatorAddress (); // main address for all interfaces in a node
      anb_tuple.localIfaceAddr = receiverIface;		    // interface receiving packets
      // interface
      for (uint32_t i = 1; i < m_ipv4->GetNInterfaces (); i++)
        {
          if (m_ipv4->GetAddress (i,0).GetLocal () == receiverIface)
            {
              anb_tuple.interface = i;
      	      AddCache(i);				//Needed for ARP issues
            }
        }
      anb_tuple.neighborhwAddr = m_state.GetHwAddress(senderIface); //hw Address of the neighbor
      anb_tuple.willingness = hello.willingness;		// willingness (inherited from OLSR)
      anb_tuple.qLength = hello.qLength;			// current queue backlog
      anb_tuple.qLengthPrev = 0;				// previous queue backlog
      anb_tuple.qLengthSchedPrev = 0;				// the queue length to be previous queue backlog
      anb_tuple.qPrev = now;					// timestamp of qLengthPrev update
      anb_tuple.lastHello = now;				// last received hello
      anb_tuple.x = hello.x;			   		// neighbor coordinates (X,Y) 
      anb_tuple.y = hello.y;				
      anb_tuple.Id = LocationList::GetNodeId(senderIface); 	// Get Node Identifier
      /*Ptr<Node> n = NodeList::GetNode(anb_tuple.Id);
      Ptr <Ipv4> ipv4 = n->GetObject<Ipv4> ();    
      for (uint32_t j=1; j<ipv4->GetNInterfaces (); j++)
        {
          if (ipv4->GetAddress(j,0).GetLocal() == senderIface)
              ipv4->SetDown(j);
        }
      */
      anb_tuple.n_interfaces = hello.n_interfaces; 		// number of interfaces of the neighbor
      for (uint32_t i = 0; i< hello.n_interfaces; i++)
	  anb_tuple.qLength_eth.push_back(hello.qLength_eth[i]); // list of queue backlogs needed?
      AddNeighborTuple(anb_tuple);
    }
}

///
/// \brief Adds a neighbor tuple to the Neighbor Set.
///
/// \param tuple the neighbor tuple to be added.
///
void
RoutingProtocol::AddNeighborTuple (const NeighborTuple &tuple)
{

  m_state.InsertNeighborTuple (tuple);
}

///
/// \brief Removes a neighbor tuple from the Neighbor Set.
///
/// \param tuple the neighbor tuple to be removed.
///
void
RoutingProtocol::RemoveNeighborTuple (const NeighborTuple &tuple)
{
  m_state.EraseNeighborTuple (tuple);
}


///
/// \brief Adds an interface association tuple to the Interface Association Set.
///
/// \param tuple the interface association tuple to be added.
///
void
RoutingProtocol::AddIfaceAssocTuple (const IfaceAssocTuple &tuple)
{
//   debug("%f: Node %d adds iface association tuple: main_addr = %d iface_addr = %d\n",
//         Simulator::Now (),
//         PROB_OLSR::node_id(ra_addr()),
//         PROB_OLSR::node_id(tuple->main_addr()),
//         PROB_OLSR::node_id(tuple->iface_addr()));

  m_state.InsertIfaceAssocTuple (tuple);
}

///
/// \brief Removes an interface association tuple from the Interface Association Set.
///
/// \param tuple the interface association tuple to be removed.
///
void
RoutingProtocol::RemoveIfaceAssocTuple (const IfaceAssocTuple &tuple)
{
//   debug("%f: Node %d removes iface association tuple: main_addr = %d iface_addr = %d\n",
//         Simulator::Now (),
//         PROB_OLSR::node_id(ra_addr()),
//         PROB_OLSR::node_id(tuple->main_addr()),
//         PROB_OLSR::node_id(tuple->iface_addr()));

  m_state.EraseIfaceAssocTuple (tuple);
}


uint16_t RoutingProtocol::GetPacketSequenceNumber ()
{
  m_packetSequenceNumber = (m_packetSequenceNumber + 1) % (BACKPRESSURE_MAX_SEQ_NUM + 1);
  return m_packetSequenceNumber;
}

/// Increments message sequence number and returns the new value.
uint16_t RoutingProtocol::GetMessageSequenceNumber ()
{
  m_messageSequenceNumber = (m_messageSequenceNumber + 1) % (BACKPRESSURE_MAX_SEQ_NUM + 1);
  return m_messageSequenceNumber;
}


///
/// \brief Sends a HELLO message and reschedules the HELLO timer.
/// \param e The event which has expired.
///
void
RoutingProtocol::HelloTimerExpire ()
{
  SendHello ();
  m_helloTimer.Schedule (m_helloInterval);
}

//This function recalculates the avg occupancy of the queue when it has enough components
void
RoutingProtocol::AvgQueueTimerExpire ()
{
  m_queueSizes.push_back(m_queuedData->GetSize());
  if (m_queueSizes.size() ==10)
  {
    uint32_t tmp =0;
    for (uint16_t i=0; i<m_queueSizes.size(); i++)
      {
	tmp = tmp + m_queueSizes[i];
      }
    tmp = tmp/m_queueSizes.size();
    m_queueAvg(tmp);
    m_queueSizes.clear();
  }
  m_avgqueueTimer.Schedule (m_queueAvgInterval);
}

//
// Recalculates the V parameters based on local congestion around the 1-hop neighborhood
//
void
RoutingProtocol::VcalcTimerExpire ()
{
  uint32_t Qth = m_queuedData->GetMaxSize();
  //uint32_t Qth = 200;
  if (m_vpolicy!=FIXED_V)
   {
     uint32_t id = m_ipv4->GetObject<Node> ()->GetId();
     int32_t neighQlen;
     if (m_vpolicy==VARIABLE_V_SINGLE_RADIO)
      {
        neighQlen = m_state.FindNeighborMaxQueue(id);
        //neighQlen = 0;
      }
    else //multiradio
      {
        //neighQlen = 0; //because the medium is full duplex
        neighQlen = m_state.FindNeighborMaxQueue(id);
      }
    int32_t qlimit4greedy;
    if ((int)(m_queuedData->GetSize())>neighQlen)
      {
        if (m_queuedData->GetMaxSize()>0)
          {
            //qlimit4greedy = (int)m_queuedData->GetMaxSize()-(int)m_queuedData->GetSize();
            qlimit4greedy = (int)Qth-(int)m_queuedData->GetSize();
          }
        else
          {//Infinite queues
            qlimit4greedy = (int)Qth-(int)m_queuedData->GetSize();
          }
      }
    else
      {
        if (m_queuedData->GetMaxSize()>0)
          {
            //qlimit4greedy = (int)m_queuedData->GetMaxSize()-(int)neighQlen;
            qlimit4greedy = (int)Qth-(int)neighQlen;
          }
        else
          {//Infinite queues
            qlimit4greedy = (int)Qth-(int)neighQlen;
          }
      }
    if (qlimit4greedy<0)
      {
        m_weightV=0;
      }
    else 
      {
        m_weightV=qlimit4greedy;
      }
    }
  m_vTrace(m_weightV);
}

//Other implementation of VcalcTimerExpire for grid multiradio

void
RoutingProtocol::VcalcTimerExpire (uint32_t direction, bool loop)
{
  if (m_vpolicy!=FIXED_V)
 {
  
  uint32_t Qth = m_queuedData->GetMaxSize();
  uint32_t id = m_ipv4->GetObject<Node> ()->GetId();
  int32_t neighQlen;

  if (m_vpolicy==VARIABLE_V_SINGLE_RADIO)
    {
      neighQlen = m_state.FindNeighborMaxQueue(id);
    }
  else //multiradio
    {
      //neighQlen = 0; //because the medium is full duplex
      neighQlen = m_state.FindNeighborMaxQueueInterface(id);
    }
  int32_t qlimit4greedy;
  uint32_t data_size=0;
  
#ifdef GRID_MULTIRADIO_MULTIQUEUE
  uint32_t j=1;
  if (m_ipv4->GetObject<Node>()->GetId() == 0)
    j=2;
#else
   uint32_t j=0;
#endif
  //to avoid extra checking
  for (uint32_t i=j; i<m_queuedData->GetNInterfaces(); i++)
    {
      if (m_queuedData->GetIfaceSize(i) > data_size)
	  data_size = m_queuedData->GetIfaceSize(i);
    }
   
  if ((int)data_size > neighQlen)
    {
      qlimit4greedy = (int)Qth - (int)data_size;
    }
  else
    {
      qlimit4greedy = (int)Qth - (int)neighQlen;
    }

   
  if (qlimit4greedy<0)
    {
      m_weightV = 0;
    }
  else 
    {
      m_weightV = qlimit4greedy;
    }
  }
  m_vTrace(m_weightV);
}

///
/// \brief Removes tuple_ if expired. Else timer is rescheduled to expire at tuple_->time().
/// \warning Actually this is never invoked because there is no support for multiple interfaces.
/// \param e The event which has expired.
///
void
RoutingProtocol::IfaceAssocTupleTimerExpire (Ipv4Address ifaceAddr)
{
  IfaceAssocTuple *tuple = m_state.FindIfaceAssocTuple (ifaceAddr);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->time < Simulator::Now ())
    {
      RemoveIfaceAssocTuple (*tuple);
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->time),
                                           &RoutingProtocol::IfaceAssocTupleTimerExpire,
                                           this, ifaceAddr));
    }
}

///
///  \brief Changes the state of the reconfState variable
///
void 
RoutingProtocol::ReconfTimerExpire ()
{
  m_reconfState = false;
}

///
/// \brief Clears the routing table and frees the memory assigned to each one of its entries.
///
void
RoutingProtocol::Clear ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Ptr<Ipv4Route> 
RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr) const
{
  NS_ASSERT (m_lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  Ipv4InterfaceAddress ifAddr;
  //SANSA+LENA
  if (m_ipv4->GetObject<Node>()->GetId()==0)
    //testing ul case
    ifAddr = m_ipv4->GetAddress (2, 0);
  else
    ifAddr = m_ipv4->GetAddress (1, 0);
  
  rt->SetSource (ifAddr.GetLocal());
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  NS_LOG_DEBUG("LoopbackRoute: nexthop is loopback: "<<rt->GetGateway()<<" src address: "<<rt->GetSource()<<" and dst address: "<<rt->GetDestination());
  rt->SetOutputDevice (m_lo);
  return rt;
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId() << " " << header.GetDestination () << " " << oif);
  Ipv4Address src = m_ipv4->GetAddress (1, 0).GetLocal();
  Ipv4Address dst = header.GetDestination();
  NS_LOG_DEBUG ("[RouteOutput]: New Data Packet to sent :: source is "<<src<<" , destination addr is "<<dst<< " time is "<<Simulator::Now());
  /*if (p!=NULL)
  {
    m_TxBeginData(src, dst, p);			//tracing the number of data packets to be originated
  }*/
  sockerr = Socket::ERROR_NOTERROR;
  return LoopbackRoute (header); 		//every data packet is sent to the RouteInput function
}

#ifdef GRID_MULTIRADIO_MULTIQUEUE
//RouteInput function for grid multi-radio scheme
bool 
RoutingProtocol::RouteInput  (Ptr<const Packet> p,
  const Ipv4Header &header, Ptr<const NetDevice> idev,
  UnicastForwardCallback ucb, MulticastForwardCallback mcb,
  LocalDeliverCallback lcb, ErrorCallback ecb)
{   
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId() << " " << header.GetDestination ());  
  NS_LOG_DEBUG ("[RouteInput]: New Data Packet to sent :: source is "<<  header.GetSource()<<" , header destination is "<<header.GetDestination()<< " time is "<<Simulator::Now());
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  //std::cout<<"Simulator time node "<<m_ipv4->GetObject<Node>()->GetId()<<" : "<<Simulator::Now()<<std::endl;
  /*if ((int32_t)header.GetProtocol()==1)
    {
      //discard icmp packets
      std::cout<<"en nodo "<<m_ipv4->GetObject<Node>()->GetId()<<" tengo icmp packet: "<<Simulator::Now()<<std::endl;
      return false;
    }*/
  
  if (header.GetTtl() == 1)
    {
      m_expTTL(1);	// the packet is going to exhaust it TTL
    }
    
   // variables to obtain dst address of the packet to be routed
  Ipv4Address dstAddr,srcAddr;
  Mac48Address hwAddr;
  EntryRoutingPacketQueue anEntry;
  Time queue_tstamp;  
  dstAddr = header.GetDestination();
  srcAddr = header.GetSource();
  
  //check to see if it is a packet for ues, in that case deliver locally
  //so the lte interface reinject the gtp encapsulated packet
  uint8_t buf[4];
  dstAddr.Serialize(buf);
  if (buf[0] == 7)
    {
      return false; //let another routing protocol try to handle this
    }
    
    //cambios en RouteInput implican cambios en la funcion callback: RoutingProtocol::TxOpportunity
  ///////////////********TO TREAT MULTIPLE FLOWS FROM THE SAME NODE************////////
  if (header.GetTtl() == 64)
    {
      //if the packet is originated in this node
      Ipv4Address src;
      if (m_ipv4->GetObject<Node>()->GetId() == 0)
        {
          //ul packets: because epc deencapsulates and send to the remotehost
	  //dl packets: the epc receives the packet from the remotehost and bp cares about it.
          src = m_ipv4->GetAddress(2,0).GetLocal();
        }
      else
        {
          src = m_ipv4->GetAddress (1, 0).GetLocal();
        }
      Ipv4Address dst = header.GetDestination();
      if (p!=NULL)
        m_TxBeginData(src, dst, p);			//tracing the number of data packets to be originated
    }    
    
   
  srcAddr.Serialize(buf);
  hwAddr  = Mac48Address::ConvertFrom(m_Mac);
  
  NS_ASSERT (m_ipv4);
  
  uint32_t output_interface;
  //initialization to one in case you only have 2 interfaces (one + loopback)
  uint32_t queue_iface=1; 
  uint32_t iface = 25; 		//Output iface: dummy default value
  
  //variable definitions
  uint32_t numOifAddresses = m_ipv4->GetNAddresses (1); //Interface Address on device 1 is used to map IP to GeoLocation 
  NS_ASSERT (numOifAddresses > 0);
  Ipv4InterfaceAddress ifAddr;
  if (numOifAddresses == 1) 
    {
      NS_LOG_DEBUG("[RouteInput]: ");
      ifAddr = m_ipv4->GetAddress (1, 0);	
      //std::cout<<"IfAddr RouteInput: "<<ifAddr.GetLocal()<<std::endl;
      //We take IP address associated to the identifier of the node for GeoLocation
    }
  else 
    {
      NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing");
    }
  uint32_t check_requeue=0;
  bool SatFlow = false;
 
  ///INSERTING THE PACKET IN A QUEUE
  Ipv4Address the_nh("127.0.0.1");  //Initialize next hop
  //if (m_infoInterfaces.size()>1)
    //{ 29/01/16: m_infoInterfaces, only makes reference to bp interfaces, it makes crashing the system
   // when there was not satellite.
      Ptr<Packet> currentPacket = p->Copy();
      Ipv4Header tmp_header = header;
      uint16_t s_port=999;
      uint16_t d_port=999;
      GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);
       //determine if it is a sat packet (simple port rule) so we place it in the corresponding queue
      ///TODO: In the future, the determination of a packet belonging to sat can be encapsulated as a function
      if (LocationList::m_SatGws.size()>0)
        {
          //there are Satellite gws in the network
	  uint16_t ref_port=1; //by default through the terrestrial
	  if (d_port > 10000)
	    {
	      ref_port = s_port;
	    }
	  else
	    {
	        ref_port = d_port;
	    }
	  if (ref_port%2==0)
	    {
	      SatFlow = true;
	    }
	/*    ///not to force a flow to go through sat
	    SatFlow = false;*/
        }
      //vigila con el SatFlow
      if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
        {
          //check if we have a rule for this packet to determine the queue
          //get sport and dport
          bool erased = false;
          Ipv4Address prev_nh("127.0.0.1");
          the_nh = m_state.FindNextHopInFlowTable(m_ipv4, srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, iface, erased, prev_nh, tmp_header.GetTtl());
          if (!the_nh.IsEqual("127.0.0.1"))
            {
               //there is a rule and we know the output iface
               queue_iface = iface;
            }
         }
      if (the_nh.IsEqual("127.0.0.1"))
        {  //if the flow has not a rule or is per packet, we enqueue it. !!! 
           //What happens with satellite flows? we will enqueued in the appropriate queue to reach satellite
           
          //sansa-lena: changing the dstAddr depending on the situation
          if (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2") )
	    { //the packet is in EPC and has been deencapsulated
	      the_nh = dstAddr;
	      uint8_t buf[4];
	      the_nh.Serialize(buf);
	      buf[3]=buf[3]-1;
	      Ipv4Address tmp_addr= Ipv4Address::Deserialize(buf);
	      queue_iface = m_ipv4->GetInterfaceForAddress(tmp_addr);
	    }
	  else
	    { //not in EPC, not in a TerrGw, or in EPC but DL traffic
	      if (LocationList::m_SatGws.size()>0)
	        { //the aim of this if is to change the dstAddr when required
		  Ptr<Node> node = NodeList::GetNode(0);
		  Ipv4Address tmp_addr = node->GetObject<Ipv4>()->GetAddress(3,0).GetLocal();
		  if (dstAddr.IsEqual(tmp_addr))
		    { //UL traffic
		      if ( (SatFlow) && !(m_ipv4->GetObject<Node>()->GetId() == (NodeList::GetNNodes()-1)) )
		        { //this is a satellite flow and we are in the terrestrial backhaul.
		          //We have to redirect the flow to a sat gw and we do that changing the dstAddr
		          Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
		          Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
		          dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();			  
		        }
		    }
		  else
		  { //DL traffic 
		    if (m_ipv4->GetObject<Node>()->GetId() == 0 && SatFlow)
	              { //this case is to force that a SatFlow to be enqueued to reach the satellite 
			Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
			Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
			dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		      }		    
		  }
	        }
	        
	      if (header.GetTtl()== 64)
                {
	           //source packet: queue according geolocation, if not congested
	           queue_iface = OutputIfaceDeterminationGridSANSALenaMR(m_ipv4, ifAddr.GetLocal(), dstAddr, hwAddr, SatFlow, check_requeue);
		   //std::cout<<"Paquete con (src,dst,s_port,d_port) ("<<srcAddr<<","<<dstAddr<<","<<s_port<<","<<d_port<<") se encola en nodo "<<m_ipv4->GetObject<Node>()->GetId()-1<<"en cola: "<<queue_iface<<std::endl;
                }
              else
                {
                  //forwarding packet
                  queue_iface = OutputIfaceDeterminationGridSANSALenaMR(m_ipv4, ifAddr.GetLocal(), dstAddr, hwAddr, SatFlow, check_requeue);
		  //std::cout<<"Paquete con (src,dst,s_port,d_port) ("<<srcAddr<<","<<dstAddr<<","<<s_port<<","<<d_port<<") se encola en nodo "<<m_ipv4->GetObject<Node>()->GetId()-1<<"en cola: "<<queue_iface<<std::endl;
                  ///inherited code from BP-MR, not included for the moment in SANSA
		  /*if (header.GetSource() == ifAddr.GetLocal())
                    {
	              //SequenceTag seqTag;
                      //bool foundSeq = p->FindFirstMatchingByteTag(seqTag);
	              //if (foundSeq)
	              //std::cout<<"Paquete "<< seqTag.GetSequence()<<" back home @ node: "<<m_ipv4->GetObject<Node>()->GetId()+1<<". Tiempo actual: "<<Simulator::Now()<<std::endl;
	              check_requeue =1;
	              m_requeue_loop=true;
                    }*/
                }
          }             
	}  //end if (the_nh.IsEqual("127.0.0.1"))    
    //} //end if (m_infoInterfaces.size()>1)
  
  ///(queue_iface-1) to access the proper element in the vector of queues in no longer valid
  ///with the idea of 11/11/15. queue_iface should correspond to the number of the iface
  QueueData(p,header, hwAddr, ucb,queue_iface,check_requeue,Simulator::Now()); 
  //TSTAMP DISCIPLINE: FIFO TIME DISCIPLINE, we want to process the packet which has spent more time in the queues
  uint32_t index = 100; //dummy initialization to check that the node is empty
  bool packet_read= false;
  queue_tstamp = 2*Simulator::Now();
  
  uint32_t j=1;
  if (m_ipv4->GetObject<Node>()->GetId() == 0)
    j=2;
  //we look for the packet which arrived before to the queue 
  //remember, queue_iface correspond to the number of the iface,
  //so we change the value of i to avoid reading empty queues
  for (uint32_t i=j; i<m_queuedData->GetNInterfaces(); i++)
    {
      packet_read = m_queuedData->ReadEntry(anEntry,i);
      if (packet_read && (anEntry.m_tstamp < queue_tstamp))
       {
	 queue_tstamp = anEntry.m_tstamp;
	 index = i;
       }
    }
    
  //there will be at least only one packet in the queues because we have inserted previously
  ///previous condition may not be true if we allow fast transmission
  if (index==100)
    { //dummy value, there are not elements in any queue
      return true;
    }
  m_queuedData->ReadEntry(anEntry, index);
  
  ///output_interface = index+1; //IP output_interface: 0 is the loopback interface
  output_interface = index;
  dstAddr = anEntry.GetHeader().GetDestination();
  srcAddr = anEntry.GetHeader().GetSource();
  hwAddr  = anEntry.GetHwAddres ();
  Ptr<Ipv4Route> rtentry;              // fill route entry of the current packet to be routed 
  rtentry = Create<Ipv4Route> ();
  rtentry->SetDestination (dstAddr);
  NS_ASSERT (m_ipv4);
  the_nh.Set("127.0.0.1");	//Initialize next hop
  iface=25;                  //Output iface: dummy default value
  uint8_t ttl;
  Ptr<Node> node = m_ipv4->GetObject<Node>();
  ttl = anEntry.GetHeader().GetTtl();   
  SatFlow = false;
   
  ///ROUTING DECISION: only backpressure for the moment
   
   uint32_t v=200;
   VcalcTimerExpire (output_interface, m_requeue_loop); 
   //the output_interface and the m_requeue_loop variables are not needed in the Vcalc process
   
    if ( (m_vpolicy==VARIABLE_V_SINGLE_RADIO) || (m_vpolicy==VARIABLE_V_MULTI_RADIO) )
      {
        if (ttl<40)
          {
            float increment=((float)((63-ttl))/(63))*(float)(m_queuedData->GetMaxSize()-m_weightV);
            if ( (m_weightV+(int)increment) < m_queuedData->GetMaxSize() )
              {
                v = m_weightV+(int)increment;
              }
            else 
              {
                v=m_queuedData->GetMaxSize();
              }
	  }
        else 
          {
            v = m_weightV;
          }
      }
    else 
      {
        v = m_weightV;
      }
     
     ///inherited from BP-MR, not used for the moment in SANSA
    /*bool centraliced=false;
    if (m_requeue_loop)
      {
        centraliced=true; 
      }
    uint32_t qlen_max; */
    
    //SANSA CODE FOR MULTIRADIO
    if (m_variant == BACKPRESSURE_CENTRALIZED_PACKET || m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
      {
	  Ipv4Header tmp_header;
	  Ptr<Packet> currentPacket;
	  uint16_t s_port=999;
          uint16_t d_port=999;
	  tmp_header= anEntry.GetHeader();
	  //std::cout<<anEntry.GetPacket()<<std::endl;
	  currentPacket = anEntry.GetPacket()->Copy();
	  GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);
	  if (LocationList::m_SatGws.size()>0)
	    {
	      //there are Satellite gws in the network
	      uint16_t ref_port=1; //by default through the terrestrial
	      if (d_port > 10000)
	        {
		  ref_port = s_port;
	        }
	      else
		{
		  ref_port = d_port;
		}
		
	      /*if (ref_port%2==0)
	        {
		  SatFlow = true;
	        }*/
	      bool reset = LocationList::GetResetLocationList();
	      if (!reset)
	      {
	         if (ref_port%2==0)
	          {
		    SatFlow = true;
	          }
	      }
	      else
	      { //reset true, odd ports go thorough satellite
	        if (ref_port==2001 || ref_port==1235)
		  SatFlow = true;
	      }
      /*	        ///not to force satflows
	        SatFlow = false;*/

	    }
	  
	  //per flow decisions 
	  bool erased = false;
	  Ipv4Address prev_nh("127.0.0.1");
	  if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	    {
              the_nh = m_state.FindNextHopInFlowTable(m_ipv4, srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, iface, erased, prev_nh, tmp_header.GetTtl());
	      //check that the interface is available: if not, force to find another path
	      if (!the_nh.IsEqual("127.0.0.1") && !m_ipv4->IsUp(iface) && !erased)
	        {
	            the_nh.Set("127.0.0.1");
	            m_state.EraseFlowTableEntry(srcAddr, dstAddr, s_port, d_port);
	        }            
	     }
	  //if it is per_packet it always enter becasue nh is initialized to 127.0.0.1
          if (the_nh.IsEqual("127.0.0.1"))
            {
	      //sansa-lena
	      if (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2") )
	        { //the packet is in EPC and has been deencapsulated
	          //std::cout<<"UL TRAFFIC ROUTE INPUT MR!!"<<std::endl;
	          the_nh = dstAddr;
		  uint8_t buf[4];
		  the_nh.Serialize(buf);
		  buf[3]=buf[3]-1;
		  Ipv4Address tmp_addr= Ipv4Address::Deserialize(buf);
		  iface = m_ipv4->GetInterfaceForAddress(tmp_addr);
	        }
	      else
	        {
		  if (LocationList::m_SatGws.size()>0)
		    {
		      Ptr<Node> node = NodeList::GetNode(0);
		      Ipv4Address tmp_addr = node->GetObject<Ipv4>()->GetAddress(3,0).GetLocal();
		      if (dstAddr.IsEqual(tmp_addr))
		        {  //these cases are to force the marked SatFlows to go through the satellite
		          //std::cout<<"UL TRAFFIC ROUTE INPUT MR!!"<<std::endl;
		          if (m_ipv4->GetObject<Node>()->GetId() == (NodeList::GetNNodes()-1) || !(SatFlow) )
		            { //we are in the node that connects the satellite link with the EPC (the extraEnodeB)
		              ///new function to determine next hop in multi-radio
		              ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		              the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, output_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      
		            }
		          else
		            { //this is a satellite flow and we are in the terrestrial backhaul. We have to redirect the flow to a sat gw
		              Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
		              Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
		              Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		              //std::cout<<"La ip address del sat Node es: "<<dstAddr<<std::endl;
			      ///new function to determine next hop in multi-radio
		              ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, output_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		            }
		        }
		      else
		        {
			  //std::cout<<"DL TRAFFIC ROUTE INPUT MR!!"<<std::endl;
			  if (m_ipv4->GetObject<Node>()->GetId() == 0 && SatFlow)
			    { //this case is to force that a SatFlow goes through the satellite 
			      Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
			      Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
			      Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
			      ///new function to determine next hop in multi-radio
		              ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, output_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      //std::cout<<"The next hop for a SatFlow packet "<<d_port<<" is: "<<the_nh<<std::endl;
			    }
			  else
			    {
			      ///new function to determine next hop in multi-radio
			      ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			      the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, output_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			    } 
		        }
		    }
		  else
		    {
		      //there are no satellite gw's.
		      ///new function to determine next hop in multi-radio
		      ///the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		      the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALenaMR(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, output_interface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		      //std::cout<<"El nexthop es: "<<the_nh<<" y la iface: "<<iface<<std::endl;
		    }
		}
	       
	        if (!the_nh.IsEqual("127.0.0.1") && m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	          {
		    //the flow table is only updated when we have a neighbor, otherwise we can create a wrong rule, creating a loop
	            m_state.UpdateFlowTableEntry(srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, the_nh, iface, ttl);
		    if (m_ipv4->GetObject<Node>()->GetId() ==0)
		      std::cout<<"the nexthop en EPC "<<m_ipv4->GetObject<Node>()->GetId()<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time: "<<Simulator::Now()<<" destination port: "<<d_port<<" source port: "<<s_port<<std::endl;
		    else
		      std::cout<<"the nexthop en eNodeB "<<m_ipv4->GetObject<Node>()->GetId()-1<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time "<<Simulator::Now()<<" y destination port: "<<d_port<<" source port: "<<s_port<<std::endl;
	          }
	    } //end if (the_nh.IsEqual("127.0.0.1"))                     
      } //end if (m_variant)
   
 
 ///Deciding where to send the packet   
    NS_LOG_DEBUG("[RouteInput]: the next hop is "<<the_nh);
    //std::cout<<"The queue_interface is: "<<output_interface<<", the FNHBPGE function decides interface: "<<iface<<" and next_hop: "<<the_nh<<std::endl;
    //packet from the queues, we perform the same operation as tx_opportunity
    if (iface == 0) //packet cannot be forwarded
      {
        std::cout<<"Es posible esto?? en tiempo: "<<Simulator::Now()<<std::endl;
        return true;
      }
    if ( !the_nh.IsEqual("127.0.0.1") && m_ipv4->IsUp(iface)) 
      { //there is a next hop and the interface is available
        if (iface == output_interface)
	  {
	    //packet sent through this interface
            Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
            rtentry->SetSource (oifAddr.GetLocal ());
	    rtentry->SetGateway (the_nh);
	    rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
	    SendQueuedData(rtentry);
	    return true;
	  }
	else
	  {
	    //the packet has to be sent through other eth, so we have to dequeue (if possible)
            //an enqueue, prior to make the SendQueuedData operation
	    if (anEntry.requeued!=0 && m_ipv4->IsUp(output_interface))
              { //in theory this should never happen as when changing interfaces
                //we empty the queues
	        //this packet has been treated before, so we can send it
                Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (output_interface, 0);
                rtentry->SetSource (oifAddr.GetLocal ());
	        rtentry->SetGateway (the_nh);
	        rtentry->SetOutputDevice (m_ipv4->GetNetDevice (output_interface));
	        SendQueuedData(rtentry);
	        return true;
              }
            else
              { //entry requeued ==0, so we dequeue from output_interface and we send through interface
	        uint32_t tmp = output_interface;
	        bool ret = m_queuedData->Dequeue(anEntry, tmp);
	        if (ret)
	          {
	            anEntry.requeued = 1;
	            QueueData(anEntry.GetPacket(), anEntry.GetHeader(), hwAddr, anEntry.GetUcb(), iface, ret, anEntry.GetTstamp());
	          }
	        else
	          return true;
              } 
            Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
            rtentry->SetSource (oifAddr.GetLocal ());
            rtentry->SetGateway (the_nh);
	    rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
	    SendQueuedData(rtentry);   
	    return true;     
	  }
      }
  return true;
}
      
#else
//Legacy RouteInput function from sparse-grid no multi-radio scheme (neither PtoP, neither hybrid)
bool 
RoutingProtocol::RouteInput  (Ptr<const Packet> p, 
  const Ipv4Header &header, Ptr<const NetDevice> idev,
  UnicastForwardCallback ucb, MulticastForwardCallback mcb,             
  LocalDeliverCallback lcb, ErrorCallback ecb)
{ 
  //////////////********TO TREAT MULTIPLE FLOWS FROM THE SAME NODE************////////
  if (header.GetTtl() == 64)
    {
      //if the packet is originated in this node
      Ipv4Address src;
      if (m_ipv4->GetObject<Node>()->GetId()==0)
        {
	  //ul packets: because epc deencapsulates and send to the remotehost
	  //dl packets: the epc receives the packet from the remotehost and bp cares about it.
	  src = m_ipv4->GetAddress (2, 0).GetLocal();
	  //std::cout<<"Genero paquete en EPC con "<<src<<" "<<std::endl;
	}
      else
        {
          src = m_ipv4->GetAddress (1, 0).GetLocal();
	}
      
      Ipv4Address dst = header.GetDestination();
      if (p!=NULL)
        m_TxBeginData(src, dst, p);			//tracing the number of data packets to be originated
    }
  
    //if (header.GetProtocol()!= TCP_PROT_NUMBER && header.GetProtocol()!=UDP_PROT_NUMBER)
    //{
    //  std::cout<<"El header es: "<<(int)header.GetProtocol()<<std::endl;
    //  std::cout<<"En nodo "<<m_ipv4->GetObject<Node>()->GetId()+1<<" time: "<<Simulator::Now()<<" la src: "<<header.GetSource()<<" la destination: "<<header.GetDestination()<<"con TTL: "<<(int)header.GetTtl()<<std::endl;
    //  std::cout<<"repetimos source: "<<m_ipv4->GetAddress (1, 0).GetLocal()<<std::endl;
    //exit(-1);
    //}
    //Mac48Address MACina = Mac48Address::ConvertFrom(idev->GetAddress());
    //std::cout<<"La MAC es: "<<MACina<<std::endl;
  
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  
  Ipv4Address dstAddr,srcAddr;
  Mac48Address hwAddr;//src mac address of the receiving packet
  bool packet_from_queue = false;
  dstAddr = header.GetDestination(); 
  srcAddr = header.GetSource(); 
  
  //check to see if it is a packet for ues, in that case deliver locally
  //so the lte interface reinject the gtp encapsulated packet
  uint8_t buf[4];
  dstAddr.Serialize(buf);
  if (buf[0] == 7)
  {
    return false; //let another routing protocol try to handle this
  }
  
  if (m_reconfState)
    { //in this state we have to store the packet in the queue to not lost it
       QueueData(p, header, hwAddr, ucb);
       return true; 
    }
  srcAddr.Serialize(buf);
  hwAddr  = Mac48Address::ConvertFrom(m_Mac);
  //NS_LOG_UNCOND( "MAC "<<hwAddr);
  Ipv4Address the_nh("127.0.0.1");	//Initialize next hop
  
  NS_ASSERT (m_ipv4);
  uint32_t numOifAddresses = m_ipv4->GetNAddresses (1); //Interface Address on device 1 is used to map IP to GeoLocation 
  NS_ASSERT (numOifAddresses > 0);
  Ipv4InterfaceAddress ifAddr;
  if (numOifAddresses == 1) 
    {
      NS_LOG_DEBUG("[RouteInput]: ");
      ifAddr = m_ipv4->GetAddress (1, 0);	//We take IP address associated to the identifier of the node for GeoLocation
    }
  else 
    {
      NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing");
    }
  
  //determination of the source port and destination port
  /*Ipv4Header tmp_header;
  Ptr<Packet> currentPacket;
  uint16_t s_port=999, d_port=999;
  currentPacket = p->Copy();
  tmp_header= header;
  GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);*/
  
  //update the flow entry table with the backward path 
  ///!!!No sense for LENA as UL and DL flows have different port number
  /*
  if (header.GetTtl() !=64)
    { //we are not in origin save the backward path
      uint32_t tmp_iface = (uint32_t) m_ipv4->GetInterfaceForDevice (idev);
      the_nh = m_state.FindNextHopInFlowTable(dstAddr, srcAddr, ifAddr.GetLocal(), d_port, s_port, tmp_iface);
      if (the_nh.IsEqual("127.0.0.1"))
        {
	  Ipv4Address neighborIpv4Address = m_state.FindtheNeighbourAtThisInterface(tmp_iface);
	  m_state.UpdateFlowTableEntry(dstAddr, srcAddr, ifAddr.GetLocal(), d_port, s_port, neighborIpv4Address, tmp_iface);
        }
    }*/

  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId() << " " << header.GetDestination ());  
  
  //NS_LOG_DEBUG ("[RouteInput]: BACKPRESSURE: New Data Packet to sent :: source is "<<  header.GetSource()<<" , header destination is "<<header.GetDestination()<<"received from "<<m_Mac<<" time is "<<Simulator::Now());
  //NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  if (header.GetTtl() == 1)
    {
      m_expTTL(1);	// the packet is going to exhaust it TTL
    }

  // obtain dst address of the packet to be routed
  
  EntryRoutingPacketQueue anEntry;
  if (m_queuedData->ReadEntry(anEntry)) // the packet to be routed is the "first" one of the data queue
    {
      dstAddr= anEntry.GetHeader ().GetDestination ();
      srcAddr= anEntry.GetHeader ().GetSource ();
      hwAddr = anEntry.GetHwAddres ();
      //need the MAC src address now
      packet_from_queue = true;
    } 
  Ptr<Ipv4Route> rtentry;              // fill route entry of the current packet to be routed 
  rtentry = Create<Ipv4Route> ();
  rtentry->SetDestination (dstAddr);
    
  uint32_t iface=25;  		        //Output iface: initialized to dummy value
  if ( m_variant == GREEDY_FORWARDING)
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      the_nh = m_state.FindNextHopGreedyForwardingSinglePath(ifAddr.GetLocal(), dstAddr, pos, iface);
      
      /*BP-SDN CODE: we did multi-radio but single queue*/
      //this code can be recover from commites on sansa sharedka previous to 14/05/15      
    }
  else if ( m_variant == MP_FORWARDING)
    {
      Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
      Vector    pos     = theNode->GetObject<MobilityModel>()->GetPosition();
      the_nh = m_state.FindNextHopGreedyForwardingMultiPath(ifAddr.GetLocal(), dstAddr, pos, iface);
    }
  else //BACKPRESSURE
    {
      Ptr<Node> node = m_ipv4->GetObject<Node> ();
      uint32_t v;
      uint8_t ttl;
      bool SatFlow = false;
      if (m_queuedData->GetSize()==0) // data queue is empty
        {
          ttl=header.GetTtl(); // take as TTL the current data packet
        }
      else
        {
          ttl=anEntry.GetHeader().GetTtl(); // data queue is not empty
        }
      VcalcTimerExpire ();
      if ( (m_vpolicy==VARIABLE_V_SINGLE_RADIO) || (m_vpolicy==VARIABLE_V_MULTI_RADIO) )
        {
          if (ttl<40)
            {
              float increment=((float)((63-ttl))/(63))*(float)(m_queuedData->GetMaxSize()-m_weightV);
              if ( (m_weightV+(int)increment) < m_queuedData->GetMaxSize() )
                {
                  v = m_weightV+(int)increment;
                }
              else 
                {
                  v=m_queuedData->GetMaxSize();
                }
            }
          else 
            {
	      //the value of v is the one calculated with VcalcTimerExpire
              v = m_weightV;
            }
        }
      else 
        {
          v = m_weightV;
        }
      if (m_variant == BACKPRESSURE_CENTRALIZED_PACKET || m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
        {
	  Ipv4Header tmp_header;
	  Ptr<Packet> currentPacket;
	  uint16_t s_port=999;
          uint16_t d_port=999;
		  
	  //depending wether we process the incoming packet or one from the queue
	  if (packet_from_queue)
	  {
	    tmp_header= anEntry.GetHeader();
	    currentPacket = anEntry.GetPacket()->Copy();
	   }
	  else
	  {
	    currentPacket = p->Copy();
	    tmp_header= header;
	  }
	  
	  GetSourceAndDestinationPort(tmp_header, currentPacket, s_port, d_port);
	  //if it is present in the flow table or flow table entry expired then compute weights
	  //initial simple traffic classification
	  /*if (d_port%2 == 0 && LocationList::m_SatGws.size()>0)
	    {
	      SatFlow = true;
	      
	    }*/
	 	  
	  if (LocationList::m_SatGws.size()>0)
	    {
	      //there are Satellite gws in the network
	      uint16_t ref_port=1; //by default through the terrestrial
	      if (d_port > 10000)
	        {
		  ref_port = s_port;
	        }
	      else
		{
		  ref_port = d_port;
		}
		
	      /*if (ref_port%2==0)
	        {
		  SatFlow = true;
	        }*/
	      bool reset = LocationList::GetResetLocationList();
	      if (!reset)
	      {
	         if (ref_port%2==0)
	          {
		    SatFlow = true;
	          }
	      }
	      else
	      { //reset true, odd ports go thorough satellite
	        if (ref_port==2001 || ref_port==1235)
		  SatFlow = true;
	      }
	    }
	  
	  //if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	    {
	      //per flow decisions 
	      bool erased = false;
	      Ipv4Address prev_nh("127.0.0.1");
	      if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	        {
                  the_nh = m_state.FindNextHopInFlowTable(m_ipv4, srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, iface, erased, prev_nh, tmp_header.GetTtl());
	          //check that the interface is available: if not, force to find another path
	          if (!the_nh.IsEqual("127.0.0.1") && !m_ipv4->IsUp(iface) && !erased)
	            {
	              the_nh.Set("127.0.0.1");
	              m_state.EraseFlowTableEntry(srcAddr, dstAddr, s_port, d_port);
	            }            
		}
	      //if it is per_packet it always enter becasue nh is initialized to 127.0.0.1
              if (the_nh.IsEqual("127.0.0.1"))
                {
	          //sansa-lena
	          if (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2") )
	            { //the packet is in EPC and has been deencapsulated
	              //std::cout<<"UL TRAFFIC ROUTE INPUT!!"<<std::endl;
	              the_nh = dstAddr;
		      uint8_t buf[4];
		      the_nh.Serialize(buf);
		      buf[3]=buf[3]-1;
		      Ipv4Address tmp_addr= Ipv4Address::Deserialize(buf);
		      iface = m_ipv4->GetInterfaceForAddress(tmp_addr);
	            }
	          else
	            {
		      if (LocationList::m_SatGws.size()>0)
		        {
		          Ptr<Node> node = NodeList::GetNode(0);
		          Ipv4Address tmp_addr = node->GetObject<Ipv4>()->GetAddress(3,0).GetLocal();
		          if (dstAddr.IsEqual(tmp_addr))
		            {  //these cases are to force the marked SatFlows to go through the satellite
		              //std::cout<<"UL TRAFFIC ROUTE INPUT!!"<<std::endl;
		              if (m_ipv4->GetObject<Node>()->GetId() == (NodeList::GetNNodes()-1) || !(SatFlow) )
		                { //we are in the node that connects the satellite link with the EPC (the extraEnodeB)
		                  the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		                }
		              else
		                { //this is a satellite flow and we are in the terrestrial backhaul. We have to redirect the flow to a sat gw
		                  Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
		                  Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
		                  Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		                  //std::cout<<"La ip address del sat Node es: "<<dstAddr<<std::endl;
		                  the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		                }
		            }
		          else
		            {
			      //std::cout<<"DL TRAFFIC ROUTE INPUT!!"<<std::endl;
			      if (m_ipv4->GetObject<Node>()->GetId() == 0 && SatFlow)
			        { //this case is to force that a SatFlow goes through the satellite 
			          Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
			          Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
			          Ipv4Address dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		                  the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			        }
			      else
			        {
			          the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
			        } 
		            }
		        }
		      else
		        {
		          //there are no satellite gw's.
		          the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSALena(ifAddr.GetLocal(), srcAddr, dstAddr, v, iface, m_ipv4, hwAddr, erased, prev_nh, SatFlow);
		          //std::cout<<"El nexthop es: "<<the_nh<<" y la iface: "<<iface<<std::endl;
		        }
		    }
	       
	          if (!the_nh.IsEqual("127.0.0.1") && m_variant == BACKPRESSURE_CENTRALIZED_FLOW)
	            {
		      //the flow table is only updated when we have a neighbor, otherwise we can create a wrong rule, creating a loop
	              m_state.UpdateFlowTableEntry(srcAddr, dstAddr, ifAddr.GetLocal(), s_port, d_port, the_nh, iface, ttl);
		      if (m_ipv4->GetObject<Node>()->GetId() ==0)
		        std::cout<<"the nexthop en EPC "<<m_ipv4->GetObject<Node>()->GetId()<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time: "<<Simulator::Now()<<" destination port: "<<d_port<<" source port: "<<s_port<<std::endl;
		      else
		        std::cout<<"the nexthop en eNodeB "<<m_ipv4->GetObject<Node>()->GetId()-1<<" srcAddr: "<<srcAddr<<" dstAddr: "<<dstAddr<<" nexthop: "<<the_nh<<" time "<<Simulator::Now()<<" y destination port: "<<d_port<<" source port: "<<s_port<<std::endl;
	            }
	        }
	  
            } //end backpressure_centralized_flow
          
        }// end if x_flow o x_packet
      else
        {
	  //paper BP-SDN
          //the_nh = m_state.FindNextHopBackpressureDistributed(m_variant, ifAddr.GetLocal(), srcAddr, dstAddr, ttl, m_queuedData->GetSize(), m_ipv4->GetObject<Node> ()->GetId(), pos, v, iface, hwAddr, m_nodesRow);
	  the_nh = m_state.FindNextHopBackpressureCentralizedGridSANSA(ifAddr.GetLocal(), srcAddr, dstAddr, m_ipv4->GetObject<Node>()->GetId(), v, iface, m_ipv4, m_nodesRow, hwAddr);
        }
    }
  NS_LOG_DEBUG("[RouteInput]: the next hop is "<<the_nh);
  QueueData(p, header, hwAddr, ucb);
  if (!the_nh.IsEqual("127.0.0.1") && m_ipv4->IsUp(iface))
    {
      Ipv4InterfaceAddress oifAddr = m_ipv4->GetAddress (iface, 0);
      rtentry->SetSource (oifAddr.GetLocal ());
      rtentry->SetGateway (the_nh);
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (iface));
      //if mac data queue is not full send data queue
      SendQueuedData(rtentry);
    }
    
  return true;
}
#endif

void 
RoutingProtocol::ReallocatePackets(uint32_t interface)
{
  std::map<PacketTuple,int> newpacketifaces;
  std::map<PacketTuple,int>::iterator it;
  
  //variable to store the new interfaces to call the callback
  std::vector<uint32_t> interfaces;
  EntryRoutingPacketQueue anEntry; 
  uint32_t iface=1;
 
  while (m_queuedData->Dequeue(anEntry, interface))
    {
      //all packets belonging to a flow in a death queue are enqueued in the same other queue
      PacketTuple aPacketTuple;
      //initialize the aPacketTuple variable;
      aPacketTuple.src = anEntry.GetHeader().GetSource();
      aPacketTuple.dst = anEntry.GetHeader().GetDestination();
      //hwAddr = anEntry.GetHwAddres();
      Ptr<Packet> currentPacket = anEntry.GetPacket()->Copy();
      Ipv4Header tmp_header = anEntry.GetHeader();
      bool SatFlow = false;
      uint32_t check_requeue=0;
      GetSourceAndDestinationPort(tmp_header, currentPacket, aPacketTuple.srcPort, aPacketTuple.dstPort);
      if (aPacketTuple.srcPort==0)
        {
           std::cout<<"aPTsrc: "<<aPacketTuple.srcPort<<","<<"aPtdst:"<<aPacketTuple.dstPort<<",time: "<<Simulator::Now()<<std::endl;
           continue;
	   //exit(-1);
	}
      //check this new entry not in newpacketifaces map variable (interface a dummy value, we are interested in the key)
      it = newpacketifaces.find(aPacketTuple);
      if (it !=newpacketifaces.end() && m_variant != BACKPRESSURE_CENTRALIZED_PACKET)
        { //the PacketTuple is already registered so we have previously selected an interface for this packet, let's say it's not the first packet
          iface = it->second;
	  //std::cout<<"Iface down: "<<interface<<" Known rule: Packet (src,dst, srcPort, dstPort) ("<<aPacketTuple.src<<","<<aPacketTuple.dst<<","<<aPacketTuple.srcPort<<","<<aPacketTuple.dstPort<<") assigned to iface: "<<iface<<", "<<Simulator::Now()<<std::endl;
        }
      else
        { //the PacketTuple is not present and we have to determine a interface 
          Ipv4Address the_nh("127.0.0.1");  //Initialize next hop
          if (LocationList::m_SatGws.size()>0)
            {
              //there are Satellite gws in the network
	      uint16_t ref_port=1; //by default through the terrestrial
	      if (aPacketTuple.dstPort > 10000)
	        {
	          ref_port = aPacketTuple.srcPort;
	        }
	      else
	        {
	          ref_port = aPacketTuple.dstPort;
	        }
	      if (ref_port%2==0)
	        {
	          SatFlow = true;
	        }
	/*        ///not to force satflows
	        SatFlow = false;*/
            }
            //vigila con el SatFlow
          if (m_variant == BACKPRESSURE_CENTRALIZED_FLOW || m_variant == BACKPRESSURE_CENTRALIZED_PACKET)
	    {
	      //in this case we do not have to see if there is a rule, because they are going to be erased
	      if (the_nh.IsEqual("127.0.0.1"))
	        {
		  //if the flow has not a rule or is per packet, we enqueue it. !!! 
                  //What happens with satellite flows? we will enqueued in the appropriate queue to reach satellite
		  Ipv4Address dstAddr(aPacketTuple.dst);
                  //sansa-lena: changing the dstAddr depending on the situation
                  if (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2") )
	            { //the packet is in EPC and has been deencapsulated
	              the_nh = aPacketTuple.dst;
	              uint8_t buf[4];
	              the_nh.Serialize(buf);
	              buf[3]=buf[3]-1;
	              Ipv4Address tmp_addr= Ipv4Address::Deserialize(buf);
	              iface = m_ipv4->GetInterfaceForAddress(tmp_addr);
	              //std::cout<<"Paquete con (src,dst,s_port,d_port) ("<<srcAddr<<","<<dstAddr<<","<<s_port<<","<<d_port<<") se encola en nodo "<<m_ipv4->GetObject<Node>()->GetId()-1<<"en cola: "<<queue_iface<<std::endl;
	            }
                  else
	            { //not in EPC, or in EPC but DL traffic
	              if (LocationList::m_SatGws.size()>0)
		        { //the aim of this if is to change the dstAddr when required
		          Ptr<Node> node = NodeList::GetNode(0);
		          Ipv4Address tmp_addr = node->GetObject<Ipv4>()->GetAddress(3,0).GetLocal();
		          if (aPacketTuple.dst.IsEqual(tmp_addr))
		            { //UL traffic
		              if ( (SatFlow) && !(m_ipv4->GetObject<Node>()->GetId() == (NodeList::GetNNodes()-1)) )
		                { //this is a satellite flow and we are in the terrestrial backhaul.
		                  //We have to redirect the flow to a sat gw and we do that changing the dstAddr
		                  Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
		                  Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
		                  dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();			  
		                }
		            }
		          else
		            { //DL traffic 
		              if (m_ipv4->GetObject<Node>()->GetId() == 0 && SatFlow)
	                        { //this case is to force that a SatFlow to be enqueued to reach the satellite 
			          Ptr<Node> satNode = NodeList::GetNode(NodeList::GetNNodes()-1);
			          Ptr<Ipv4> satNode_ipv4 = satNode->GetObject<Ipv4>();
			          dstAddr = satNode_ipv4->GetAddress(1,0).GetLocal();
		                }		    
		              }
	                }
	        
	              if (tmp_header.GetTtl()== 64)
                        {
	                  //source packet: queue according geolocation, if not congested
	                  iface = OutputIfaceDeterminationGridSANSALenaMR(m_ipv4, m_ipv4->GetAddress (1, 0).GetLocal(), dstAddr, anEntry.GetHwAddres(), SatFlow, check_requeue);
		          //std::cout<<"Paquete con (src,dst,s_port,d_port) ("<<srcAddr<<","<<dstAddr<<","<<s_port<<","<<d_port<<") se encola en nodo "<<m_ipv4->GetObject<Node>()->GetId()-1<<"en cola: "<<queue_iface<<std::endl;
                        }
                      else
                        {
                          //forwarding packet
                          iface = OutputIfaceDeterminationGridSANSALenaMR(m_ipv4, m_ipv4->GetAddress (1, 0).GetLocal(), dstAddr, anEntry.GetHwAddres(), SatFlow, check_requeue);
		          //std::cout<<"Paquete con (src,dst,s_port,d_port) ("<<srcAddr<<","<<dstAddr<<","<<s_port<<","<<d_port<<") se encola en nodo "<<m_ipv4->GetObject<Node>()->GetId()-1<<"en cola: "<<queue_iface<<std::endl;
                          ///inherited code from BP-MR, not included for the moment in SANSA
		          /*if (header.GetSource() == ifAddr.GetLocal())
                           {
	                     //SequenceTag seqTag;
                            //bool foundSeq = p->FindFirstMatchingByteTag(seqTag);
	                    //if (foundSeq)
	                    //std::cout<<"Paquete "<< seqTag.GetSequence()<<" back home @ node: "<<m_ipv4->GetObject<Node>()->GetId()+1<<". Tiempo actual: "<<Simulator::Now()<<std::endl;
	                    check_requeue =1;
	                    m_requeue_loop=true;
                          }*/
                        }
                    }          
	        } // end if (the_nh.IsEqual....
	    } //end if (m_variant...      
	  //store the found interface for these packets
	  newpacketifaces.insert(std::pair<PacketTuple,int>(aPacketTuple,iface));
	  //std::cout<<"Packet (src,dst, srcPort, dstPort) ("<<aPacketTuple.src<<","<<aPacketTuple.dst<<","<<aPacketTuple.srcPort<<","<<aPacketTuple.dstPort<<") assigned to iface: "<<iface<<std::endl;
        } //end else the PacketTuple is not present  
      //we set the reenqueue flag to 0-2 (depending on congestion), to allow backpressure decide the output queue for this flow/packet
      QueueData(anEntry.GetPacket(), anEntry.GetHeader(), anEntry.GetHwAddres(), anEntry.GetUcb(), iface, check_requeue, anEntry.GetTstamp());      
      if (std::find(interfaces.begin(), interfaces.end(), iface) == interfaces.end())
        { //this interface is not in the list, we insert it in the interfaces list
          interfaces.push_back(iface);
	}
    } //end while
  //activate the transmission of the packets in the new queue if not busy
  for (uint32_t i=0; i<interfaces.size(); i++)
    {// we call the callback for the different ifaces
      Ptr<PointToPointNetDevice> dev = m_ipv4->GetNetDevice(interfaces[i])->GetObject<PointToPointNetDevice>();
      if (dev && dev->IsReadyTx())
	{
	  Address mac = m_ipv4->GetNetDevice(interfaces[i])->GetAddress();
          //std::cout<<"Reallocate calls callback for interface: "<<interfaces[i]<<" mac: "<<mac<<std::endl;
	  TxOpportunity (mac);          
	}
    } 
}


void 
RoutingProtocol::AddCache(uint32_t i)
{
 NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ());
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (l3->GetNAddresses (i) > 1)
    {
      NS_LOG_WARN ("BACKPRESSURE does not work with more then one address per each interface.");
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    return;
  
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  // Allow neighbor manager use this interface for layer 2 feedback if possible
  //Jordi 04/12/2013: I have to remove this part of code to make it compatible with grid multiradio
  /*Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  Ptr<PointToPointNetDevice> pTop = dev->GetObject<PointToPointNetDevice> ();
  if (wifi == 0 || pTop == 0)
  {
    return;
  }
  Ptr<WifiMac> macW = wifi->GetMac ();
  if (macW == 0)
    return;*/
  //I have no mac in PtoP device
  if ( ( l3->GetInterface (i)->GetArpCache() ) != 0)
    {
      m_state.InsertArpTuple (l3->GetInterface (i)->GetArpCache ());
    }
  else
    NS_LOG_UNCOND("ArpCache is NULL");
}

//set backhaul interface down
void 
RoutingProtocol::SetInterfaceDown (uint32_t iface) 
{
  NS_LOG_DEBUG("Down Iface");
  m_ipv4->SetDown(iface);
  //std::cout<<"switching OFF enodeb with IP: "<<m_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
  // TODO: Proactively Update Routing Tables given the new state"
}

//set backhaul interface up
void
RoutingProtocol::SetInterfaceUp (uint32_t iface) 
{
  NS_LOG_DEBUG("Up Iface");
  m_ipv4->SetUp(iface);
  //std::cout<<"switching ON enodeb with IP: "<<m_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
}

//set all backhaul interfaces down
void
RoutingProtocol::SetNodeDown () 
{
  // TODO: Tx Everything before switching OFF the node Proactively Update Routing Tables given the new state"
  // Prior to shutting down the node we should empty queues
  uint32_t i=1;
  if (m_ipv4->GetObject<Node>()->GetId() == 0)
    i=2;
  for (uint32_t j=i; j<m_ipv4->GetNInterfaces();j++)
    {
      m_ipv4->SetDown(j);
    }
}

//set all the backhaul interfaces up
void
RoutingProtocol::SetNodeUp ()
{
  uint32_t i=1;
  if (m_ipv4->GetObject<Node>()->GetId() == 0)
    i=2;
  for (uint32_t j=i; j<m_ipv4->GetNInterfaces(); j++)
    {
      m_ipv4->SetUp(j);
    }
}

//Erase the flow tables after a change in the topology
void 
RoutingProtocol::EraseFlowTable()
{
  //a) Erase the flow table and the neighbor table
  m_state.EraseFlowTable();
  //not needed part b) because we have information of the neighbor before switching it off
  //b) Enter in reconfiguration state to stop transmission during a time
  //m_reconfState = false;
  //m_reconfTimer.Schedule (2*m_helloInterval);
  
  
}

void 
RoutingProtocol::SetInfoInterfaces (int iface_id, int value)
{
  std::pair<std::map<int,int>::iterator,bool> ret;
  ret = m_infoInterfaces.insert (std::pair<int,int>(iface_id,value) );
  if (ret.second==false) 
    {
      //std::cout << "element "<<iface_id<<" already existed";
      //std::cout << " with a value of " << ret.first->second << '\n';
      //we update the value
      ret.first->second = value;
      //std::cout<< "and we change to value: "<< ret.first->second<<std::endl;
    }
}


void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{}

void 
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{}
void 
RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{}
void 
RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{}

uint32_t
RoutingProtocol::GetDataQueueSize()
{
  return m_queuedData->GetSize();
}

uint32_t
RoutingProtocol::GetVirtualDataQueueSize (uint8_t direction)
{
  uint32_t queue_length;
  queue_length = m_queuedData->GetIfaceSize((uint32_t)direction);
  return queue_length;
}


uint32_t
RoutingProtocol::GetDataQueueSizePrev()
{
  return m_queuedData->GetSizePrev();
}

uint32_t
RoutingProtocol::GetShadow(Ipv4Address dstAddr)
{
  return m_state.GetShadow(dstAddr);
}

//GRID
//Stage 1: BP-MR. Before computing the actual next-hop the packet is associated to the closest interface to the destination
uint32_t
RoutingProtocol::OutputIfaceDeterminationGrid(Ptr<Ipv4> m_ipv4, uint8_t  ttl, Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t &check_requeue, Mac48Address from)
{
  uint32_t output_interface=0;
  Vector currPos = LocationList::GetLocation(currAddr);
  Vector dPos = LocationList::GetLocation(dstAddr); 		        // Get coordinates from the destination
  NeighborSameLengthSet theNeighGoodWeight; 				// Nodes with the same length; Candidate List WHAT IS THE ROLE OF THESE PARAMETERS?
  NeighborSameLengthSet theNeighBadWeight;
  theNeighGoodWeight.reserve(100);
  theNeighBadWeight.reserve(100);
  Vector nPos;
  std::vector<double> distances;
  
  double eucDistFromCurr;
  double eucDistFromNeigh;
  //first: get the references
  eucDistFromCurr = CalculateDistance(dPos, currPos);                   // Distance to the destination from the current node
  //second: obtain the proper distances of the neighbors
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  uint32_t neighbor_index = 0;
  uint32_t useful_neighbor= 50;
  NeighborSet neighbor= m_state.GetNeighbors();
  bool penalty;
  
  for (NeighborSet::iterator it = neighbor.begin();
       it != neighbor.end(); it++)
    {
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_state.m_neighValid.GetSeconds());
      if ( ( Simulator::Now().GetSeconds()-it->lastHello.GetSeconds() ) > m_state.m_neighValid.GetSeconds() ) 
        {
	  continue;  //Neighbor is no longer valid, jump to the next neighbor
        }
      if ( it->theMainAddr.IsEqual(dstAddr) ) 
        {
	  if ( ( GetVirtualDataQueueSize(it->interface-1)< m_queuedData->GetMaxSize() ) ) // Neighbor is the destination, enqueue if not overflow
	    return it->interface;
        }
      nPos = Vector(it->x, it->y, 0);
      eucDistFromNeigh = CalculateDistance (dPos, nPos);
      distances.push_back(eucDistFromNeigh);
      if (eucDistFromNeigh > eucDistFromCurr)
        {
	  penalty = true; //Neighbor is farther from the destination penalice decissions
        }
      else
        {
	  //the neighbor is closer to destination
	  if (from == it->neighborhwAddr)
	    {
	      //std::cout<<"From: "<<from<<", neighbor: "<<it->neighborhwAddr<<std::endl;
      	      eucDistFromNeigh = 10*eucDistFromNeigh; //to avoid choosing again the same interface when there is no valid neighbor
	      distances.erase(distances.end()-1);
	      distances.push_back(eucDistFromNeigh);
	      penalty = true;
	    }
	  else
	    {
	      penalty = false; //reward decisions
	    }
	}
	if (!penalty)
	  { //I save the neighbor with a false penalty
             theNeighGoodWeight.push_back(&(*it));
	     eqNeighs++;
	     useful_neighbor = neighbor_index;
	  }
        else
	 {
	     theNeighBadWeight.push_back(&(*it));
	 }
	 neighbor_index++;
    }
    if (eqNeighs==0)
      {
         //erase the vectors, there is a hole
         //we select the neighbor which is closer to destination: GF 
         //the closer neighbor cannot be possible if I have already discarded
         
         eqNeighs=0;
         theNeighGoodWeight.erase (theNeighGoodWeight.begin(),theNeighGoodWeight.end());
         theNeighBadWeight.erase(theNeighBadWeight.begin(), theNeighBadWeight.end());
	 eucDistFromCurr = CalculateDistance(dPos, currPos);
	 double ref_distance= 99999.0;
	 //search for the minimum distance
	 for (uint32_t i=0; i<distances.size(); i++)
	   {
	     if (distances[i] < ref_distance) //In this case distances constains "distances" from farther or the previous hop
	       {
	         ref_distance = distances[i];
	       }
	   }
	 for (NeighborSet::iterator it = neighbor.begin();
	      it != neighbor.end(); it++)
	    {  
              if ( ( Simulator::Now().GetSeconds()-it->lastHello.GetSeconds() ) > m_state.m_neighValid.GetSeconds() )
               { 
	          continue;  //Neighbor is not present, jump to the next neighbor
               }
	      Vector nPos = Vector(it->x, it->y, 0);
	      eucDistFromNeigh = CalculateDistance(dPos, nPos);
	      if (eucDistFromNeigh == ref_distance)
	        {
		  theNeighGoodWeight.push_back(&(*it));
	          eqNeighs++;
	        }
	    }
      }
    if (eqNeighs>1)
      { 
	//ALTERNATIVE A
	//in a GRID, the maximum number of closer neighbors is two, but let's do something more intelligent, 
	//to insert the packet in the interface with lower load instead of randomly 
	//float nlen;
	uint32_t selected_eth, nlen=0;
	uint32_t i=0,index=0;
	uint64_t rate_bps, rate_bps_tmp;
	Ptr<NetDevice> aDev;
	Ptr<PointToPointNetDevice> aPtopDev;
	Ptr<WifiNetDevice> aWifiDev;
	
	for (i=0; i<eqNeighs; i++)
	  {
	    selected_eth = theNeighGoodWeight[i]->interface-1;
	    nlen = nlen + m_queuedData->GetIfaceSize(selected_eth);
	  }
	  
	if (nlen == 0)
	  {//all the queues are empty, so select the fastest iface
	    rate_bps= 0;
	    for (i=0; i<eqNeighs; i++)
	      {
		if (theNeighGoodWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
		aDev= m_ipv4->GetNetDevice(theNeighGoodWeight[i]->interface);
	        aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
                aWifiDev = aDev->GetObject<WifiNetDevice> ();
		if (aWifiDev == NULL)
	          rate_bps_tmp = aPtopDev->GetRatebps();
	        else
	          rate_bps_tmp = aWifiDev->GetRatebps();
		if (rate_bps_tmp > rate_bps)
		  index=i;
	      }
	      output_interface = theNeighGoodWeight[index]->interface;
	  }
	else
	  {
	    nlen = 99999;
	    for (i=0; i<eqNeighs; i++)
	      {
		if (theNeighGoodWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
	        selected_eth = theNeighGoodWeight[i]->interface-1;
	        aDev = m_ipv4->GetNetDevice(theNeighGoodWeight[i]->interface);
	        aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
                aWifiDev = aDev->GetObject<WifiNetDevice> ();
	        if (aWifiDev == NULL)
	          rate_bps = aPtopDev->GetRatebps();
	        else
	          rate_bps = aWifiDev->GetRatebps();
	        
		if (((float)m_queuedData->GetIfaceSize(selected_eth)/ (float) rate_bps) < nlen)
	          {
		    nlen = m_queuedData->GetIfaceSize(selected_eth);
		    index = i;
	          }
	      }
	      //std::cout<<"El ttl_2 es: "<<(int32_t)ttl<<" el vecino es: "<<theNeighSameWeight[index]->theMainAddr<<"("<<theNeighSameWeight[index]->interface<<") y la destination address: "<<dstAddr<<", la interface: "<<theNeighSameWeight[index]->interface<<std::endl;
	      output_interface = theNeighGoodWeight[index]->interface;	
	  }	  
      }
    else //number of neighbor equal to one
      { 
	uint32_t selected_eth = theNeighGoodWeight[0]->interface-1;
	if (m_queuedData->GetIfaceSize(selected_eth) < (m_queuedData->GetMaxSize()/2) && (!m_histeresis[selected_eth]))
	  {
	    //std::cout<<"El ttl_3 es: "<<(int32_t)ttl<<" el vecino es: "<<theNeighSameWeight[0]->theMainAddr<<"("<<theNeighSameWeight[0]->interface<<") y la destination address: "<<dstAddr<<", la interface: "<<theNeighSameWeight[0]->interface<<std::endl;
	    output_interface = theNeighGoodWeight[0]->interface;	
 	  }
	else
	  { //this interface is full, so we should select other interface to place the packet 
	    //one neighbor further from destination should be chosen
	    if (useful_neighbor!=50)
	      { //there is one useful_neighbor (closer) but congested
	         m_histeresis[selected_eth]=true;
	         if (m_queuedData->GetIfaceSize(selected_eth)< (m_queuedData->GetMaxSize()/4))
		    m_histeresis[selected_eth]=false;
		 
	         uint32_t tmp = AlternativeIfaceSelection(m_ipv4, useful_neighbor, distances);
	         if (tmp==10)
		   {
	             output_interface = theNeighGoodWeight[0]->interface;
		   }
	         else
	           {
	             output_interface = tmp;
	             check_requeue = 2;
		   }
	      }
	    else
	      { //there is not a useful neighbor (one closer)
		m_histeresis[selected_eth]=false;
		output_interface = theNeighGoodWeight[0]->interface;
	      }
	  }
      }  
    return output_interface;
}

bool
RoutingProtocol::IsValidNeighborSANSA (Time lastHello, uint32_t interface, bool SatFlow, bool SatNode)
{
  
  if (!(m_ipv4->IsUp(interface)))
    {
      //if the interface is not up is not a valid neighbor
      return false;
    }
  
  //time constraints of a possible neighbor
  if ( (Simulator::Now().GetSeconds()-lastHello.GetSeconds()) > m_state.m_neighValid.GetSeconds())
    {
      if (SatNode == true && interface==1)
	{
	   //is a valid neighbor, and we assume satellite is always available
	}
      else
        {
	  //if the neighbor is not present and it is not the satellite interface, we discard it
	  return false; 			//Neighbor is not present, jump to the next neighbor
        }
    }
      
  //flow constraints and type of link
  if (SatNode == true)
    {
      if (!SatFlow && interface==1)
	{
	  //we are in a node with a satellite link, but the flow is not 
	  //decided to go through the satellite connection
	  //std::cout<<"En "<<currAddr<<"neighbor es: "<<it->neighborMainAddr<<"y su m_neighValid.GetSeconds es: "<<m_neighValid.GetSeconds()<<std::endl;  
	  return false;
	}
    }
      
  if (!SatFlow && LocationList::m_SatGws.size()>0)
    { //if it is not a satflow, it has to go through the terrestrial backhaul, this 
      //is relevant in the EPC node. We do not consider the last interface as a possible neighbor
      //in case of being a SatFlow
      if ((m_ipv4->GetObject<Node>()->GetId()==0) && (interface==m_ipv4->GetNInterfaces()-1))
	{
	  //std::cout<<"hello"<<std::endl;
	  return false;
	}
    }  
  return true;
}


///////////////////////////////////////
//Stage 1:SANSA LENA MR
uint32_t
RoutingProtocol::OutputIfaceDeterminationGridSANSALenaMR(Ptr<Ipv4> m_ipv4, Ipv4Address const &currAddr, Ipv4Address const &dstAddr, Mac48Address from, bool SatFlow, uint32_t &check_requeue)
{
  //taking as a reference the function OutputIfaceDeterminationGrid of BP-MR, we adapt it to 
  //the SANSA LENA MR case
  uint32_t output_interface=0;
  NeighborSameLengthSet theNeighGoodWeight; 				// Nodes with the same length; Candidate List WHAT IS THE ROLE OF THESE PARAMETERS?
  NeighborSameLengthSet theNeighBadWeight;
  theNeighGoodWeight.reserve(10);
  theNeighBadWeight.reserve(10);
  uint32_t HopsFromCurr = LocationList::GetHops(currAddr, dstAddr, SatFlow);
  /* if (m_ipv4->GetObject<Node>()->GetId() == 2 )
     std::cout<<"en enode b currAddr: "<<currAddr<<", dst: "<< dstAddr << "y numero hops: "<<HopsFromCurr<<" para satflow: "<<SatFlow<<std::endl;*/
  std::vector<uint32_t> distances;
  //second: obtain the proper distances of the neighbors
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  uint32_t neighbor_index = 0;
  uint32_t useful_neighbor= 50;
  NeighborSet neighbor= m_state.GetNeighbors();
  bool penalty;
  uint32_t count_closer=0;
  
  for (NeighborSet::iterator it = neighbor.begin(); it!=neighbor.end(); it++)
    {
      uint32_t HopsNeigh = LocationList::GetHops(it->theMainAddr, dstAddr,SatFlow);
      if (IsValidNeighborSANSA( it->lastHello, it->interface, SatFlow, m_state.GetSatNodeValue()))
        {
	  if (HopsFromCurr > HopsNeigh)
	    {
	      count_closer++;
	    }
        }
    }
  
  for (NeighborSet::iterator it = neighbor.begin();
       it != neighbor.end(); it++)
    {
      /*bool IsNeigSatNode = false;
      std::map<Ipv4Address,uint32_t>::iterator iter = LocationList::m_SatGws.find(it->theMainAddr);
      if (iter != LocationList::m_SatGws.end())
	IsNeigSatNode = true;*/
      ///conditions to evaluate if a neighbor is valid
      if (!IsValidNeighborSANSA(it->lastHello, it->interface, SatFlow, m_state.GetSatNodeValue()))
      {
	neighbor_index++; 
	//y esto sube pero no distances vector: quizas mal? en teoría, corregido. Dejo comentario para 
	//saber posible fuente de error
	continue;     
      }
      ///
      if ( it->theMainAddr.IsEqual(dstAddr) ) 
        {
	    return it->interface;
        }
        
      uint32_t HopsFromNeigh = LocationList::GetHops(it->theMainAddr, dstAddr, SatFlow);
      distances.push_back(HopsFromNeigh);
      if (HopsFromNeigh >= HopsFromCurr)
        {
	  if (count_closer == 0)
	    penalty = false; //reward decision because there is no other closer
	  else
	    penalty = true; //Neighbor is farther or equal from the destination penalice decissions
        }
      else
        {
	  //the neighbor is closer or equal to destination
	  if (from == it->neighborhwAddr)
	    {
	      //std::cout<<"From: "<<from<<", neighbor: "<<it->neighborhwAddr<<std::endl;
      	      HopsFromNeigh = 10*HopsFromNeigh; //to avoid choosing again the same interface when there is no valid neighbor
	      distances.erase(distances.end()-1);
	      distances.push_back(HopsFromNeigh);
	      penalty = true;
	    }
	  else
	    {
	        penalty = false; //reward decisions
	        ///this approach is not working 11/02/16
	      /*if (SatFlow) // we try to push to the closer path according to the penalty of the CalculatePenaltyNeighGridSansa_v2
               { 
		 float penalty = m_state.CalculatePenaltyNeighGridSANSA_v2(m_ipv4, LocationList::GetNodeId(currAddr), dstAddr, HopsFromCurr, HopsFromNeigh, it->neighborhwAddr, from, SatFlow);
		 if (penalty == -1)
		   penalty = false;
		 else
		   penalty = true;
	       }*/
                     
	      /*if (m_ipv4->GetObject<Node>()->GetId() == 2 )
	        std::cout<<"entro aqui para vecino: "<<it->theMainAddr<<std::endl;*/
	    }
	}
	if (!penalty)
	  { //I save the neighbor with a false penalty
             theNeighGoodWeight.push_back(&(*it));
	     eqNeighs++;
	     //this variable will be used when only one neighbor
	     useful_neighbor = neighbor_index;
	  }
        else
	 {
	     theNeighBadWeight.push_back(&(*it));
	 }
	 neighbor_index++;
    }
    if (eqNeighs==0)
      {
         //erase the vectors, there is a hole
         //we select the neighbor which is closer to destination: GF 
         //the closer neighbor cannot be possible if I have already discarded        
         eqNeighs=0;
         theNeighGoodWeight.erase (theNeighGoodWeight.begin(),theNeighGoodWeight.end());
         theNeighBadWeight.erase(theNeighBadWeight.begin(), theNeighBadWeight.end());
	 uint32_t ref_distance= 100; //set to 100 hops as a higher bound
	 //search for the minimum distance
	 for (uint32_t i=0; i<distances.size(); i++)
	   {
	     if (distances[i] < ref_distance) //In this case distances constains "distances" from farther or the previous hop
	       {
	         ref_distance = distances[i];
	       }
	   }
	 for (NeighborSet::iterator it = neighbor.begin();
	      it != neighbor.end(); it++)
	    { 
	      /*bool IsNeigSatNode = false;
	      std::map<Ipv4Address,uint32_t>::iterator iter = LocationList::m_SatGws.find(it->theMainAddr);
	      if (iter != LocationList::m_SatGws.end())
	        IsNeigSatNode = true;*/
              if (!IsValidNeighborSANSA(it->lastHello, it->interface, SatFlow, m_state.GetSatNodeValue()))
	        continue; 
	      uint32_t HopsFromNeigh = LocationList::GetHops(it->theMainAddr, dstAddr, SatFlow);
	      if (HopsFromNeigh == ref_distance)
	        {
		  theNeighGoodWeight.push_back(&(*it));
	          eqNeighs++;
	        }
	    }
      }
    if (eqNeighs>1)
      { 
	//ALTERNATIVE A
	//in a GRID, the maximum number of closer neighbors is two, but let's do something more intelligent, 
	//to insert the packet in the interface with lower load instead of randomly 
	//float nlen;
	uint32_t selected_eth, nlen=0;
	uint32_t i=0;
	std::vector<uint32_t> index;
	uint64_t rate_bps, rate_bps_tmp;
	Ptr<NetDevice> aDev;
	Ptr<PointToPointNetDevice> aPtopDev;
	Ptr<WifiNetDevice> aWifiDev;
	
	for (i=0; i<eqNeighs; i++)
	  {
	    selected_eth = theNeighGoodWeight[i]->interface;
	    nlen = nlen + m_queuedData->GetIfaceSize(selected_eth);
	  }
	  
	if (nlen == 0)
	  {//all the queues are empty, so select the fastest iface
	    
	    rate_bps= 0;
	    for (i=0; i<eqNeighs; i++)
	      {
		if (theNeighGoodWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
		aDev= m_ipv4->GetNetDevice(theNeighGoodWeight[i]->interface);
	        aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
                aWifiDev = aDev->GetObject<WifiNetDevice> ();
		if (aWifiDev == NULL)
	          rate_bps_tmp = aPtopDev->GetRatebps();
	        else
	          rate_bps_tmp = aWifiDev->GetRatebps();
		if (rate_bps_tmp >= rate_bps)
		  {
		    rate_bps = rate_bps_tmp;
		    index.push_back(i);
		  }
	      }
	    
	    if (index.size()>1)
	      {
		Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
                uint32_t aNode = x->GetInteger(0,index.size()-1);
		output_interface = theNeighGoodWeight[aNode]->interface;
	       
	      }
	    else
	      output_interface = theNeighGoodWeight[index[0]]->interface;
	  }
	else
	  {
	    float tmp_ratio = 99999.99;
	    float neigh_ratio;
	    
	    for (i=0; i<eqNeighs; i++)
	      {
		if (theNeighGoodWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
	        selected_eth = theNeighGoodWeight[i]->interface;
	        aDev = m_ipv4->GetNetDevice(theNeighGoodWeight[i]->interface);
	        aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
                aWifiDev = aDev->GetObject<WifiNetDevice> ();
	        if (aWifiDev == NULL)
	          rate_bps = aPtopDev->GetRatebps();
	        else
	          rate_bps = aWifiDev->GetRatebps();
	        
		neigh_ratio =((float)m_queuedData->GetIfaceSize(selected_eth)/ (float) rate_bps);
		if ( neigh_ratio <= tmp_ratio)
	          {
		    tmp_ratio = neigh_ratio;
		    index.push_back(i);
	          }
	      }
	      
	    //std::cout<<"El ttl_2 es: "<<(int32_t)ttl<<" el vecino es: "<<theNeighSameWeight[index]->theMainAddr<<"("<<theNeighSameWeight[index]->interface<<") y la destination address: "<<dstAddr<<", la interface: "<<theNeighSameWeight[index]->interface<<std::endl;
	    if (index.size()>1)
	      {
		Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
                uint32_t aNode = x->GetInteger(0,index.size()-1);
		output_interface = theNeighGoodWeight[aNode]->interface;
	       
	      }
	    else
	      output_interface = theNeighGoodWeight[index[0]]->interface;	
	  }	  
      }
    else //number of neighbor equal to one
      { 
	//initial: only the following line and commenting the other code
	output_interface = theNeighGoodWeight[0]->interface;
///original histeresis code	
	uint32_t selected_eth = theNeighGoodWeight[0]->interface;
	if (m_queuedData->GetIfaceSize(selected_eth) < (m_queuedData->GetMaxSize()*3/4) && (!m_histeresis[selected_eth]))
	  { //original m_queuedData->GetMaxSize()/2
	    //std::cout<<"El ttl_3 es: "<<(int32_t)ttl<<" el vecino es: "<<theNeighSameWeight[0]->theMainAddr<<"("<<theNeighSameWeight[0]->interface<<") y la destination address: "<<dstAddr<<", la interface: "<<theNeighSameWeight[0]->interface<<std::endl;
	    output_interface = theNeighGoodWeight[0]->interface;	
 	  }
	else
	  { //this interface is full, so we should select other interface to place the packet 
	    //one neighbor further from destination should be chosen
	    //useful neighbor == 50 when we do not decide to reward any neighbor decision: penalty = true; 
	    if (useful_neighbor!=50)
	      { //there is one useful_neighbor (closer) but congested
	         m_histeresis[selected_eth]=true;
	         if (m_queuedData->GetIfaceSize(selected_eth)< (m_queuedData->GetMaxSize()*5/10)) //original m_queuedData->GetMaxSize()/4 
		    m_histeresis[selected_eth]=false;
		 
	         uint32_t tmp = AlternativeIfaceSelectionSANSA(m_ipv4, useful_neighbor, dstAddr);//, distances);
	         if (tmp==10)
		   {
	             output_interface = theNeighGoodWeight[0]->interface;
		   }
	         else
	           {
	             output_interface = tmp;
	             check_requeue = 2;
		   }
	      }
	    else
	      { //there is not a useful neighbor (one closer)
		///m_histeresis[selected_eth]=false;
		output_interface = theNeighGoodWeight[0]->interface;
	      }
	  }
      }  
    return output_interface;    
}


void
RoutingProtocol::MapMACAddressToIPinterface(Address MAC, uint32_t &interface)
{
  uint32_t tmp=0;
  interface = 0;
  Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
  while (tmp != theNode->GetNDevices())
    {
      if (theNode->GetDevice(tmp)->GetAddress() == MAC)
       {
	 interface = m_ipv4->GetInterfaceForDevice(theNode->GetDevice(tmp));
	 return;
       }
      tmp++;
    }
}

uint32_t 
RoutingProtocol::AlternativeIfaceSelection (Ptr<Ipv4> m_ipv4, uint32_t useful_neighbor, std::vector<double> &distances)
{
  //As the selected interface is congested, we need a neighbor further from destination to forward the packet
  uint32_t neighbor_recheck_index= 0, eqNeighs=0;
  NeighborSameLengthSet candidates_good, candidates_bad, candidate_useful;
  candidates_good.reserve(5);
  candidates_bad.reserve(5);
  candidate_useful.reserve(1); //save the useful neighbor to compare with the queues
  NeighborSet neighbor = m_state.GetNeighbors();
  std::vector<double> distance_tmp;
  for (uint32_t i=0;i< distances.size();i++)
    distance_tmp.push_back(distances[i]);
  distance_tmp.erase(distance_tmp.begin()+useful_neighbor);
  double min_distance = distance_tmp[0];
  for (uint32_t i=1; i<distance_tmp.size(); i++)
    {
      if (distance_tmp[i]<min_distance)
	min_distance = distance_tmp[i];
    }
    //std::cout<<"El valor de min distance: "<<min_distance<<std::endl;
  for (NeighborSet::iterator it = neighbor.begin();
       it != neighbor.end(); it++)
    {
      //if (!m_ipv4->IsUp(it->interface))
      if (!IsValidNeighborSANSA(it->lastHello, it->interface, true, m_state.GetSatNodeValue()) )
        {
	  //do not consider switch off ifaces but do not forge to update the counter
	  neighbor_recheck_index ++;
	  continue;
	}
      
      if (neighbor_recheck_index == useful_neighbor)
      {
	candidate_useful.push_back(&(*it));
	neighbor_recheck_index ++;
	continue;
      }
      
      if (distances[neighbor_recheck_index] == min_distance)
        { //select the neighbors farther but closer to destination
	  candidates_good.push_back(&(*it));
	  eqNeighs++;
	 }
      else
	{ //the remaining neighbor farther farther to destination
	  candidates_bad.push_back(&(*it));
	}
	//do not forget to update the counter
	neighbor_recheck_index ++;
    }
      
    if (eqNeighs == 0)
      return 10; //dummy value
    else if (eqNeighs == 1)
      {
	uint32_t min_value = m_queuedData->GetIfaceSize(candidate_useful[0]->interface);
	if ( m_queuedData->GetIfaceSize(candidates_good[0]->interface) < min_value)
          return candidates_good[0]->interface;
	else
	  return 10;
      }
    else
      {
	//we place the packet in the closest and less congested interface to destination
	uint32_t selected_eth;
	//uint32_t min_value=m_queuedData->GetMaxSize();
	uint32_t min_value = m_queuedData->GetIfaceSize(candidate_useful[0]->interface);
	uint32_t index=20;
	for (uint32_t i=0; i<eqNeighs; i++)
	  {
	    selected_eth = candidates_good[i]->interface;
	    if (m_queuedData->GetIfaceSize(selected_eth) < min_value)
	      {
		min_value = m_queuedData->GetIfaceSize(selected_eth);
		index = i;
	      }
	  }
	  if (index !=20)
	    return candidates_good[index]->interface;
	  else
	    return candidates_bad[0]->interface;
      }
}

uint32_t 
RoutingProtocol::AlternativeIfaceSelectionSANSA (Ptr<Ipv4> m_ipv4, uint32_t useful_neighbor, Ipv4Address dstAddr)//, std::vector<uint32_t> &distances)
{
  //As the selected interface is congested, we need a neighbor further from destination to forward the packet
  uint32_t neighbor_recheck_index= 0, eqNeighs=0;
  NeighborSameLengthSet candidates_good, candidates_bad, candidate_useful;
  candidates_good.reserve(5);
  candidates_bad.reserve(5);
  candidate_useful.reserve(1); //save the useful neighbor to compare with the queues
  NeighborSet neighbor = m_state.GetNeighbors();
  std::vector<uint32_t> distance_tmp;
  /*for (uint32_t i=0;i< distances.size();i++)
    distance_tmp.push_back(distances[i]);*/
  for (NeighborSet::iterator it = neighbor.begin(); it!= neighbor.end(); it++)
    {
      if (IsValidNeighborSANSA(it->lastHello, it->interface, true, m_state.GetSatNodeValue()) )
        { //we put the SatFlow flag to true to consider also satellite, in fact the idea
          //is that in these conditions we offload the traffic to satellite
	  distance_tmp.push_back(LocationList::GetHops(it->theMainAddr, dstAddr, true));
        }
    }
  
  //distance_tmp.erase(distance_tmp.begin()+useful_neighbor);
  distance_tmp[useful_neighbor]=999; //big value
  uint32_t min_distance = distance_tmp[0];
  for (uint32_t i=1; i<distance_tmp.size(); i++)
    {
      if (distance_tmp[i]<min_distance)
	min_distance = distance_tmp[i];
    }
  for (NeighborSet::iterator it = neighbor.begin();
       it != neighbor.end(); it++)
    {
      //if (!m_ipv4->IsUp(it->interface))
	if (!IsValidNeighborSANSA(it->lastHello, it->interface, true, m_state.GetSatNodeValue()) )
        {
	  //do not consider switch off ifaces but do not forge to update the counter
	  neighbor_recheck_index ++;
	  continue;
	}
      
      if (neighbor_recheck_index == useful_neighbor)
      {
	candidate_useful.push_back(&(*it));
	neighbor_recheck_index ++;
	continue;
      }
      
      if (distance_tmp[neighbor_recheck_index] == min_distance)
        { //select the neighbors farther but closer to destination
	  candidates_good.push_back(&(*it));
	  eqNeighs++;
	 }
      else
	{ //the remaining neighbor farther farther to destination
	  candidates_bad.push_back(&(*it));
	}
	//do not forget to update the counter
	neighbor_recheck_index ++;
    }
      
    if (eqNeighs == 0)
      return 10; //dummy value
    else if (eqNeighs == 1)
      {
	uint32_t min_value = m_queuedData->GetIfaceSize(candidate_useful[0]->interface);
	if ( m_queuedData->GetIfaceSize(candidates_good[0]->interface) < min_value)
          return candidates_good[0]->interface;
	else
	  return 10;
      }
    else
      {
	//we place the packet in the closest and less congested interface to destination
	uint32_t selected_eth;
	//uint32_t min_value=m_queuedData->GetMaxSize();
	uint32_t min_value = m_queuedData->GetIfaceSize(candidate_useful[0]->interface);
	uint32_t index=20;
	for (uint32_t i=0; i<eqNeighs; i++)
	  {
	    selected_eth = candidates_good[i]->interface;
	    if (m_queuedData->GetIfaceSize(selected_eth) < min_value)
	      {
		min_value = m_queuedData->GetIfaceSize(selected_eth);
		index = i;
	      }
	  }
	  if (index !=20)
	    return candidates_good[index]->interface;
	  else
	    return candidates_bad[0]->interface;
      }
}

void
RoutingProtocol::GetSourceAndDestinationPort(Ipv4Header header, Ptr<Packet> packet, uint16_t &s_port, uint16_t &d_port)
{
  switch (header.GetProtocol())
    {
      case TCP_PROT_NUMBER:
        {
	  //TCP case get source, dst port
          TcpHeader tcph;
          packet->RemoveHeader(tcph);
          s_port = tcph.GetSourcePort();
          d_port = tcph.GetDestinationPort();
	  //std::cout<<"Hola  en nodo "<<m_ipv4->GetObject<Node>()->GetId()<<" dst: "<<header.GetDestination()<<" pq entro aquí??"<<s_port<<" "<<d_port<<std::endl;
	  //exit(-1);
        }
        break;
      case UDP_PROT_NUMBER:
        {
          //UDP case get source, dst port
          UdpHeader udph;
          packet->RemoveHeader(udph);
          s_port = udph.GetSourcePort();
          d_port = udph.GetDestinationPort();
	  if (s_port == 2152 && d_port == 2152)
	    { //this a GTP tunnelling transmission
	      GtpuHeader gtph;
	      packet->RemoveHeader(gtph);
	      //right now we have to see which is the protocol beneath
	      packet->RemoveHeader(header);
	      
	      switch (header.GetProtocol())
	        {
		  case TCP_PROT_NUMBER:
		    {
		      //TCP case get source, dst port
                     TcpHeader tcph;
                     packet->RemoveHeader(tcph);
                     s_port = tcph.GetSourcePort();
                     d_port = tcph.GetDestinationPort();
		     if (d_port == 0 && s_port == 0)
		     {
		       std::cout<<" Hola en nodo "<<m_ipv4->GetObject<Node>()->GetId()<<" dst: "<<header.GetDestination()<<" pq entro aquí 2??"<<s_port<<" "<<d_port<<std::endl;
		       std::cout<<header<<std::endl;
		     }
		    }
		    break;
		  case UDP_PROT_NUMBER:
		    {
		      UdpHeader udph;
                      packet->RemoveHeader(udph);
                      s_port = udph.GetSourcePort();
                      d_port = udph.GetDestinationPort();
		    }
		    break;
		  default:
		    {
		      NS_LOG_UNCOND("Protocol unrecognized RouteInput");
		      std::cout<<"No reconozco el udp protocol en nodo "<<m_ipv4->GetObject<Node>()->GetId()<<" "<<Simulator::Now()<<std::endl;
		    }
		}
	    }
	 }
       break;
       default:
	 NS_LOG_UNCOND("Protocol unrecognized RouteInput");
	 std::cout<<"Reconozco protocolo "<<(int32_t)header.GetProtocol()<<" en enodeB "<<m_ipv4->GetObject<Node>()->GetId()-1<<" "<<Simulator::Now()<<std::endl;
    }
}



Ptr<RoutingPacketQueue> 
RoutingProtocol::GetQueuedData ()
{
  return m_queuedData;
}

} // namespace backpressure

} // namespace ns3
