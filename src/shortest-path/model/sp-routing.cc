/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Arizona
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Junseok Kim <junseok@email.arizona.edu> <engr.arizona.edu/~junseok>
 */

#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/ipv4-static-routing.h"
#include "sp-routing.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/callback.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

/***********Shortest Path Strategy ************/
#define SINGLE_PATH		0
#define MULTI_PATH 		1
/*********************************************/


using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpRouting");

NS_OBJECT_ENSURE_REGISTERED (ShortestPathRouting);

TypeId
ShortestPathRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ShortestPathRouting")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<ShortestPathRouting> ()
    .AddAttribute ("RoutingTable1", "Pointer to Routing Table 1.",
                   PointerValue (),
                   MakePointerAccessor (&ShortestPathRouting::SetRtable1),
                   MakePointerChecker<ShortestPathRoutingTable> ())
    .AddAttribute ("RoutingTable2", "Pointer to Routing Table 2.",
		   PointerValue (),
                   MakePointerAccessor (&ShortestPathRouting::SetRtable2),
                   MakePointerChecker<ShortestPathRoutingTable> ())
    .AddAttribute ("ShortestPathStrategy", "Shortest path Routing policy.",
		    EnumValue (SINGLE_PATH),
		    MakeEnumAccessor (&ShortestPathRouting::m_variant),
		    MakeEnumChecker ( SINGLE_PATH, "single",
				       MULTI_PATH, "multi"))
    .AddAttribute ("RowNodes", "Number of nodes in a row",
		    UintegerValue(5),
		    MakeUintegerAccessor (&ShortestPathRouting::m_nodesRow),
		    MakeUintegerChecker<uint32_t>())
    .AddTraceSource ("TxBeginData", "Number of packets originated at the node",
                     MakeTraceSourceAccessor (&ShortestPathRouting::m_TxBeginData),
		     "ns3::shortestpath::RoutingProtocol::TraceShortestPathParams")
    .AddTraceSource ("TxDataAir", "Time to transmit the packet over the air interface.",
		     MakeTraceSourceAccessor(&ShortestPathRouting::m_TxDataAir),
		     "ns3::shortestpath::RoutingProtocol::TraceShortestPathParams")
    ;
  return tid;
}


ShortestPathRouting::ShortestPathRouting () 
{
  m_rrobin = false;
  NS_LOG_FUNCTION_NOARGS ();
}

ShortestPathRouting::~ShortestPathRouting () 
{
  NS_LOG_FUNCTION_NOARGS ();
}

Ptr<Ipv4Route>
ShortestPathRouting::LoopbackRoute (const Ipv4Header & hdr) const
{
  //loopback interface is m_ipv4->GetNetDevice (0)
  NS_ASSERT (m_ipv4->GetNetDevice (0)!=0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  Ipv4InterfaceAddress ifAddr;
  ifAddr = m_ipv4->GetAddress (1, 0);
  
  // CODE for the multiradio setup of ICC-SDN paper, where we choose one interface 
  // horizontal communications and other interface for vertical communications
  /*uint8_t buff_i[4];
  hdr.GetDestination().Serialize(buff_i);
  if (buff_i[3] > 128)
    ifAddr = m_ipv4->GetAddress (2, 0);
  else
    ifAddr = m_ipv4->GetAddress (1, 0);*/
  rt->SetSource (ifAddr.GetLocal());
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  NS_LOG_DEBUG("LoopbackRoute: nexthop is loopback: "<<rt->GetGateway()<<" src address: "<<rt->GetSource()<<" and dst address: "<<rt->GetDestination());
  rt->SetOutputDevice (m_ipv4->GetNetDevice (0));
  return rt;
}


Ptr<Ipv4Route>
ShortestPathRouting::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, enum Socket::SocketErrno &sockerr)
{
  //FOR SANSA project (and also paper ICC-SDN 15), we changed the shortest path code to do
  //the forwarding as it is done in Backpressure RouteOutput->Loopback->RouteInput 
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId() << " " << header.GetDestination () << " " << oif);
  Ipv4Address src = m_ipv4->GetAddress (1, 0).GetLocal(); //main address
  Ipv4Address dst = header.GetDestination();
  
  // CODE for the multiradio setup of ICC-SDN paper, where we choose one interface 
  // horizontal communications and other interface for vertical communications
  /*uint8_t buff_i[4];
  dst.Serialize(buff_i);
  if (buff_i[3] >128)
    src = m_ipv4->GetAddress (2, 0).GetLocal();
  else
    src = m_ipv4->GetAddress (1, 0).GetLocal();*/
      
  NS_LOG_DEBUG ("[RouteOutput]: New Data Packet to sent :: source is "<<src<<" , destination addr is "<<dst<< " time is "<<Simulator::Now());
  /*if (p!=NULL)
  {
    m_TxBeginData(src, dst, p);			//tracing the number of data packets to be originated
  }*/
  sockerr = Socket::ERROR_NOTERROR;
  return LoopbackRoute (header); 		//every data packet is sent to the RouteInput function
  
  
}

bool
ShortestPathRouting::RouteInput  (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, 
                             UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                             LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (header.GetDestination ());
  
  uint8_t buf[4];
  header.GetDestination().Serialize(buf);
  if (buf[0] == 7)
  {
    return false; //let another routing protocol try to handle this, I am 
    //in EPC node and we will send this packet to the enodeBapp to encapsulate it
  }
  //check if I am destination
  for (uint32_t i=0; i<m_address.size(); i++)
  {
    if (header.GetDestination () == m_address[i])
    {
      NS_LOG_DEBUG ("I'm the destination");
      //std::cout<<"El ttl es: "<<(int)header.GetTtl()<<std::endl;
      lcb (p, header, m_ipv4->GetInterfaceForDevice (idev));
      return true;
    }
  else if (header.GetDestination () == m_broadcast[i])
    {
      NS_LOG_DEBUG ("It's broadcast");
      return true;
    }
  }
  
  Mac48Address MACina = Mac48Address::ConvertFrom(idev->GetAddress());
  uint32_t iface=0;
  MapMACAddressToIPinterface(MACina, iface);
  Ipv4Address src,relay;
        
  if (iface ==0) //if it is the loopback interface, it means that the packet comes from the RouteLoopback
    {
      //if the received interface is loopback we have to change this. We find the src IP
      //according to the header destination and replace the iface number. 
	
      //ICC-SDN code to have MR-SP. We have only 2 interfaces
      /*relay = header.GetDestination();
      uint8_t buff_j[4];
      relay.Serialize(buff_j);
      if (buff_j[3] > 128) 
	src = m_ipv4->GetAddress(2,0).GetLocal();
      else
	src = m_ipv4->GetAddress(1,0).GetLocal();*/
	    
      /*for (uint32_t i=1; i< m_ipv4->GetNInterfaces(); i++)
        {
	  //loop to determine the interface in src
	  if (m_ipv4->IsUp(i))
            {
	      //to determine the source/interface ip which is going to forward the packet
	      src_tmp = m_ipv4->GetAddress (i, 0).GetLocal();
	      //std::cout<<"El broadcast de esta interficie es: "<<m_ipv4->GetAddress (i, 0).GetBroadcast()<<std::endl;
	      switch (m_variant)
	      {
		case (0):
	          relay = m_rtable1->LookupRoute (src_tmp, header.GetDestination ());
		  std::cout<<"El valor de src_tmp es: "<<src_tmp<<" y relay es: "<<relay<<std::endl;
	          break;
                case (1):
	          if (!m_rrobin)
	            {
		      relay= m_rtable1->LookupRoute (src_tmp, header.GetDestination ());
		    }
		  else 
		    {
		      relay= m_rtable2->LookupRoute (src_tmp, header.GetDestination ());
		    }
	          break;
		default:
		  std::cout<<"m_variant not working"<<std::endl;
	      }
	      uint8_t buff_i[4], buff_j[4];
	      src_tmp.Serialize(buff_i);
	      relay.Serialize(buff_j);
	      int32_t diffNet = (int32_t)buff_i[2] - (int32_t)buff_j[2];;
	      if (diffNet==0 && src_tmp != relay)
	      {
		src = src_tmp;
	      }
	    }
	}*/
      
      if (m_ipv4->GetObject<Node>()->GetId() == 0 && header.GetDestination().IsEqual("1.0.0.2") )
        {  //we select the interface with the 1.0.0.1 address: SANSA+LENA integration
	   src = m_ipv4->GetAddress (2, 0).GetLocal();
	}
      else
	{//the src interface selected is the first one which is up
	  for (uint32_t i=1; i< m_ipv4->GetNInterfaces(); i++)
          {
	    if (m_ipv4->IsUp(i))
	      {
	        src = m_ipv4->GetAddress (i, 0).GetLocal();
	      }
	    break;
	  }
	}	
    } //if (iface != 0)
  else
    {
      src = m_ipv4->GetAddress (iface, 0).GetLocal();
    }
      
  switch (m_variant)
    {
      case (0):
	{
	  //ICC-SDN code to have MR-SP.
	  /*relay = header.GetDestination();
	  uint8_t buff_j[4];
	  relay.Serialize(buff_j);
	  if (buff_j[3] > 128) 
	    relay = m_rtable2->LookupRoute (src, header.GetDestination ());
	  else
	    relay = m_rtable1->LookupRoute(src, header.GetDestination ());*/
	  //LENA INTEGRATION
	  if (header.GetDestination().IsEqual("1.0.0.2"))
	    {
	      relay = Ipv4Address("1.0.0.2");
	      iface = m_ipv4->GetInterfaceForAddress(Ipv4Address("1.0.0.1"));
	    }
	  else
	    {
	      relay = m_rtable1->LookupRoute (src, header.GetDestination ()); 
	      //SANSA-LENA integration
	      iface= FindIfaceAssociatedtoRelayLena(relay);
	      if (iface==0)
	        NS_LOG_DEBUG ("Can't find a route!!");
	      //square/rectangular grid without LENA integration
	      //iface = FindIfaceAssociatedtoRelay (relay);
	    }
	  //std::cout<<"El src es: "<<src<< "El relay es: "<<relay<<" y la iface: "<<iface<<" la destination: "<<header.GetDestination()<<std::endl;
	}
	break;
      case (1):
	{
	  if (!m_rrobin)
	    {
	      relay= m_rtable1->LookupRoute (src, header.GetDestination ());
	      //SANSA-LENA integration
	      iface= FindIfaceAssociatedtoRelayLena(relay);
	      if (iface==0)
	        NS_LOG_DEBUG ("Can't find a route!!");
	      //square/rectangular grid without LENA integration
	      //iface = FindIfaceAssociatedtoRelay (relay);
	      m_rrobin = true;
	    }
	  else 
	    {
	      relay= m_rtable2->LookupRoute (src, header.GetDestination ());
	      //SANSA-LENA integration
	      iface= FindIfaceAssociatedtoRelayLena(relay);
	      if (iface==0)
	        NS_LOG_DEBUG ("Can't find a route!!");
	      //square/rectangular grid without LENA integration
	      //iface = FindIfaceAssociatedtoRelay (relay);
	      m_rrobin = false;
	    }
	}
	break;
	default:
	 std::cout<<"m_variant not working"<<std::endl;
    }
  
  NS_LOG_FUNCTION (this << src << "->" << relay << "->" << header.GetDestination ());
  NS_LOG_DEBUG ("Relay to " << relay);
  if (src == relay)
    {
      NS_LOG_DEBUG ("Can't find a route!!");
    }
  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
  route->SetGateway (relay);
  route->SetSource (header.GetSource ());
  route->SetDestination (header.GetDestination ());
  route->SetOutputDevice (m_ipv4->GetNetDevice (iface));
  {
    Ptr<NetDevice> aDev = route->GetOutputDevice();
    Ptr<PointToPointNetDevice> aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
    Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
    int64_t rate_bps;
    int32_t mode; //1 wifi, 2 ppp
    if (aWifiDev==NULL)
      { //ppp device
        rate_bps = aPtopDev->GetRatebps();
        mode = 2;
      }
    else
      { //wifi device
        rate_bps = aWifiDev->GetRatebps();
        mode = 1;
      }
     m_TxDataAir(p, header, rate_bps,mode);
     if (header.GetTtl() == 64)
       {
	  Ipv4Address dst = header.GetDestination();
	  Ipv4Address src = header.GetSource();
	  m_TxBeginData(src, dst, p, header, rate_bps, mode);			//tracing the number of data packets to be originated
	}
  }     
  ucb (route, p, header);
  return true;
}

void 
ShortestPathRouting::NotifyInterfaceUp (uint32_t interface)
{
  NS_LOG_FUNCTION (this << interface);
}
void 
ShortestPathRouting::NotifyInterfaceDown (uint32_t interface)
{
  NS_LOG_FUNCTION (this << interface);
}
void 
ShortestPathRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION(this << interface << address << m_rtable1);
  NS_LOG_FUNCTION(this << interface << address << m_rtable2);
  m_ifaceId.push_back(interface);
  m_address.push_back(address.GetLocal ());
  //std::cout<<"La local address = "<<m_address<<"y la interface: "<<m_ifaceId<<"en node"<<m_ipv4->GetObject<Node>()->GetId()+1<<std::endl;
  m_broadcast.push_back(address.GetBroadcast ());
}
void 
ShortestPathRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION(this << interface << address);
}
void 
ShortestPathRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_FUNCTION(this << ipv4);
  m_ipv4 = ipv4;
}
void
ShortestPathRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId ();
  m_rtable1->Print (stream);
  m_rtable2->Print (stream);
}
void
ShortestPathRouting::SetRtable1 (Ptr<ShortestPathRoutingTable> p)
{
  NS_LOG_FUNCTION(p);
  m_rtable1 = p;
}

void
ShortestPathRouting::SetRtable2 (Ptr<ShortestPathRoutingTable> p)
{
  NS_LOG_FUNCTION(p);
  m_rtable2 = p;
}

uint32_t
ShortestPathRouting::FindIfaceAssociatedtoRelay (Ipv4Address relay)
{
  //this function finds the interface number of the current node which is in contact with the relay
  //in a square/rectangular grid
  
  //check first if we are point to point 
  Ptr<NetDevice> aDev = m_ipv4->GetNetDevice (1);
  Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
  
  if (aWifiDev!=NULL)
  {
    //we are using wifi devices single radio, so we return the unique useful interface
    //if we use PtoP devices, we are in a grid. (code from ICC BP-SDN 15)
    uint8_t buff_i[4];
    relay.Serialize(buff_i);
    if (buff_i[3]>128)
      return 2;
    else 
      return 1;
  }
    
  
  uint8_t buff_curr[4];
  uint8_t buff_relay[4];
  m_ipv4->GetAddress (1, 0).GetLocal().Serialize(buff_curr);
  relay.Serialize(buff_relay);
  int32_t diffNet=0;
  //uint32_t id_relay=105;
  uint32_t direction=0; //1W, 2E, 3S, 4N
  uint32_t id_local= m_ipv4->GetObject<Node>()->GetId();
  uint32_t interface=1;
  diffNet= (int32_t)buff_relay[2] - (int32_t)buff_curr[2];
  if (diffNet==0)
    {
      if (((int32_t)buff_relay[3]-(int32_t)buff_curr[3]) > 0)
	//id_relay = id_local + 1;
      direction = 2;
      else
	//id_relay = id_local - 1;
      direction = 1;
    }
  else if (diffNet== -1)
    {
	//id_relay = id_local - 1;
	direction = 1;	
    }
  else if (diffNet == 1)
    {
       //id_relay = id_local + 1;
       direction = 2;
    }
  else if (diffNet == (int32_t)m_nodesRow - 1)
    {
       //id_relay = id_local + m_nodesRow;
       direction = 4;
    }
  else if (diffNet == 1 - (int32_t)m_nodesRow)
    {
       //id_relay = id_local - m_nodesRow;
       direction = 3;
    }
    
    switch (direction)
    {
      case 1: //W
	interface = 1; //la idea es leerlo del vector de address
	break;
      case 2: //E
	if (id_local % m_nodesRow == 0)
	  interface = 1;
	else
	  interface = 2;
	break;
      case 3: //S
	  if (m_ipv4->GetNInterfaces() >4)
	    interface = 3;
	  else
	    interface = 2;
	break;
      case 4: //N
         if (m_ipv4->GetNInterfaces() ==5)
	   interface = 4;
	 else if (m_ipv4->GetNInterfaces() ==4) //be careful: loopback interface
	   interface = 3;
	 else
	   interface = 2;
	break;
    }
    
    return interface;
}

uint32_t
ShortestPathRouting::FindIfaceAssociatedtoRelayLena (Ipv4Address relay)
{
  uint32_t id = LocationList::GetNodeId(relay);
  int32_t diffNet;
  Ptr<Ipv4> relay_ipv4= NodeList::GetNode(id)->GetObject<Ipv4>();
  uint8_t buf_local[4], buf_relay[4];
  for (uint32_t i =1; i< m_ipv4->GetNInterfaces(); i++)
    {
      m_ipv4->GetAddress(i,0).GetLocal().Serialize(buf_local);
      for (uint32_t j=1; j<relay_ipv4->GetNInterfaces(); j++)
        {
	  relay_ipv4->GetAddress(j,0).GetLocal().Serialize(buf_relay);
	  diffNet= (int32_t)buf_local[3] - (int32_t)buf_relay[3];
	  if (diffNet==1 || diffNet==-1)
	    {
	      if (m_ipv4->IsUp(i) && relay_ipv4->IsUp(j))
	        {//we have found our link point
		  return m_ipv4->GetInterfaceForAddress(m_ipv4->GetAddress(i,0).GetLocal());
	        }
	    }
        }
    }
    return 0;
  
}

void
ShortestPathRouting::MapMACAddressToIPinterface(Mac48Address MAC, uint32_t &interface)
{
  uint32_t tmp=0;
  Ptr<Node> theNode = m_ipv4->GetObject<Node> ();
  while (tmp != theNode->GetNDevices()-1)
    {
      Mac48Address tmp_mac = Mac48Address::ConvertFrom(theNode->GetDevice(tmp)->GetAddress());
      if (tmp_mac == MAC)
       {
	 interface = m_ipv4->GetInterfaceForDevice(theNode->GetDevice(tmp));
	 return;
       }
      tmp++;
    }
}




} // namespace ns3


