/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 José Núñez
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
 * Authors: José Núñez  <jose.nunez@cttc.cat>
 *         
 */

///
/// \file	BackpressureState.cc
/// \brief	Implementation of all functions needed for manipulating the internal
///		state of an BACKPRESSURE node.
///

#include "ns3/simulator.h"
#include "ns3/integer.h"
#include "backpressure-state.h"
#include "ns3/location-list.h"
#include "ns3/random-variable-stream.h"
#include "ns3/node.h"
#include "ns3/ipv4.h"
#include "ns3/node-list.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/backpressure.h"

#define MAX_NEIGHBORS 10

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BackpressureState");
/********** Neighbor Set Manipulation **********/

Ipv4Address
BackpressureState::FindNextHopBasic (Ipv4Address const &srcAddr, Ipv4Address const &dstAddr)
{
  uint32_t length=1400;
  Ipv4Address theNextHop;

  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
  {
    if ( it->neighborMainAddr.IsEqual(dstAddr) )
    {
      return (it->neighborMainAddr);
    }
    if ( it->qLength < length )
    { 
      theNextHop = it->neighborMainAddr;
      length=it->qLength;
    }
  }
  return (theNextHop);
}

void
BackpressureState::PrintNeighInfo(uint32_t anode)
{
  if (anode==0)
  {
    NS_LOG_UNCOND("START");
    for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      //NS_LOG_UNCOND("Node "<<anode<<" has a Neigh with queue length "<<it->qLength<<" and ip is "<<it->neighborMainAddr<< " last hello in "<<it->lastHello<<" time is "<<Simulator::Now());
        NS_LOG_UNCOND("Node: "<<anode<<" originator: "<<it->theMainAddr<<" neigInt: "<<it->neighborMainAddr<<" receiver IP: "<<it->localIfaceAddr);
    }
    NS_LOG_UNCOND("END");
  }
}

//
// Set Time neighbor is considered valid without receiving HELLO broadcast messages
//
void
BackpressureState::SetNeighExpire(const Time timer)
{
  m_neighValid = timer;
}


/* Used For Backpressure Collection Protocol
void
PrintSameLength(NeighborSameLengthSet &theNeighSameLength, anode);
{
  if (anode==0){
  for (NeighborSameLengthSet::iterator it = theNeighSameLength.begin();
       it != theNeighSameLength.end(); it++)
  {
    NS_LOG_UNCOND("Same Length Node 0: "<<" has queue length "<<it->qLength<<" and ip is "<<it->neighborMainAddr);
  }
  }
}
*/

//
// Implementation of the Backpressure Collection Protocol: ToReview
//

Ipv4Address
BackpressureState::FindNextHopBcp (Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t myLength, uint32_t anode)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; // nodes with the same length
  theNeighSameWeight.reserve(MAX_NEIGHBORS);

  uint32_t eqNeighs = 0;				        //number of neighs with same queue length size 
  double maxWeight  = 0.0;					//Weight of the current node
							  	//maximum backpressure weight algorithm
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
  {
    int32_t diffBcklg = myLength - it->qLength; 		//queue differential
    double etx        = 1.0;    				//all links are equal in terms of cost
    double rateLink   = 1.0;					//all the rates have the same capacity
    double V        = 0;
    double currWeight = (diffBcklg - (V*etx)) * rateLink;
    NS_LOG_DEBUG("[FindNextHopBcp]: currweight calculated is "<<currWeight);
    
    if (currWeight == maxWeight)   	               //PUT the neighbor in LIST OF CANDIDATE NEIGHBORS
    {
      NS_LOG_DEBUG("[FindNextHopBcp]: neighbor of the same length "<<it->qLength);
      if (maxWeight > 0.0){
        theNeighSameWeight.push_back(&(*it));
        eqNeighs++;
      }
    }
    else if ( currWeight > maxWeight )		//PUT the FIRST  in  LIST OF CANDIDATE NEIGHBORS
    { 
      //delete Neighbors of the same previous weight
      theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end());
      //add the first element
      theNeighSameWeight.push_back(&(*it));
      eqNeighs  = 1;
      maxWeight = currWeight;
    } 
  }

  if ( eqNeighs > 1 )
  {						//SOME CANDIDATE NEIGHBORS WITH THE SAME WEIGHT
      Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> (); 
      x->SetAttribute("Min", IntegerValue(0));
      x->SetAttribute("Max", IntegerValue(theNeighSameWeight.size()-1));
      //PrintSameLength(theNeighSameLength, anode);
      uint32_t aNode = x->GetInteger();
      NS_LOG_DEBUG("[FindNextHopBcp]: ip dst address in case of draw "<<theNeighSameWeight[aNode]->neighborMainAddr);
      NS_LOG_DEBUG("[FindNextHopBcp]: next hop is sameLength "<<theNeighSameWeight[aNode]->neighborMainAddr);
      return (theNeighSameWeight[aNode]->neighborMainAddr);
  }
  else if (eqNeighs==1)
  {						//ONE CANDIDATE
    NS_LOG_DEBUG("[FindNextHopBcp]: node : "<<anode<<" nexthop only one candidate "<<theNeighSameWeight[0]->neighborMainAddr<<" length is "<<theNeighSameWeight[0]->qLength<<" equal neighs "<<eqNeighs);

    return (theNeighSameWeight[0]->neighborMainAddr);
  }
  else 
  {   						//DO NOT FORWARD THE PACKET, KEEP IT
      NS_LOG_DEBUG ("[FindNextHopBcp]: Do not forward; the current node keeps the packet");
      return Ipv4Address ("127.0.0.1");
  }
}

//previous code taking into account the priority of the routing protocol
uint32_t 
BackpressureState::GetQlengthCentralized(uint32_t id,Ipv4Address &addr)
{

  Ptr<Node> node14 = NodeList::GetNode(id);
  Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
  Ptr<Ipv4ListRouting> listRouting = DynamicCast<Ipv4ListRouting> (rproto);
  addr = Ipv4Address();
  for (uint32_t j=0; j< listRouting->GetNRoutingProtocols(); j++){
     int16_t priority;
     Ptr<Ipv4RoutingProtocol> temp = listRouting->GetRoutingProtocol (j, priority);
     if (priority==10){
          Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (temp);
          uint32_t length = back->GetDataQueueSize();
	  //uint32_t length = back->GetDataQueueSizeDirection(direction);
          addr = (ipv4->GetAddress(1,0).GetLocal());
          return length;
       }
  }
  return -1;
}

/*uint32_t 
BackpressureState::GetQlengthCentralized(uint32_t id,Ipv4Address &addr)
{
  Ptr<Node> node14 = NodeList::GetNode(id);
  Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
  addr = Ipv4Address();
  Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (rproto);
  uint32_t length = back->GetDataQueueSize();
  addr = (ipv4->GetAddress(1,0).GetLocal());
  return length;  
}*/


//previous code taking into account the priority of the routing protocol
void 
BackpressureState::UpdateIfaceQlengthCentralized (uint32_t id, Ipv4Address addr)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (it->theMainAddr.IsEqual(addr))
        {
	  Ptr<Node> node14 = NodeList::GetNode(id);
          Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
          Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
          Ptr<Ipv4ListRouting> listRouting = DynamicCast<Ipv4ListRouting> (rproto);
	  for (uint32_t j=0; j< listRouting->GetNRoutingProtocols(); j++)
	    {
	      int16_t priority;
	      Ptr<Ipv4RoutingProtocol> temp = listRouting->GetRoutingProtocol (j, priority);
	      if (priority==10)
	      {
		Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (temp);
		for (uint32_t k=0; k< it->n_interfaces; k++)
		  {
		    if (it->qLength_eth[k] !=255)
		      it->qLength_eth[k]= back->GetVirtualDataQueueSize((uint8_t)k);
		  }
	      }
	    }
	}
      else
	  continue;
    }
}


/*void 
BackpressureState::UpdateIfaceQlengthCentralized (uint32_t id, Ipv4Address addr)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (it->theMainAddr.IsEqual(addr))
        {
	  Ptr<Node> node14 = NodeList::GetNode(id);
          Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
          Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
          Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (rproto);
	  for (uint32_t k=0; k< it->n_interfaces; k++)
	    {
	       if (it->qLength_eth[k] !=255)
		  it->qLength_eth[k]= back->GetVirtualDataQueueSize((uint8_t)k);
	     }
	}
      else
	  continue;
    }
}*/

	
//previous code taking into account the priority of the routing protocol
uint32_t 
BackpressureState::GetVirtualQlengthCentralized(uint32_t id,Ipv4Address &addr, uint32_t direction)
{

  Ptr<Node> node14 = NodeList::GetNode(id);
  Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
  Ptr<Ipv4ListRouting> listRouting = DynamicCast<Ipv4ListRouting> (rproto);
  addr = Ipv4Address();
  for (uint32_t j=0; j< listRouting->GetNRoutingProtocols(); j++){
     int16_t priority;
     Ptr<Ipv4RoutingProtocol> temp = listRouting->GetRoutingProtocol (j, priority);
     if (priority==10){
          Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (temp);
          //uint32_t length = back->GetDataQueueSize();
	      uint32_t length = back->GetVirtualDataQueueSize(direction);
          addr = (ipv4->GetAddress(1,0).GetLocal());
          return length;
       }
  }
  return -1;
}

/*uint32_t 
BackpressureState::GetVirtualQlengthCentralized(uint32_t id,Ipv4Address &addr, uint32_t direction)
{

  Ptr<Node> node14 = NodeList::GetNode(id);
  Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
  addr = Ipv4Address();
  Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (rproto);
  uint32_t length = back->GetVirtualDataQueueSize(direction);
  addr = (ipv4->GetAddress(1,0).GetLocal());
  return length;
}*/



//
//  Given the node identifier return the current queue backlog and the address
//  
//
/*uint32_t 
BackpressureState::getQlengthCentralized(uint32_t id,Ipv4Address &addr, uint32_t &vcong)
{

  Ptr<Node> node14 = NodeList::GetNode(id);
  Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
  Ptr<Ipv4ListRouting> listRouting = DynamicCast<Ipv4ListRouting> (rproto);
  addr = Ipv4Address();
  for (uint32_t j=0; j< listRouting->GetNRoutingProtocols(); j++){
     int16_t priority;
     Ptr<Ipv4RoutingProtocol> temp = listRouting->GetRoutingProtocol (j, priority);
     if (priority==10){
          Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (temp);
          uint32_t length = back->GetDataQueueSize();
          vcong = back->GetShadow();
          //if (id==10){
          //NS_LOG_UNCOND("vcong node in hole: "<<vcong);
          //}
          addr = (ipv4->GetAddress(1,0).GetLocal());
          return length;
       }
  }
  return -1;
}*/

////////////////////////m_shadow version//////////////////////////////////////
//previous code taking into account the priority of the routing protocol
uint32_t 
BackpressureState::getQlengthCentralized(uint32_t id,Ipv4Address &addr, uint32_t &vcong, Ipv4Address dstAddr)
{

  Ptr<Node> node14 = NodeList::GetNode(id);
  Ptr<Ipv4> ipv4 = node14->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> rproto = ipv4->GetRoutingProtocol();
  Ptr<Ipv4ListRouting> listRouting = DynamicCast<Ipv4ListRouting> (rproto);
  addr = Ipv4Address();
  for (uint32_t j=0; j< listRouting->GetNRoutingProtocols(); j++){
     int16_t priority;
     Ptr<Ipv4RoutingProtocol> temp = listRouting->GetRoutingProtocol (j, priority);
     if (priority==10){
          Ptr<ns3::backpressure::RoutingProtocol> back = DynamicCast<ns3::backpressure::RoutingProtocol> (temp);
          uint32_t length = back->GetDataQueueSize();
          vcong = back->GetShadow(dstAddr);
          addr = (ipv4->GetAddress(1,0).GetLocal());
          return length;
       }
  }
  return -1;
}

Ipv4Address
BackpressureState::FindNextHopGreedyForwardingSinglePath(Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, Vector currPos, uint32_t &iface)
{
  Vector dPos = LocationList::GetLocation(dstAddr);                     // Get coordinates from the destination
  double eucDistFromSrc = CalculateDistance(dPos, currPos);         // Distance to the destination from the current node 
  double eucDistCloser = eucDistFromSrc;                        // Initialize the distance closer to the destination with the distance from the current node
  
  Ipv4Address theNextHop;                              // Keep the candidate

  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeigh = CalculateDistance(dPos, nPos);
      if (eucDistFromNeigh<eucDistCloser)
	eucDistCloser = eucDistFromNeigh;
    }
    
   for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeigh = CalculateDistance(dPos, nPos);
      if (eucDistFromNeigh == eucDistCloser)
        {
	  //we have found the neighbor which is closer to destination from the current pos
	  iface=it->interface;
          theNextHop=it->neighborMainAddr; // delete candidate Neighbor
          return (theNextHop);
        }
    }
  //NS_LOG_UNCOND("Greedy Forwarding output iface: "<<iface<<" next hop:"<<theNextHop);
  return (Ipv4Address("127.0.0.1"));
}


Ipv4Address
BackpressureState::FindNextHopGreedyForwardingMultiPath(Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, Vector currPos, uint32_t &iface)
{
  //MP approach: random
  Vector dPos = LocationList::GetLocation(dstAddr);                     // Get coordinates from the destination
  double eucDistFromSrc = CalculateDistance(dPos, currPos);         // Distance to the destination from the current node 
  double eucDistCloser = eucDistFromSrc;                        // Initialize the distance closer to the destination with the distance from the current node
  
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(MAX_NEIGHBORS);				// Up to MAX_NEIGHBORS
  uint32_t eqNeighs=0;
  
  //Ipv4Address theNextHop;                              // Keep the candidate

  //find the minimum distance from the currPos to dstPos
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeigh = CalculateDistance(dPos, nPos);
      if (eucDistFromNeigh<eucDistCloser)
	eucDistCloser = eucDistFromNeigh;
    }
    
   //check the neighbors with the same distance
   for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeigh = CalculateDistance(dPos, nPos);
      if (eucDistFromNeigh == eucDistCloser)
        {
	  theNeighSameWeight.push_back(&(*it));
          eqNeighs++;
        }
    }
    
   //select the neighbor
   if (eqNeighs == 1)
     {
       iface = theNeighSameWeight[0]->interface;
       return (theNeighSameWeight[0]->neighborMainAddr);
     }
   else if (eqNeighs >1)
     {
        Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> (); 
	x->SetAttribute("Min", IntegerValue(0));
	x->SetAttribute("Max", IntegerValue(theNeighSameWeight.size()-1)); // Some Candidate Neighbors can be next-hop 
        uint32_t aNode = x->GetInteger();
        //NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
        iface = theNeighSameWeight[aNode]->interface;
        return (theNeighSameWeight[aNode]->neighborMainAddr);
     }
      
  //NS_LOG_UNCOND("Greedy Forwarding output iface: "<<iface<<" next hop:"<<theNextHop);
  return (Ipv4Address("127.0.0.1"));
}


//Try to replicate the penalty computation in the grid environment
//First steps towards sparse multi-radio 
float 
BackpressureState::CalculatePenaltyNeighGrid(uint32_t anode, Vector dPos, uint32_t ttl, double distFromCurr, double distFromNeigh, const Mac48Address &hwNeigh, const Mac48Address &from, Ipv4Address dstAddr)
{
  bool closer_is_loop=false, any_closer_on = false;
  Time now = Simulator::Now();		// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t count_closer=0;
  double minDistance=99999.9;
  std::map<Ipv4Address,uint32_t>::iterator it;  

  for (NeighborSet::iterator it = m_neighborSet.begin();//detect whether neighbor closer is OFF or not
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeighToDst = CalculateDistance(dPos, nPos);
      if ((now.GetSeconds()-it->lastHello.GetSeconds())<1.0)
        { // ALIVE_Neighbor
        
          if (eucDistFromNeighToDst <=minDistance)
		minDistance = eucDistFromNeighToDst;
          
	  if (distFromCurr>=eucDistFromNeighToDst)
            {
              count_closer++;    
              if (from == it->neighborhwAddr)
	        {
                  closer_is_loop=true;
                }
              any_closer_on=true;
            }
        }
    }


  float penalty;
  if (distFromNeigh > distFromCurr)
    {//the neighbor is farther from destination
      if ( (!any_closer_on) )
        {
	  if (from == hwNeigh)
            {
              penalty  = 0.0; // backpressure will direct the packet to it
                    //penalty  = +1.0; // backpressure will direct the packet to it
            }
            else
            {
		/*if (distFromNeigh== minDistance)
                  penalty = -1.0; // reward if node not visited though farther
                else
		  penalty = 0.0;	//backpressure will direct the packet to it*/
		penalty = -1.0;
             }
         }
       else
       {	//some neighbor is closer I should check if that one causes loop if the one closer is loop I should reward
         if ( (count_closer==1) && (closer_is_loop) )
           {
             penalty = -1.0;   //reward the farther if the neighbor closer congestion is a loop
           }
         else
           {
             penalty = 1.0; // penalize the farther if closer is not explored yet
           }
        }
     }
  else
    { //the neighbor is closer or equal to destination
      if (from == hwNeigh)//avoid routing loop
        {
 	  NS_LOG_DEBUG("avoid Loop Node:"<<anode<<" from:"<<from<<" NeighTo:"<<hwNeigh);
          penalty = +0.0;//penalize routing decission that motivate loops
          //penalty = +1.0;//penalize routing decission that motivate loops
         }
       else
         {
	   /*if (distFromNeigh == minDistance)
	      penalty = -1.0;   //no loops; reward routing decissions
            else
	      penalty = 0.0; //backpressure will direct the packet to it*/
	   penalty = -1.0;
          }
          
      }
  return penalty;
}

////////////////
//Penalty computation in the SANSA environment, where distance is measured in hops to node connected to gw
/////////////////////////////
//This version takes into account all kind of traffics
float 
BackpressureState::CalculatePenaltyNeighGridSANSA_v2(Ptr<Ipv4> m_ipv4, uint32_t anode, Ipv4Address dstAddr, double HopsFromCurr, double HopsFromNeigh, const Mac48Address &hwNeigh, const Mac48Address &from, bool SatFlow)
{
  bool closer_is_loop=false, any_closer_on = false;
  Time now = Simulator::Now();		// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t count_closer=0;
  std::map<Ipv4Address,uint32_t>::iterator it;  

  for (NeighborSet::iterator it = m_neighborSet.begin();//detect whether neighbor closer is OFF or not
       it != m_neighborSet.end(); it++)
    {
      uint32_t HopsNeigh = LocationList::GetHops(it->theMainAddr, dstAddr, SatFlow);
      //if ((now.GetSeconds()-it->lastHello.GetSeconds())<1.0)
      if (IsValidNeighborSANSA( m_ipv4 ,it->lastHello, it->interface, SatFlow))
        { //ALIVE neighbor
	 //uint32_t HopsNeigh = LocationList::GetHops(it->theMainAddr, dstAddr, SatFlow);
	 //if (HopsFromCurr>=HopsNeigh)
	   if (HopsFromCurr> HopsNeigh)
             {
                count_closer++;    
                if (from == it->neighborhwAddr)
	          {
                    closer_is_loop=true;
                  }
                any_closer_on=true;
             }
         }
    }
 

  float penalty;
  if (HopsFromNeigh >= HopsFromCurr)
  //if (HopsFromNeigh > HopsFromCurr)
    {//the neighbor is farther or equal from destination
      if ( (!any_closer_on) )
        {
	  if (from == hwNeigh)
            {
              penalty  = 0.0; // backpressure will direct the packet to it
            }
          else
            {
		penalty = -1.0;
            }
         }
      else
       {	//some neighbor is closer I should check if that one causes loop if the one closer is loop I should reward
         if ( (count_closer==1) && (closer_is_loop) )
           {
             penalty = -1.0;   //reward the farther if the neighbor closer congestion is a loop
           }
         else
           {
             penalty = 1.0; // penalize the farther if closer is not explored yet
           }
        }
     }
  else
    { //the neighbor is closer to destination
      if (from == hwNeigh)//avoid routing loop
        {
 	  NS_LOG_DEBUG("avoid Loop Node:"<<anode<<" from:"<<from<<" NeighTo:"<<hwNeigh);
          penalty = +0.0;//penalize routing decission that motivate loops
        }
      else
        {
	  penalty = -1.0;
        }
          
      }
  return penalty;
}

//////////////////////////
float
BackpressureState::CalculatePenaltyNeigh(uint32_t anode, Vector dPos, uint32_t pen_strat, uint32_t ttl, double distFromCurr, double distFromNeigh, double distFromCurrToSrc, double distFromNeighToSrc, uint32_t virt_cong, const Mac48Address &hwNeigh, const Mac48Address &from, Ipv4Address dstAddr)
{
  bool closer_is_loop=false, any_closer_on = false;
  Time now = Simulator::Now();		// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t count_closer=0;
  double minDistance=99999.9;
  //double maxDistance=0;
std::map<Ipv4Address,uint32_t>::iterator it;  

  for (NeighborSet::iterator it = m_neighborSet.begin();//detect whether neighbor closer is OFF or not
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeighToDst = CalculateDistance(dPos, nPos);
      if ((now.GetSeconds()-it->lastHello.GetSeconds())<3.0)
        { // ALIVE_Neighbor
        
          if (eucDistFromNeighToDst <=minDistance)
		minDistance = eucDistFromNeighToDst;
          
	  if (distFromCurr>=eucDistFromNeighToDst)
            {
              count_closer++;
	      
	      //if (eucDistFromNeighToDst>=maxDistance)
		//maxDistance= eucDistFromNeighToDst;
	            
              if (from == it->neighborhwAddr)
                {
                  closer_is_loop=true;
                }
              any_closer_on=true;
            }
        }
    }

if (any_closer_on)
    {
      it= m_shadow.find(dstAddr);
      if ( it!=m_shadow.end())
        {
	      m_shadow.erase (it);
	      //std::cout<<"estoy borrando una entrada en la shadow queue del nodo: "<<anode<<" en el tiempo:"<<Simulator::Now().GetSeconds()<<std::endl;
        }
    }

  float penalty=0.0;
  if (distFromNeigh > distFromCurr)
    {//the neighbor is farther from destination
      switch (pen_strat)
        {
          case (1):
          case (4):
            penalty = 1.0;   //penalice
            break;
          case (2):
          case (5):
	    {
         
	      if ( (!any_closer_on) )
                {
                  penalty  = -200.0; //reward decissions
                  NS_LOG_DEBUG("increment shadow: "<<200<<" node:"<<anode);
		  it = m_shadow.find(dstAddr);
		  if (it == m_shadow.end())
		    {
		      m_shadow.insert(std::pair<Ipv4Address, uint32_t> (dstAddr,200));
		            //std::cout<<"Pongo la shadow a 200 en node: "<<anode<<" y el dstAddr: "<<dstAddr<<std::endl;
		    }
                }
            else
	        {
	             //std::cout<<"Soy el node: "<<anode<<" y tengo la penalty a 0 para el vecino: "<<distFromNeigh<<std::endl;
	             penalty = 0.0; //use the drift
	        }
            break;
	    }
          case (3):
          case (6):
	  {
            if ( (!any_closer_on) )
              {
		if (from == hwNeigh)
                  {
                    penalty  = 0.0; // backpressure will direct the packet to it
                    //penalty  = +1.0; // backpressure will direct the packet to it
                  }
                else
                  {
		    /*if (distFromNeigh== minDistance)
                      penalty = -1.0; // reward if node not visited though farther
                    else
		       penalty = 0.0;	//backpressure will direct the packet to it*/
		    penalty = -1.0;
                  }
                //NS_LOG_DEBUG("increment shadow: "<<m_shadow<<" node:"<<anode);
                //m_shadow = 200;    //store state
              }
            else
              {	//some neighbor is closer I should check if that one causes loop if the one closer is loop I should reward
                if ( (count_closer==1) && (closer_is_loop) )
                  {
                    penalty = -1.0;   //reward the farther if the neighbor closer congestion is a loop
                  }
                else
                  {
                    penalty = 1.0; // penalize the farther if closer is not explored yet
                  }
              }
            break;
	  }
        }
    }
  else
    { //the neighbor is closer to the destination
      //get shadow from neighbor
      switch (pen_strat)
      {
        case (1):
        case (4):
          penalty=-1.0;
          break;
        case (2):
        case (5):
      	  penalty=-1.0 + virt_cong;
          break;
        case (3):
        case (6):
	{
          if (from == hwNeigh)//avoid routing loop
            {
 	      NS_LOG_DEBUG("avoid Loop Node:"<<anode<<" from:"<<from<<" NeighTo:"<<hwNeigh);
              penalty = +0.0;//penalize routing decission that motivate loops
              //penalty = +1.0;//penalize routing decission that motivate loops
            }
          else
            {
	      /*if (distFromNeigh == minDistance)
	         penalty = -1.0;   //no loops; reward routing decissions
              else
		 penalty = 0.0; //backpressure will direct the packet to it*/	 
	      penalty = -1.0;
            }
          break;
	}
       }   
    }
  return penalty;
}

Ipv4Address
BackpressureState::FindNextHopInFlowTable(Ipv4Address srcAddr, Ipv4Address dstAddr, Ipv4Address currAddr, uint16_t s_port, uint16_t d_port, uint32_t &iface) //, bool &erased, Ipv4Address &prev_nh)
{
  FlowTuple entry;
  entry.src = srcAddr;
  entry.dst = dstAddr;
  entry.srcPort = s_port;
  entry.dstPort = d_port;
  //NS_LOG_UNCOND("FindNextHopInFlowTable: prior loop");
  
  for (FlowSet::iterator it = m_flowSet.begin ();
      it != m_flowSet.end (); )
    {
       if (*it == entry)
         {
           NS_LOG_DEBUG("FindNextHopInFlowTable: found flow entry nexthop is: "<<it->nh<<" output iface: "<<(int)it->iface);
           Time now = Simulator::Now();	
           /*if ( (now.GetSeconds ()-it->lstRouteDec.GetSeconds ())<1.0 ) //check if route in found flow entry is valid
             {
	       NS_LOG_DEBUG("FindNextHopInFlowTable: Flow Entry valid and route valid");
               iface = it->iface; //output interface
               return it->nh;     // next hop
             }
           else
             {
	       NS_LOG_DEBUG("FindNextHopInFlowTable: FlowTable Entry valid and route invalid");
  	       entry.lstRouteDec = Simulator::Now();   //route lifetime of the flow entry
               entry.lstEntryUsed =  Simulator::Now(); //update last time the flow entry was used
               return Ipv4Address("127.0.0.1");
             }*/
	   if (((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())<= 10.0) && ((now.GetSeconds()-it->lstRouteDec.GetSeconds())<=10.0))
	   //if ((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())<= 1.75)
	     {
	       //the entry is valid because it is in use
	       iface = it->iface;
	       //it->lstRouteDec = Simulator::Now();   //route lifetime of the flow entry
	       it->lstEntryUsed = Simulator::Now(); //update last time the flow entry was used
	       return it->nh;
	     }
	   else
	     {
	       NS_LOG_DEBUG("FindNextHopInFlowTable: Erase NOT USED Flow Entry at "<< currAddr<<" for src:"<<it->src<<" dst:"<<it->dst<<" sport:"<<it->srcPort<<" dport"<<it->dstPort<<" time:"<<Simulator::Now()); 
               it = m_flowSet.erase (it);
	     }  
         }
       else
         {
           NS_LOG_DEBUG("FindNextHopInFlowTable: Not the entry looked for, check validity of flow entry for potential remove ");
           Time now = Simulator::Now();	
           if ( ((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())>10.0) || ((now.GetSeconds()-it->lstRouteDec.GetSeconds())>10.0) ) //check validity of flow entry
	   //if ( ((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())>1.75) )
	     {
	       //this entry flow is not valid anymore
	       NS_LOG_DEBUG("FindNextHopInFlowTable: Erase OLD Flow Entry at "<< currAddr<<" for src:"<<it->src<<" dst:"<<it->dst<<" sport:"<<it->srcPort<<" dport"<<it->dstPort<<" time:"<<Simulator::Now()); 
               it = m_flowSet.erase (it);	       
             }
           else
             {
               it++;
             }
         }
    }
  NS_LOG_DEBUG("FindNextHopInFlowTable: flow entry not present: insert in flow table");
  entry.lstRouteDec = Simulator::Now();   //timestamps for tracking the lifetime of the route in the flow entry
  entry.lstEntryUsed =  Simulator::Now(); //timestamp with the use of the flow entry
  entry.nh = Ipv4Address("127.0.0.1"); //dummy initialization for the FindNextHopBackpressureCentralized
  m_flowSet.push_back (entry);
  return Ipv4Address("127.0.0.1");
}

//////////////////v2 of FindNextHopInFlowTable
Ipv4Address
BackpressureState::FindNextHopInFlowTable(Ptr<Ipv4> m_ipv4, Ipv4Address srcAddr, Ipv4Address dstAddr, Ipv4Address currAddr, uint16_t s_port, uint16_t d_port, uint32_t &iface, bool &erased, Ipv4Address &prev_nh, uint8_t ttl)
{
  FlowTuple entry;
  entry.src = srcAddr;
  entry.dst = dstAddr;
  entry.srcPort = s_port;
  entry.dstPort = d_port;
  //NS_LOG_UNCOND("FindNextHopInFlowTable: prior loop");
  
  for (FlowSet::iterator it = m_flowSet.begin ();
      it != m_flowSet.end (); )
    {
      //std::cout<<"Hola en: "<<currAddr<<"en time: "<<Simulator::Now()<<" el size de flowset: "<<m_flowSet.size()<<std::endl;
      /*if (currAddr.IsEqual("10.0.0.5"))
      {
        exit(-1);
      }*/
      
      if (*it == entry)
        {
          NS_LOG_DEBUG("FindNextHopInFlowTable: found flow entry nexthop is: "<<it->nh<<" output iface: "<<(int)it->iface);
          Time now = Simulator::Now();	
          //std::cout<<"fistro1"<<std::endl; 
	  if (it->ttl != ttl)
	    {//there has been a potential loop so we remove the rule to force its recalculation
	      //std::cout<<"We find a loop "<<srcAddr<<" "<<dstAddr<<" "<<currAddr<<" ttl regla: "<<(int32_t)it->ttl<<" ttl paquete: "<<(int32_t)ttl<<" time: "<<Simulator::Now()<<std::endl;
	      it = m_flowSet.erase (it);
	    }
	  else
	    {
	      if (((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())<= 10.0) && ((now.GetSeconds()-it->lstRouteDec.GetSeconds())<=100.0))
	        {
		  //the entry is valid because it is in use
		  //we have to check if the neighbor is available, if not, we cannot return this interface value
		  //std::cout<<"fistro2"<<std::endl; 
		  //This check has to be skipped in the EPC node for the UL traffic
		  if (! (m_ipv4->GetObject<Node>()->GetId() == 0 && dstAddr.IsEqual("1.0.0.2")) )
		    {
		      for (NeighborSet::iterator it2 = m_neighborSet.begin(); it2 != m_neighborSet.end(); it2++)
		        {
			  //if ((it2->interface == it->iface) && (it->nh == it2->theMainAddr))
			  //std::cout<<"fistro2.5"<<" elneighbor: "<<it2->theMainAddr<<" el hop en la table: "<<it->nh<< "y estoy en nodo: "<<currAddr<<std::endl;
			  if (it2->interface == it->iface)
			    {
			      //std::cout<<"SRC: "<<srcAddr<<" "<<"DST: "<<dstAddr<<" "<<"NEIGH: "<<it2->theMainAddr<<std::endl;
			      //exit(-1);
			      //std::cout<<"fistro3"<<std::endl; 
			      //time constraints of a possible neighbor
			      if ( ((now.GetSeconds()-it2->lastHello.GetSeconds()) > m_neighValid.GetSeconds())) 
			        {
			          //std::cout<<"fistro4"<<std::endl; 
			          if (m_SatNode == true && it2->interface ==1)
			            {
			              //it is assumed that the satellite link is always available
			              iface = it->iface;
			              //it->lstRouteDec = Simulator::Now();   //route lifetime of the flow entry
			              it->lstEntryUsed = Simulator::Now(); //update last time the flow entry was used
			              return it->nh;
			            }
			          else
			            {
			              //the neighbor is lost and we need to refresh the rule erasing it
			              std::cout<<"Borro aqui en: "<<currAddr<<std::endl;
			              it = m_flowSet.erase(it);  
			            }
			        }
			      else
			        {
			          //std::cout<<"Entro aqui en: "<<currAddr<<std::endl;
			          iface = it->iface;
			          //it->lstRouteDec = Simulator::Now();   //route lifetime of the flow entry
			          it->lstEntryUsed = Simulator::Now(); //update last time the flow entry was used
			          return it->nh;
			        }
			    }
		        }
		    }
		  else
		    {
		      //this case is entered in the hop between EPC and remotehost for UL traffic
		      //it is assumed this link is always available (in fact, we do not have hello's on this interface)
		      iface = it->iface;
		      //it->lstRouteDec = Simulator::Now();   //route lifetime of the flow entry
		      it->lstEntryUsed = Simulator::Now(); //update last time the flow entry was used
		      return it->nh;
		    }
	        }
	      else
	        {
		  NS_LOG_DEBUG("FindNextHopInFlowTable: Erase NOT USED Flow Entry at "<< currAddr<<" for src:"<<it->src<<" dst:"<<it->dst<<" sport:"<<it->srcPort<<" dport"<<it->dstPort<<" time:"<<Simulator::Now()); 
		  erased = true;
		  prev_nh = it->nh;
		  it = m_flowSet.erase (it);   
	        }  
	    }
        }
       else
         {
           NS_LOG_DEBUG("FindNextHopInFlowTable: Not the entry looked for, check validity of flow entry for potential remove ");
           Time now = Simulator::Now();	
           //if ( ((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())>10.0) || ((now.GetSeconds()-it->lstRouteDec.GetSeconds())>10.0) ) //check validity of flow entry
	   if ( ((now.GetSeconds ()-it->lstEntryUsed.GetSeconds())>10.0) )
	     {
	       //this entry flow is not valid anymore
	       NS_LOG_DEBUG("FindNextHopInFlowTable: Erase OLD Flow Entry at "<< currAddr<<" for src:"<<it->src<<" dst:"<<it->dst<<" sport:"<<it->srcPort<<" dport"<<it->dstPort<<" time:"<<Simulator::Now()); 
               it = m_flowSet.erase (it);	       
             }
           else
             {
               it++;
             }
         }
    }
  NS_LOG_DEBUG("FindNextHopInFlowTable: flow entry not present: insert in flow table");
  entry.lstRouteDec = Simulator::Now();   //timestamps for tracking the lifetime of the route in the flow entry
  entry.lstEntryUsed =  Simulator::Now(); //timestamp with the use of the flow entry
  entry.nh = Ipv4Address("127.0.0.1"); //dummy initialization for the FindNextHopBackpressureCentralized
  m_flowSet.push_back (entry);
  return Ipv4Address("127.0.0.1");
}



void 
BackpressureState::UpdateFlowTableEntry(Ipv4Address srcAddr, Ipv4Address dstAddr, Ipv4Address currAddrr, uint16_t s_port, uint16_t d_port, Ipv4Address nh, uint32_t iface, uint8_t ttl)
{
  FlowTuple entry;
  entry.src = srcAddr;
  entry.dst = dstAddr;
  entry.srcPort = s_port;
  entry.dstPort = d_port;
  for (FlowSet::iterator it = m_flowSet.begin ();
      it != m_flowSet.end (); it++)
    {
       if (*it == entry)
        {
          NS_LOG_DEBUG("UpdateFlowTableEntry: Update Nexthop for srcIP:"<<srcAddr<<" dstIP:"<<dstAddr<<" currAddr: "<<currAddrr<<" sport:"<<s_port<<" dport:"<<d_port<<" nh:"<<nh<<" iface:"<<iface<<" time:"<<Simulator::Now());
          it->nh = nh;
          it->iface = iface;
	  it->ttl = ttl;
	  //std::cout<<"UpdateFlowTableEntry: Update Nexthop for srcIP:"<<srcAddr<<" dstIP:"<<dstAddr<<" currAddr: "<<currAddrr<<" sport:"<<s_port<<" dport:"<<d_port<<" nh:"<<nh<<" iface:"<<iface<<" time:"<<Simulator::Now()<<" ttl: "<<(uint32_t)it->ttl<<std::endl;
          it->lstRouteDec = Simulator::Now();
        }
      else
        {
          NS_LOG_DEBUG("UpdateFlowTableEntry: not found for srcIP:"<<srcAddr<<" dstIP:"<<dstAddr<<" sport:"<<s_port<<" dport:"<<d_port<<" nh:"<<nh<<" time:"<<Simulator::Now());
        }
    }
}

void
BackpressureState::EraseFlowTableEntry(Ipv4Address srcAddr, Ipv4Address dstAddr, uint16_t s_port, uint16_t d_port)
{
  FlowTuple entry;
  entry.src = srcAddr;
  entry.dst = dstAddr;
  entry.srcPort = s_port;
  entry.dstPort = d_port;
  for (FlowSet::iterator it = m_flowSet.begin ();
      it != m_flowSet.end (); it++)
    {
       if (*it == entry)
        {
	  it = m_flowSet.erase (it);
	  return;
	}
    }
}

void 
BackpressureState::EraseFlowTable()
{
  //for (FlowSet::iterator it = m_flowSet.begin (); it != m_flowSet.end (); )
    //it = m_flowSet.erase (it);
  m_flowSet.clear();
  //not needed because we have information of the neighbor before switching it off
  //m_neighborSet.clear();
}


Ipv4Address
BackpressureState::FindNextHopBackpressureCentralized(uint32_t strategy, Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t ttl, uint32_t myLength, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface, const Mac48Address &from, uint32_t nodesRow, Ptr<Ipv4> m_ipv4)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(MAX_NEIGHBORS);				// Up to MAX_NEIGHBORS
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  Vector dPos   = LocationList::GetLocation(dstAddr); 		        // Get coordinates from the destination
  Vector curPos = LocationList::GetLocation(currAddr); 		        // Get coordinates from the destination
  Vector sPos   = LocationList::GetLocation(srcAddr); 		        // Get coordinates from the destination
  double vqLength = CalculateDistance(dPos, curPos);                   // Distance to the destination from the current node to the destination
  double distFromCurrToSrc = CalculateDistance(sPos, currPos);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;							// Maximum weight initialization
  //PrintNeighInfo(anode);
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      if ((now.GetSeconds()-it->lastHello.GetSeconds())>3.0)
	{
          NS_LOG_DEBUG("the neighbor "<<it->theMainAddr<<" is not present");
          continue;							//Neighbor is not present, jump to the next neighbor
        }
      
      if ( it->theMainAddr.IsEqual(dstAddr) )      			//One Hop to the destination 
      //if ( it->neighborMainAddr.IsEqual(dstAddr) )
        { 
	  //destination address is always interface 1, because this is the main addr
	  /*uint32_t parity = anode%2;
	  if (parity ==0)
	  {
	    iface = it->interface;
	    theNextHop = it->neighborMainAddr;
	  }
	  else
	  { //parity ==1
	    Vector nPos = Vector(it->x, it->y, 0);
	    uint32_t tmp_iface = it->interface;
	    //look for the other interface with the same position
	    for (NeighborSet::iterator it2 = m_neighborSet.begin();
		  it2 != m_neighborSet.end(); it2++)
		 {
		    if (nPos.x == it2->x && nPos.y == it2->y && tmp_iface!=it2->interface)
		      iface = it2->interface;
		      theNextHop = it->neighborMainAddr;
		 }
	  }*/
	 iface = it->interface;
	 theNextHop = it->neighborMainAddr;
         return theNextHop;
        }
	
	uint32_t theId=it->theMainAddr.Get();			// Calculation of the identifier of the current node
        uint32_t minId=Ipv4Address("10.0.100.0").Get();
        uint32_t id = theId-minId-1;					
	uint32_t virt_cong;
	Ipv4Address tmp("10.0.200.1"); //dummy direction
	int nlen = getQlengthCentralized(id, tmp, virt_cong, dstAddr);
	
	//int nlen = GetQlengthCentralized(id,tmp);
        //NS_LOG_UNCOND("the min ID is "<<minId<<" the id is "<<id);
	  
      int32_t diffBcklg = (int)myLength - (int)nlen; 			// Data queue differential on a centraliced way
      it->qLength = nlen;						// Update Queue Backlog using centraliced info
      if ((now.GetSeconds()-it->qPrev.GetSeconds())>0.1)
        {
          it->qLengthPrev = it->qLengthSchedPrev;
          it->qLengthSchedPrev = it->qLength;
          it->qPrev = now;
        }
      Vector nPos = Vector(it->x, it->y, 0);
      double nvqLength = CalculateDistance(dPos, nPos);
      double distFromNeighToSrc = CalculateDistance(sPos, nPos);
      if (it->neighborhwAddr == from)
        {
          m_weightV = 200;
        }
      float penalty = CalculatePenaltyNeigh(anode, dPos, strategy, ttl, vqLength, nvqLength, distFromCurrToSrc, distFromNeighToSrc, virt_cong, it->neighborhwAddr, from,dstAddr);
      // all the links have the same capacity
      //float rateLink     = 1.0;
      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
      if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
        {
           if (maxWeight > 0.0)
            {//now I can have negative values
              theNeighSameWeight.push_back(&(*it));
              eqNeighs++;
            }
          if ( it->neighborMainAddr.IsEqual(dstAddr) )      //One Hop to the destination 
            { //if a neighbor is my destination don't compute anything important for bidirectional flows
              iface = it->interface;
              theNextHop = it->neighborMainAddr;
              return theNextHop;
            }
        }
      else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
        { 
          theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
          theNeighSameWeight.push_back(&(*it));
          eqNeighs  = 1;
          maxWeight = currWeight;
        }
     }
    //finishing neighbor loop 
    if ( eqNeighs > 1 )
      {	
	//let's introduce a check whether we have a rule in this node for this flow -> not having an improvement
	/*std::vector<uint> rules_per_neighbor (theNeighSameWeight.size(),0); //initialized to zero 
	for (FlowSet::iterator it = m_flowSet.begin(); it!=m_flowSet.end() ; it++)
	  {
	    if ((it->src.IsEqual(srcAddr)) && (it->dst.IsEqual(dstAddr)) && !(it->nh.IsEqual("127.0.0.1")))
	    {
	      //there is a rule with this flow
	      //std::cout<<"There is a rule with this flow"<<std::endl;
	      //std::cout<<"rule SRC_IP: "<<it->src<<" rule DST_IP: "<<it->dst<<"rule nexthop: "<<it->nh<<std::endl;
	      //std::cout<<"function currAddr: "<<currAddr<<"function SRC_IP: "<<srcAddr<<" rule DST_IP: "<<dstAddr<<std::endl;
	    
	      for (uint i=0; i<theNeighSameWeight.size(); i++)
	        {
		  if (it->nh.IsEqual(theNeighSameWeight[i]->neighborMainAddr))
		  {
		      rules_per_neighbor[i]++;
		      //std::cout<<"El vecino: "<<theNeighSameWeight[i]->neighborMainAddr<<" tiene reglas: "<<rules_per_neighbor[i]<<std::endl;
		  }
	        }
	    }
	  }
	int max= -999999;
	uint32_t index=355; //dummy value to not erase all the time
	//find the neighbor with more rules for this src-dst
	for (uint i=0; i<theNeighSameWeight.size();i++)
	{
	  if (((int)rules_per_neighbor[i] > max) && (rules_per_neighbor[i]!=0))
	  {
	    max = rules_per_neighbor[i];
	    index = i;
	  }
	}
	
	if (index!=355)
	{
	  //uint8_t buff_i[4], buff_j[4];
	  //theNeighSameWeight[index]->neighborMainAddr.Serialize(buff_i);
	  Vector nPos = Vector(theNeighSameWeight[index]->x, theNeighSameWeight[index]->y, 0);
	  Vector tmp;
	  //we have to erase its counterpart
	  theNeighSameWeight.erase(theNeighSameWeight.begin()+index);
	  for (uint i =0; i<theNeighSameWeight.size();i++)
	  {
	    tmp = Vector(theNeighSameWeight[i]->x, theNeighSameWeight[i]->y, 0);
	    if ( (tmp.x == nPos.x) && (tmp.y == nPos.y) )
	      {
		theNeighSameWeight.erase(theNeighSameWeight.begin()+i);
	      }
	  }
	  //for (uint i=0; i<theNeighSameWeight.size();i++)
	    //{
	      //theNeighSameWeight[i]->neighborMainAddr.Serialize(buff_j);
	      //if (buff_i[3]>128 && buff_j[3]>128)
	        //{
		  //theNeighSameWeight.erase(theNeighSameWeight.begin()+i);
	        //}
	      //else if (buff_i[3]<128 && buff_j[3]<128)
	        //{
		  //theNeighSameWeight.erase(theNeighSameWeight.begin()+i);
	        //}
	    //}
	  
	  
	}*/
	
	//ORIGINAL CODE
        /*UniformRandomVariable node(0, theNeighSameWeight.size()-1); 				  // Some Candidate Neighbors can be next-hop 
        uint32_t aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
        NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
        iface = theNeighSameWeight[aNode]->interface;
        return (theNeighSameWeight[aNode]->neighborMainAddr);*/
	
	//BP-SDN CODE:even/odd
	//Due to construction all the nodes will have 2 neighbors with the same weight,
	//according to this, depending on the identifier of the node, we will force one channel or the other
	//interface
	/*UniformRandomVariable node(0, theNeighSameWeight.size()-1);
	uint32_t parity = anode%2;
	uint32_t aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
	//std::cout<<"En node: "<<anode+1<<"los potenciales vecinos son: "<<theNeighSameWeight.size()<<std::endl;
	//if parity == 0 we look for the next node with the 1 interface
	//if parity == 1 we look for the next node with the 2 interface
	if (parity==0)
	  {
	    while (theNeighSameWeight[aNode]->interface !=1)
	    {
	      aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
	    }
	    
	  }
	  else
	   {
	     while (theNeighSameWeight[aNode]->interface !=2)
	    {
	      aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
	    }
	   }
	
	iface = theNeighSameWeight[aNode]->interface;
        return (theNeighSameWeight[aNode]->neighborMainAddr);	*/
      
      //BP-SDN CODE:per row
	/*UniformRandomVariable node(0, theNeighSameWeight.size()-1);
	uint32_t row = anode/nodesRow;
	uint32_t parity = row%2;
	uint32_t aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
	//if parity == 0 we look for the next node with the 1 interface
	//if parity == 1 we look for the next node with the 2 interface
	if (parity==0)
	  {
	    while (theNeighSameWeight[aNode]->interface !=1)
	    {
	      aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
	    }
	    
	  }
	  else
	   {
	     while (theNeighSameWeight[aNode]->interface !=2)
	    {
	      aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
	    }
	   }
	
	iface = theNeighSameWeight[aNode]->interface;
        return (theNeighSameWeight[aNode]->neighborMainAddr);*/
	
	//BP-SDN CODE: missconception previously. If the neighbor it is in my row I send through
	//the interface 1, otherwise with interface 2

	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
        x->SetAttribute("Min", IntegerValue(0));
	x->SetAttribute("Max", IntegerValue(theNeighSameWeight.size()-1));
	uint32_t aNode = x->GetInteger(0,theNeighSameWeight.size()-1);
	Vector nPos = Vector(theNeighSameWeight[aNode]->x, theNeighSameWeight[aNode]->y, 0);
	//if the y coordinate is the same use interface 1, if not interface 2
	uint32_t index= aNode;
	//std::cout<<"Hola el vecino tiene posición: "<<nPos.x<<","<<nPos.y<<std::endl;
	if ((curPos.y - nPos.y) == 0.0)
	  {
	    //std::cout<<"Hola 1 el vecino tiene posición: "<<nPos.x<<","<<nPos.y<<std::endl;
	    if (theNeighSameWeight[aNode]->interface != 1)
	    {
	      Vector tmp;
	      for (uint32_t i =0; i<theNeighSameWeight.size(); i++)
	        {
		  tmp = Vector(theNeighSameWeight[i]->x, theNeighSameWeight[i]->y, 0);
		  if ( (tmp.y == nPos.y) && (theNeighSameWeight[i]->interface != theNeighSameWeight[aNode]->interface) )
		    {
		      index = i;
		    }
	        }
	    }
	  }
	else
	  { // we go out through the interface 2
	  //std::cout<<"Hola 2 el vecino tiene posición: "<<nPos.x<<","<<nPos.y<<std::endl;
	    if (theNeighSameWeight[aNode]->interface != 2)
	    {
	      Vector tmp;
	      for (uint32_t i =0; i<theNeighSameWeight.size(); i++)
	        {
		  tmp = Vector(theNeighSameWeight[i]->x, theNeighSameWeight[i]->y, 0);
		  if ( (tmp.y == nPos.y) && (theNeighSameWeight[i]->interface != theNeighSameWeight[aNode]->interface) )
		    {
		       index = i;
		    }
	        }
	    }
	  }
	  iface = theNeighSameWeight[index]->interface;
	  return (theNeighSameWeight[index]->neighborMainAddr);
      }
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        NS_LOG_DEBUG("[FindNextHop]: The next hop is "<<theNeighSameWeight[0]->neighborMainAddr);
        iface = theNeighSameWeight[0]->interface;
        return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        NS_LOG_DEBUG("[FindNextHop]: There are no next hops ");
        iface = 0;
        return Ipv4Address ("127.0.0.1");
      }
}

float
BackpressureState::GetRate(uint32_t anode,Ipv4Address neighAddr,Ptr<Ipv4> m_ipv4,uint32_t interface)
{
  Ptr<NetDevice> aDev= m_ipv4->GetNetDevice(interface);
  Ptr<PointToPointNetDevice> aPtopDev = aDev->GetObject<PointToPointNetDevice> ();
  Ptr<WifiNetDevice> aWifiDev = aDev->GetObject<WifiNetDevice> ();
    uint64_t rate_bps;
  if (aWifiDev == NULL)
    {//ppp device
      rate_bps = aPtopDev->GetRatebps();
    }
  else
    {//wifi device
      rate_bps = aWifiDev->GetRatebps();
    }
  return float(rate_bps/1000000);
}

Ipv4Address
BackpressureState::FindNextHopBackpressureDistributed(uint32_t strategy, Ipv4Address const &curAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t ttl, uint32_t myLength, uint32_t anode, Vector curPos, uint32_t m_weightV,uint32_t &iface, const Mac48Address &from, uint32_t )
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight;
  Time now = Simulator::Now();						// Get the current Time
  theNeighSameWeight.reserve(MAX_NEIGHBORS);				// Up to MAX_NEIGHBORS
  Vector dPos = LocationList::GetLocation(dstAddr); 		        // Get coordinates from the destination
  Vector sPos = LocationList::GetLocation(srcAddr);
  double vqLength = CalculateDistance(dPos, curPos);                   // Distance to the destination from the current node to the destination
  double distFromCurrToSrc = CalculateDistance(sPos, curPos);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;							// Maximum weight initialization
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      if ((now.GetSeconds()-it->lastHello.GetSeconds())>0.2)
        {
	      NS_LOG_DEBUG("neighbor "<<it->theMainAddr<<" is not present");
          continue;			//consider neighbor is not present, jump to the next neighbor
        }
       
       if ( it->theMainAddr.IsEqual(dstAddr) )      			//One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          iface = it->interface;
          theNextHop = it->neighborMainAddr;
         return theNextHop;
        }
        
      int32_t diffBcklg = (int)myLength - (int)it->qLength; 		// Data queue differential using HELLO information
      Vector nPos = Vector(it->x, it->y, 0);
      double nvqLength = CalculateDistance(dPos, nPos);
      double distFromNeighToSrc = CalculateDistance(sPos, nPos);

      uint32_t theId=it->theMainAddr.Get();			// Calculation of the identifier of the current node
      uint32_t minId=Ipv4Address("10.0.100.0").Get();
      uint32_t id = theId-minId-1;
      uint32_t virt_cong;
      uint32_t nlen = getQlengthCentralized(id,it->theMainAddr,virt_cong, dstAddr);
      if (nlen>0){NS_LOG_DEBUG("fast dirty");}
      float penalty    = CalculatePenaltyNeigh(anode, dPos, strategy, ttl, vqLength, nvqLength, distFromCurrToSrc, distFromNeighToSrc, virt_cong, it->neighborhwAddr, from, dstAddr);
      float rateLink   = 1.0;								  // all the links have the same capacity
      float currWeight = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
      if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
        {
           if (maxWeight > 0.0)
            {//now I can have negative values
              theNeighSameWeight.push_back(&(*it));
              eqNeighs++;
            }
        }
      else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
        { 
          theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
          theNeighSameWeight.push_back(&(*it));
          eqNeighs  = 1;
          maxWeight = currWeight;
        }
     }
    if ( eqNeighs > 1 )
      {							
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
        x->SetAttribute("Min", IntegerValue(0));
	x->SetAttribute("Max", IntegerValue(theNeighSameWeight.size()-1));
	uint32_t aNode = x->GetInteger();
        NS_LOG_DEBUG("[FindNextHopDiPUMP]: next hop is sameLength "<<theNeighSameWeight[aNode]->neighborMainAddr);
        iface = theNeighSameWeight[aNode]->interface;
        return (theNeighSameWeight[aNode]->neighborMainAddr);
      }
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
        return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
        return Ipv4Address ("127.0.0.1");
      }
}

bool
BackpressureState::areAllCloserToSrc(Vector srcPos,Vector currPos)
{
 double eucDistFromCurr = CalculateDistance(srcPos, currPos);
 bool allCloser=false;
 for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    {
      Vector nPos = Vector(it->x, it->y, 0);
      double eucDistFromNeigh;
      eucDistFromNeigh = CalculateDistance(srcPos, nPos);
      if (eucDistFromNeigh<eucDistFromCurr)
        {
          allCloser= true;
        }
      else
        {
          return false;
        }
    }
  return allCloser;
}


/////////////////////////////////////MULTIINTERFACE_PTOP_SINGLE_QUEUE
//Taking into account all kind of traffics. Hops are obtained from one IP to the other
Ipv4Address
BackpressureState::FindNextHopBackpressureCentralizedGridSANSA(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t anode, uint32_t m_weightV, uint32_t &iface, Ptr<Ipv4> m_ipv4, uint32_t nodesColumn, const Mac48Address &from)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(4);					// The maximum number of neighbors in a grid is 4
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t HopsFromCurr = LocationList::GetHops(currAddr, dstAddr);
  //NS_LOG_UNCOND("sPos:"<<sPos<<" dPos:"<<dPos<<" currPos:"<<currPos<<" ttl:"<<ttl);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;	
  // Maximum weight initialization
  int32_t diffBcklg=0, diffNet=0; //diffnet cannot be uint because it can take negative values
  uint8_t buf_curr[4];
  uint8_t buf_neig[4];
  currAddr.Serialize(buf_curr);
  //std::cout<<"el valor de currAddr = "<<currAddr<<std::endl;
      
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
  
    {
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_neighValid.GetSeconds());
      if ( ( now.GetSeconds()-it->lastHello.GetSeconds() ) > m_neighValid.GetSeconds() )
        {
	  //std::cout<<"Neighbor no present"<<std::endl;
          continue;							//Neighbor is not present, jump to the next neighbor
        }
        
      if ( it->theMainAddr.IsEqual(dstAddr))      //One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          iface = it->interface;
          theNextHop = it->neighborMainAddr;
          return theNextHop;
        }
      uint32_t id=105; //dummy initialization of the neighbor identifier value
      it->theMainAddr.Serialize(buf_neig);
      if (buf_curr[2]>=200) //we are in the gw node
        { //the src is the gw/epc
	  id = LocationList::GetGwId(it->theMainAddr);
        }
      else if (buf_neig[2]>=200)
	{// this condition will only occur in downlink
	  id = NodeList::GetNNodes()-1;
	}
      else
        { //we are in the backhaul
          diffNet = (int32_t)buf_neig[2] - (int32_t)buf_curr[2];
          //This way of finding the id is only valid for "regular square/rectangle" deployments 
          if (diffNet==0)
            {
	      if (((int32_t)buf_neig[3]-(int32_t)buf_curr[3]) > 0)
	        id = anode + 1;
	      else
	        id = anode - 1;
	    }
          else if (diffNet == -1)
            {
	      id = anode - 1;
            }
          else if (diffNet == 1)
	    {
	      id = anode + 1;
	    }
          else if (diffNet == (int32_t)nodesColumn - 1)
	    {
	      id = anode + nodesColumn;
	    }
          else if (diffNet == 1 - (int32_t)nodesColumn)
	    {
	      id = anode - nodesColumn;
	    }
         }
      uint32_t nlen, myLength;
      Ipv4Address tmp("10.0.250.1"); //dummy direction
      nlen= GetQlengthCentralized(id,tmp);
      myLength= GetQlengthCentralized(anode,tmp);
      //std::cout<<"PENALTY GRID, myLength= "<<myLength<<", el neighLen= "<<nlen<<std::endl;
      diffBcklg = (int)myLength - (int)nlen; 			// Data queue differential on a centraliced way
      it->qLength = GetQlengthCentralized(id, tmp);	// Update Queue Backlog using centraliced info
      //std::cout<<"El valor de id: "<<id+1<<", el valor de anode: "<<anode+1<<" y el valor de theMainAddr:"<<it->theMainAddr<<", el valor de tmp: "<<tmp<<std::endl;
      if ((now.GetSeconds()-it->qPrev.GetSeconds())>0.1)
        {
          it->qLengthPrev = it->qLengthSchedPrev;
          it->qLengthSchedPrev = it->qLength;
          it->qPrev = now;
        }
      uint32_t HopsFromNeigh = LocationList::GetHops(it->theMainAddr, dstAddr);
      //std::cout<<"en nodo: "<<it->theMainAddr<<" en Time: "<<Simulator::Now()<<" los hops son: "<<HopsFromNeigh<<std::endl;
      float penalty= CalculatePenaltyNeighGridSANSA_v2(m_ipv4, anode, dstAddr, HopsFromCurr, HopsFromNeigh, it->neighborhwAddr, from,0);
      if (from == it->neighborhwAddr)
      {
	m_weightV = 200;
      }
      //float rateLink     = 1.0;							  // all the links have the same capacity
      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
      //std::cout<<"El valor de currWeight: "<<currWeight<<", V= "<<m_weightV<<", penalty= "<<penalty<<std::endl;
      if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
        {
           if (maxWeight > 0.0)
            {//now I can have negative values
              theNeighSameWeight.push_back(&(*it));
              eqNeighs++;
            }
        }
      else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
        { 
          theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
          theNeighSameWeight.push_back(&(*it));
          eqNeighs  = 1;
          maxWeight = currWeight;
        }
     } //end neighbor loop
       
    if ( eqNeighs > 1 )
      {		
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
        uint32_t aNode = x->GetInteger(0,theNeighSameWeight.size()-1);
        NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
	iface = theNeighSameWeight[aNode]->interface;
        return (theNeighSameWeight[aNode]->neighborMainAddr);
      }
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
	return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
	return Ipv4Address ("127.0.0.1");
      }
}
/////////////////////////////////////////////////////////////////////////////
//3rd attempt: to keep a route if possible
Ipv4Address
BackpressureState::FindNextHopBackpressureCentralizedGridSANSAv2(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t anode, uint32_t m_weightV, uint32_t &iface, Ptr<Ipv4> m_ipv4, uint32_t nodesColumn, const Mac48Address &from, bool erased, Ipv4Address prev_nh)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(4);					// The maximum number of neighbors in a grid is 4
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t HopsFromCurr = LocationList::GetHops(currAddr, dstAddr);
  //NS_LOG_UNCOND("sPos:"<<sPos<<" dPos:"<<dPos<<" currPos:"<<currPos<<" ttl:"<<ttl);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;	
  // Maximum weight initialization
  int32_t diffBcklg=0, diffNet=0; //diffnet cannot be uint because it can take negative values
  uint8_t buf_curr[4];
  uint8_t buf_neig[4];
  currAddr.Serialize(buf_curr);
  //std::cout<<"el valor de currAddr = "<<currAddr<<std::endl;
      
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
  
    {
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_neighValid.GetSeconds());
      if ( ( now.GetSeconds()-it->lastHello.GetSeconds() ) > m_neighValid.GetSeconds() )
        {
	  //std::cout<<"Neighbor no present"<<std::endl;
          continue;							//Neighbor is not present, jump to the next neighbor
        }
        
      if ( it->theMainAddr.IsEqual(dstAddr))      //One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          iface = it->interface;
          theNextHop = it->neighborMainAddr;
          return theNextHop;
        }
      uint32_t id=105; //dummy initialization of the neighbor identifier value
      it->theMainAddr.Serialize(buf_neig);
      if (buf_curr[2]>=200) //we are in the gw node
        { //the src is the gw/epc
	  id = LocationList::GetGwId(it->theMainAddr);
        }
      else if (buf_neig[2]>=200)
	{// this condition will only occur in downlink
	  id = NodeList::GetNNodes()-1;
	}
      else
        { //we are in the backhaul
          diffNet = (int32_t)buf_neig[2] - (int32_t)buf_curr[2];
          //This way of finding the id is only valid for "regular square/rectangle" deployments 
          if (diffNet==0)
            {
	      if (((int32_t)buf_neig[3]-(int32_t)buf_curr[3]) > 0)
	        id = anode + 1;
	      else
	        id = anode - 1;
	    }
          else if (diffNet == -1)
            {
	      id = anode - 1;
            }
          else if (diffNet == 1)
	    {
	      id = anode + 1;
	    }
          else if (diffNet == (int32_t)nodesColumn - 1)
	    {
	      id = anode + nodesColumn;
	    }
          else if (diffNet == 1 - (int32_t)nodesColumn)
	    {
	      id = anode - nodesColumn;
	    }
         }
      uint32_t nlen, myLength;
      Ipv4Address tmp("10.0.250.1"); //dummy direction
      nlen= GetQlengthCentralized(id,tmp);
      myLength= GetQlengthCentralized(anode,tmp);
      //std::cout<<"PENALTY GRID, myLength= "<<myLength<<", el neighLen= "<<nlen<<std::endl;
      diffBcklg = (int)myLength - (int)nlen; 			// Data queue differential on a centraliced way
      it->qLength = GetQlengthCentralized(id, tmp);	// Update Queue Backlog using centraliced info
      //std::cout<<"El valor de id: "<<id+1<<", el valor de anode: "<<anode+1<<" y el valor de theMainAddr:"<<it->theMainAddr<<", el valor de tmp: "<<tmp<<std::endl;
      if ((now.GetSeconds()-it->qPrev.GetSeconds())>0.1)
        {
          it->qLengthPrev = it->qLengthSchedPrev;
          it->qLengthSchedPrev = it->qLength;
          it->qPrev = now;
        }
      uint32_t HopsFromNeigh = LocationList::GetHops(it->theMainAddr, dstAddr);
      //std::cout<<"en nodo: "<<it->theMainAddr<<" en Time: "<<Simulator::Now()<<" los hops son: "<<HopsFromNeigh<<std::endl;
      float penalty= CalculatePenaltyNeighGridSANSA_v2(m_ipv4, anode, dstAddr, HopsFromCurr, HopsFromNeigh, it->neighborhwAddr, from,0);
      if (from == it->neighborhwAddr)
      {
	m_weightV = 200;
      }
      //float rateLink     = 1.0;							  // all the links have the same capacity
      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
      //std::cout<<"El valor de currWeight: "<<currWeight<<", V= "<<m_weightV<<", penalty= "<<penalty<<std::endl;
      if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
        {
           if (maxWeight > 0.0)
            {//now I can have negative values
              theNeighSameWeight.push_back(&(*it));
              eqNeighs++;
            }
        }
      else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
        { 
          theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
          theNeighSameWeight.push_back(&(*it));
          eqNeighs  = 1;
          maxWeight = currWeight;
        }
     } //end neighbor loop
       
    if ( eqNeighs > 1 )
      {	
	  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
          uint32_t aNode = x->GetInteger(0,theNeighSameWeight.size()-1);
          NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
	  if (erased)
	  {
	    for (uint32_t i=0; i <eqNeighs; i++)
	    {
	      if (prev_nh.IsEqual(theNeighSameWeight[i]->neighborMainAddr))
	        {
		  iface = theNeighSameWeight[i]->interface;
		  return prev_nh;
	        }
	    }
	  }
	  iface = theNeighSameWeight[aNode]->interface;
          return (theNeighSameWeight[aNode]->neighborMainAddr);
	
      }
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
	return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
	return Ipv4Address ("127.0.0.1");
      }
}

///////////////////////////////////////////////////////////////////////////
bool
BackpressureState::IsValidNeighborSANSA(Ptr<Ipv4> m_ipv4, Time lastHello, uint32_t interface, bool SatFlow )
{
  
  if (!(m_ipv4->IsUp(interface)))
    {
      //if the interface is not up is not a valid neighbor
      return false;
    }
     
  //time constraints of a possible neighbor
  if ( (Simulator::Now().GetSeconds()-lastHello.GetSeconds()) > m_neighValid.GetSeconds() )
    {
      if (m_SatNode == true && interface==1)
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
    if (m_SatNode == true)
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

/////////BP for GRID-SANSA-LENA//////////////////////////////////////////////
Ipv4Address 
BackpressureState::FindNextHopBackpressureCentralizedGridSANSALena(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t m_weightV, uint32_t &iface, Ptr<Ipv4> m_ipv4, const Mac48Address &from, bool erased, Ipv4Address prev_nh, bool SatFlow)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(4);					// The maximum number of neighbors in a grid is 4
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t HopsFromCurr = LocationList::GetHops(currAddr, dstAddr, SatFlow);
  
    
  //NS_LOG_UNCOND("sPos:"<<sPos<<" dPos:"<<dPos<<" currPos:"<<currPos<<" ttl:"<<ttl);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;	
  // Maximum weight initialization
  int32_t diffBcklg=0;
      
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
  
    {
         //if (m_ipv4->GetObject<Node>()->GetId() == 5)
	   //if (currAddr.IsEqual("1.0.0.1"))
           //std::cout<<"En "<<currAddr<<"neighbor es: "<<it->neighborMainAddr<<"y su m_neighValid.GetSeconds es: "<<m_neighValid.GetSeconds()<<std::endl;  
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_neighValid.GetSeconds());
      ///conditions to evaluate if a neighbor is valid
      if (!IsValidNeighborSANSA(m_ipv4, it->lastHello, it->interface, SatFlow))
	continue;
      ///
      if ( it->theMainAddr.IsEqual(dstAddr))      //One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          iface = it->interface;
          theNextHop = it->neighborMainAddr;
	  //std::cout<<"next hop es destination:"<<theNextHop<<std::endl;
          return theNextHop;
        }
        
      
      uint32_t id = LocationList::GetNodeId(it->neighborMainAddr);
      uint32_t nlen, myLength;
      Ipv4Address tmp("10.0.250.1"); //dummy direction
      nlen= GetQlengthCentralized(id,tmp);
      uint32_t anode = LocationList::GetNodeId(currAddr);
      myLength= GetQlengthCentralized(anode,tmp);
      diffBcklg = (int)myLength - (int)nlen; 			// Data queue differential on a centraliced way
      it->qLength = nlen;	// Update Queue Backlog using centraliced info
      if ((now.GetSeconds()-it->qPrev.GetSeconds())>0.1)
        {
          it->qLengthPrev = it->qLengthSchedPrev;
          it->qLengthSchedPrev = it->qLength;
          it->qPrev = now;
        }
      uint32_t HopsFromNeigh = LocationList::GetHops(it->theMainAddr, dstAddr, SatFlow);
      /*if (SatFlow)
      {
	std::cout<<"Los hops de currAddr: "<<currAddr<<" from neigh "<<it->theMainAddr<<" to dest: "<<dstAddr<<": "<<HopsFromNeigh<<std::endl;
      }*/
      float penalty= CalculatePenaltyNeighGridSANSA_v2(m_ipv4, anode, dstAddr, HopsFromCurr, HopsFromNeigh, it->neighborhwAddr, from, SatFlow);
      if (from == it->neighborhwAddr)
      {
	m_weightV = 200;
      }
      //float rateLink     = 1.0;							  // all the links have the same capacity
      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
      //std::cout<<"El valor de currWeight: "<<currWeight<<", V= "<<m_weightV<<", penalty= "<<penalty<<std::endl;
      if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
        {
           if (maxWeight > 0.0)
            {//now I can have negative values
              theNeighSameWeight.push_back(&(*it));
              eqNeighs++;
            }
        }
      else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
        { 
          theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
          theNeighSameWeight.push_back(&(*it));
          eqNeighs  = 1;
          maxWeight = currWeight;
        }
     } //end neighbor loop
       
    if ( eqNeighs > 1 )
      {	
	  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
          uint32_t aNode = x->GetInteger(0,theNeighSameWeight.size()-1);
	  NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop, select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
	  if (erased)
	  {
	    for (uint32_t i=0; i <eqNeighs; i++)
	    {
	      if (prev_nh.IsEqual(theNeighSameWeight[i]->neighborMainAddr))
	        {
		  iface = theNeighSameWeight[i]->interface;
		  return prev_nh;
	        }
	    }
	  }
	  /*if(SatFlow)
	  {
	    for (uint32_t i=0; i <eqNeighs; i++)
	    {
	      std::cout<<"flujo sat: los neighbor son: "<< theNeighSameWeight[i]->neighborMainAddr<<"y la interface: "<<theNeighSameWeight[i]->interface<<std::endl;
	    }
	    std::cout<<"Flujo Sat:Tengo varios vecinos en "<<currAddr<<" y eligo a: "<<theNeighSameWeight[aNode]->neighborMainAddr<<std::endl;
	  }*/
	  iface = theNeighSameWeight[aNode]->interface;
          return (theNeighSameWeight[aNode]->neighborMainAddr);
	
      }
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
	return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
	return Ipv4Address ("127.0.0.1");
      }
}

//// BP for Grid SANSA-LENA  MultiRadio///////
Ipv4Address
BackpressureState::FindNextHopBackpressureCentralizedGridSANSALenaMR(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t m_weightV, uint32_t &iface, uint32_t output_interface, Ptr<Ipv4> m_ipv4, const Mac48Address &from, bool erased, Ipv4Address prev_nh, bool SatFlow)
{ //the base is FindNextHopBackpressurePenaltyGridEnhancedHybrid (BP-MR) combined with 
  //FindNextHopBackpressureCentralizedGridSANSALena
  
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(MAX_NEIGHBORS);					// Up to MAX_NEIGHBORS
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  uint32_t HopsFromCurr = LocationList::GetHops(currAddr, dstAddr, SatFlow);
  
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  // Maximum  weight initialization
  float maxWeight = 0;
  
  for (NeighborSet::iterator it = m_neighborSet.begin(); 
       it != m_neighborSet.end(); it++)    
    {
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_neighValid.GetSeconds());
      //if ( ( now.GetSeconds()-it->lastHello.GetSeconds() ) > m_neighValid.GetSeconds() )
      ///conditions to evaluate if a neighbor is valid
      if (!IsValidNeighborSANSA(m_ipv4, it->lastHello, it->interface, SatFlow))
	continue;     
      ///
      if ( it->theMainAddr.IsEqual(dstAddr))      //One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          iface = it->interface;
          theNextHop = it->neighborMainAddr;
	  //std::cout<<"next hop es destination:"<<theNextHop<<std::endl;
          return theNextHop;
        }
      
      uint32_t id = LocationList::GetNodeId(it->neighborMainAddr);
      uint32_t nlen, myLength;
      Ipv4Address tmp("10.0.250.1"); //dummy direction
      uint32_t anode = LocationList::GetNodeId(currAddr);
      myLength =  GetVirtualQlengthCentralized(anode, tmp, it->interface); 
      int32_t diffBcklg=0;
      uint32_t HopsFromNeigh = LocationList::GetHops(it->theMainAddr, dstAddr, SatFlow);
      float penalty= CalculatePenaltyNeighGridSANSA_v2(m_ipv4, anode, dstAddr, HopsFromCurr, HopsFromNeigh, it->neighborhwAddr, from, SatFlow);
      if (from == it->neighborhwAddr)
        {
	  m_weightV = 200;
	}
      else
        {
	  m_weightV = 200-myLength;
        }
      
      for (uint32_t j=0; j< it->n_interfaces; j++) 
        { //loop through all the interfaces of the neighbor
          //it->n_interfaces is of the same size of m_infoInterfaces, hence only counting 
          //backhauling ifaces. new index  k;
          uint32_t k=0;
          nlen = it->qLength_eth[j];
	  if (nlen == 255)
	    continue;
	  else
	    {
	      //this is to access to the corresponding interfaces. In order to reduce the size
	      //of the hello packet, it only reports the length of backhauling ifaces
	      if (id==0)
		//0: loopback, 1:lte, 2:connection to remote host
		k = j+3;
	      else
		k = j+1;
	      
	      nlen = GetVirtualQlengthCentralized(id, tmp, k); 
	      diffBcklg = myLength - nlen;
	      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
	      // Lyapunov-drift-plus-penalty function
	      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   
              if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
                {
                  if (maxWeight > 0.0)
                    {//now I can have negative values
                      theNeighSameWeight.push_back(&(*it));
                      eqNeighs++;
                    }
                }
              else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
                { 
                  theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
                  theNeighSameWeight.push_back(&(*it));
                  eqNeighs  = 1;
                  maxWeight = currWeight;
                }
	    } 	    
	}
	UpdateIfaceQlengthCentralized(id, it->theMainAddr);
    } //end for (NeighborSet::iterator it = m_neighborSet.begin(); 
       
    if ( eqNeighs > 1 )
      {		
        //select the local interface with the lower queue occupancy.
	//First, we check if the current output interface is a valid one
	//first check if not some of the neighbours share the same interface, 
	//otherwise, we will not be able to split traffic between different neighbors
	
	bool allNeighSameInterface = false;
	//in theory we do not have multipoint ifaces
	if (m_ipv4->GetNInterfaces() > 2 && !allNeighSameInterface)
	{
	  //we follow a local criteria to serve the packet because we have multiple interfaces
	   for (uint32_t i=0; i<eqNeighs; i++)
	   {
	      if (theNeighSameWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
	      if (output_interface == theNeighSameWeight[i]->interface)
	        { //to avoid reenqueueing
	           iface = theNeighSameWeight[i]->interface;
	           return (theNeighSameWeight[i]->neighborMainAddr);
	        }
	   }
	  uint32_t nlen, index=0, queue_space=99999;
	  uint32_t anode = LocationList::GetNodeId(currAddr);
	  Ipv4Address tmp2("10.1.200.1"); //dummy direction
	  for (uint32_t i=0; i<eqNeighs; i++)
	    {
	      if (theNeighSameWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
	      nlen = GetVirtualQlengthCentralized(anode, tmp2 ,theNeighSameWeight[i]->interface-1); 
	      if ( nlen < queue_space)
	      {
	        queue_space = nlen;
	        index = i;
	      }
	    }
	   iface = theNeighSameWeight[index]->interface;
           return (theNeighSameWeight[index]->neighborMainAddr);
	}
	else
	{ 
	  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
          uint32_t aNode = x->GetInteger(0,theNeighSameWeight.size()-1);
          NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
	  if (erased)
	    { //the flow entry expires and we keep the previous next hop if possible
	      for (uint32_t i=0; i <eqNeighs; i++)
	      {
	        if (prev_nh.IsEqual(theNeighSameWeight[i]->neighborMainAddr))
	          {
		    iface = theNeighSameWeight[i]->interface;
		    return prev_nh;
	          }
	      }
	    }
	  iface = theNeighSameWeight[aNode]->interface;
          return (theNeighSameWeight[aNode]->neighborMainAddr);
	} 
      }//end if(eqNeighs>1)
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
	return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
	return Ipv4Address ("127.0.0.1");
      }
}

/////////////////////////////////////

//GRID POINT TO POINT
Ipv4Address
BackpressureState::FindNextHopBackpressurePenaltyGridEnhanced(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface,uint32_t ttl, Ptr<Ipv4> m_ipv4, bool centraliced, uint32_t output_interface, uint32_t nodesRow, const Mac48Address &from)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  //theNeighSameWeight.reserve(MAX_NEIGHBORS);				// Up to MAX_NEIGHBORS
  theNeighSameWeight.reserve(4);				
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  Vector dPos = LocationList::GetLocation(dstAddr); 		        // Get coordinates from the destination
  double eucDistFromCurr;
//NS_LOG_UNCOND("sPos:"<<sPos<<" dPos:"<<dPos<<" currPos:"<<currPos<<" ttl:"<<ttl);
  eucDistFromCurr = CalculateDistance(dPos, currPos);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;	
  // Maximum weight initialization
  int32_t diffBcklg=0, diffNet=0; //diffnet cannot be uint because it can take negative values
  uint8_t buf_curr[4];
  uint8_t buf_neig[4];
  currAddr.Serialize(buf_curr);
    
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    
    {
      
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_neighValid.GetSeconds());
      if ( ( now.GetSeconds()-it->lastHello.GetSeconds() ) > m_neighValid.GetSeconds() )
        {
	  //std::cout<<"Neighbor no present"<<std::endl;
          continue;							//Neighbor is not present, jump to the next neighbor
        }
        
      if ( it->theMainAddr.IsEqual(dstAddr) )      //One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          iface = it->interface;
          theNextHop = it->neighborMainAddr;
          return theNextHop;
        }
      uint32_t id=105; //dummy initialization
      it->theMainAddr.Serialize(buf_neig);
      diffNet = (int32_t)buf_neig[2] - (int32_t)buf_curr[2];
      //std::cout<<"El valor de diffNet: "<<(int)diffNet<<"buff_neig:"<<(int)buf_neig[2]<<", buff_curr: "<<(int)buf_curr[2]<<",buf_curr[3]: "<<(int)buf_curr[3]<<",buf_neigh[3]: "<<(int)buf_neig[3]<<", at node: "<<anode<<"y el valor de nodesRow: "<<nodesRow<<std::endl;
      //This way of finding your id is only valid for "regular square/rectangle" deployments 
      if (diffNet==0)
        {
	  if (((int32_t)buf_neig[3]-(int32_t)buf_curr[3]) > 0)
	    id = anode + 1;
	  else
	    id = anode - 1;
	}
      else if (diffNet == -1)
        {
	  id = anode - 1;
        }
      else if (diffNet == 1)
	{
	  id = anode + 1;
	}
      else if (diffNet == (int32_t)nodesRow - 1)
	{
	  id = anode + nodesRow;
	}
      else if (diffNet == 1 - (int32_t)nodesRow)
	{
	  id = anode - nodesRow;
	}
	//std::cout<<"El valor de id: "<<id+1<<"y el valor de anode: "<<anode+1<<std::endl;
      //if (anode==16)
	//std::cout<<"El vecino con la IP: "<<it->theMainAddr<<" ID: "<<id<<std::endl;
      
      Vector nPos = Vector(it->x, it->y, 0);
      uint32_t nlen, myLength;
      Ipv4Address tmp("10.0.200.1"); //dummy direction
      nlen= GetQlengthCentralized(id,tmp);
      myLength= GetQlengthCentralized(anode,tmp);
      //std::cout<<"PENALTY GRID, myLength= "<<myLength<<", el neighLen= "<<nlen<<std::endl;
      diffBcklg = (int)myLength - (int)nlen; 			// Data queue differential on a centraliced way
      it->qLength = GetQlengthCentralized(id, tmp);	// Update Queue Backlog using centraliced info
      //std::cout<<"El valor de id: "<<id+1<<", el valor de anode: "<<anode+1<<" y el valor de theMainAddr:"<<it->theMainAddr<<", el valor de tmp: "<<tmp<<std::endl;
      if ((now.GetSeconds()-it->qPrev.GetSeconds())>0.1)
        {
          it->qLengthPrev = it->qLengthSchedPrev;
          it->qLengthSchedPrev = it->qLength;
          it->qPrev = now;
        }
      double eucDistFromNeigh;
      eucDistFromNeigh = CalculateDistance(dPos, nPos);//distance from neighbor to the destination
      
      float penalty= CalculatePenaltyNeighGrid(anode, dPos, ttl, eucDistFromCurr, eucDistFromNeigh, it->neighborhwAddr, from,dstAddr);
      
      //std::cout<<"En FNHBPGE function, From: "<<from<<", neighbor: "<<it->neighborhwAddr<<std::endl;
      if (from == it->neighborhwAddr)
      {
	m_weightV = 200;
	//std::cout<<"En FNHBPGE function, hw neigh: "<<it->neighborhwAddr<<" and from: "<<from<<std::endl;
      }
	           
      //float rateLink     = 1.0;							  // all the links have the same capacity
      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
      //std::cout<<"El valor de currWeight: "<<currWeight<<", V= "<<m_weightV<<", penalty= "<<penalty<<std::endl;
      if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
        {
           if (maxWeight > 0.0)
            {//now I can have negative values
              theNeighSameWeight.push_back(&(*it));
              eqNeighs++;
            }
        }
      else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
        { 
          theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
          theNeighSameWeight.push_back(&(*it));
          eqNeighs  = 1;
          maxWeight = currWeight;
        }
     } //end neighbor loop
       
    if ( eqNeighs > 1 )
      {		
        /*UniformRandomVariable node(0, theNeighSameWeight.size()-1); 				  // Some Candidate Neighbors can be next-hop 
        uint32_t aNode = node.GetInteger(0,theNeighSameWeight.size()-1);
        NS_LOG_DEBUG("[FindNextHopDiPUMP]: next hop is sameLength "<<theNeighSameWeight[aNode]->neighborMainAddr);
        iface = theNeighSameWeight[aNode]->interface;*/
	//select the local interface with the lower queue occupancy.
	//First, we check if the current output interface is a valid one
	for (uint32_t i=0; i<eqNeighs; i++)
	  {
	    if (output_interface == theNeighSameWeight[i]->interface)
	    {
	      iface = theNeighSameWeight[i]->interface;
	      //std::cout<<"IFACE FISTRA 2vecinos es: "<<iface<<"y la IP FISTRA es: "<<theNeighSameWeight[i]->neighborMainAddr<<std::endl;
	      return (theNeighSameWeight[i]->neighborMainAddr);
	    }
	  }
	uint32_t nlen, index=0, queue_space=99999;
	//id = m_ipv4->GetObject<Node>()->GetId();
	Ipv4Address tmp2("10.1.200.1"); //dummy direction
	for (uint32_t i=0; i<eqNeighs; i++)
	  {
	    nlen = GetVirtualQlengthCentralized(anode, tmp2 ,theNeighSameWeight[i]->interface-1); 
	    if ( nlen < queue_space)
	    {
	      queue_space = nlen;
	      index = i;
	    }
	  }
	 iface = theNeighSameWeight[index]->interface;
        return (theNeighSameWeight[index]->neighborMainAddr);
      }
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
	//iface = m_ipv4->GetInterfaceForAddress(theNeighSameWeight[0]->localIfaceAddr);
	//std::cout<<"IFACE FISTRA 1 vecino es: "<<iface<<"y la IP FISTRA es: "<<theNeighSameWeight[0]->neighborMainAddr<<std::endl;
        return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
	return Ipv4Address ("127.0.0.1");
      }
}

/////////////////////////////////////HYBRID GRID
//this is the function used in the BP-MR paper
Ipv4Address
BackpressureState::FindNextHopBackpressurePenaltyGridEnhancedHybrid(Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t qlen_max, uint32_t anode, Vector currPos, uint32_t m_weightV, uint32_t &iface,uint32_t ttl, Ptr<Ipv4> m_ipv4, bool centraliced, uint32_t output_interface, uint32_t nodesRow, const Mac48Address &from)
{
  Ipv4Address theNextHop;
  NeighborSameLengthSet theNeighSameWeight; 				// Nodes with the same length; Candidate List
  theNeighSameWeight.reserve(MAX_NEIGHBORS);					// Up to MAX_NEIGHBORS
  Time now = Simulator::Now();						// Get the current time to recalculate previous queue backlog for the variable-V algorithm
  Vector dPos = LocationList::GetLocation(dstAddr); 		        // Get coordinates from the destination
  double eucDistFromCurr= CalculateDistance(dPos, currPos);
  uint32_t eqNeighs = 0;				        	// Number of neighs with same queue length size 
  float maxWeight = 0;	
  
  for (NeighborSet::iterator it = m_neighborSet.begin();
       it != m_neighborSet.end(); it++)
    
    {
      NS_LOG_DEBUG("duration of a neighbor as valid "<<m_neighValid.GetSeconds());
      //if ( ( now.GetSeconds()-it->lastHello.GetSeconds() ) > m_neighValid.GetSeconds() )
      if ( ( now.GetSeconds()-it->lastHello.GetSeconds() ) > 1.0 )
        {
	  continue;							//Neighbor is not present, jump to the next neighbor
        }
        
      //all the nodes have the Wifi interface, so we can obtain the node identifier through this IP
      uint32_t theId=it->theMainAddr.Get();			// Calculation of the identifier of the current node
      uint32_t minId=Ipv4Address("10.0.100.0").Get();
      uint32_t id = theId-minId-1;	
      Ipv4Address tmp("10.0.200.1"); //dummy address
      
      if ( it->theMainAddr.IsEqual(dstAddr))      //One Hop to the destination 
        { //if a neighbor is my destination don't compute anything important for bidirectional flows
          if ( (GetVirtualQlengthCentralized(anode, tmp, it->interface-1) < qlen_max))
	  {
            iface = it->interface;
            theNextHop = it->neighborMainAddr;
            return theNextHop;
	  }
        }
      				
      //NS_LOG_UNCOND("the min ID is "<<minId<<" the id is "<<id);
      uint32_t myLength, nlen;
      Vector nPos = Vector(it->x, it->y, 0);
      //remember: asking info of queues are referred from 0 to n, IP address numbered from 1 to n+1 
      //because 0 is the loopback interface
      myLength =  GetVirtualQlengthCentralized(anode, tmp, it->interface-1); 
      //if (anode==12)
	//std::cout<<"MyLength: "<<myLength<<",iface: "<<it->interface<<std::endl;
      int32_t diffBcklg;
      double eucDistFromNeigh;
      eucDistFromNeigh = CalculateDistance(dPos, nPos);//distance from neighbor to the destination
      float penalty= CalculatePenaltyNeighGrid(anode, dPos, ttl, eucDistFromCurr, eucDistFromNeigh, it->neighborhwAddr, from, dstAddr);
      if (from == it->neighborhwAddr)
        {
	   m_weightV = 200;
	}
      else
       {
	 m_weightV = 200-myLength;
       }
      
      for (uint32_t j=0; j< it->n_interfaces; j++) 
        { //loop through all the interfaces of the neighbor
          nlen = it->qLength_eth[j];
	  if (nlen == 255)
	    continue;
	  else
	    {
	      nlen = GetVirtualQlengthCentralized(id, tmp, j);
	      diffBcklg = myLength - nlen;
	      it->qLength = GetQlengthCentralized(id, tmp);            	// Update Queue Backlog using centraliced info
	      if ((now.GetSeconds()-it->qPrev.GetSeconds())>0.1)
                {
                  it->qLengthPrev = it->qLengthSchedPrev;
                  it->qLengthSchedPrev = it->qLength;
                  it->qPrev = now;
                }
	      float rateLink = GetRate(anode,it->theMainAddr, m_ipv4, it->interface);
	      float currWeight   = ((float)diffBcklg - ((float)m_weightV*penalty) ) * rateLink;   // Lyapunov-drift-plus-penalty function
	      
	      /*if (anode==12 && output_interface==1)
	      {
	         //std::cout<<"(Nodo Actual,Vecino,ifaceConVecino,ifaceEvaluada,V,penalty,myLength,nLen,rateLink)"<<"("<<currAddr<<","<<it->theMainAddr<<","
		 //<<it->interface<<","<<j<<","<<m_weightV<<","<<penalty<<","<<myLength<<","<<nlen<<","<<rateLink<<")"<<std::endl;
		 //std::cout<<"en nodo 13 tengo más de un vecino y la output iface es: "<<output_interface<<std::endl;
		 std::cout<<"(NodoActual, Vecino, ifaceConVecino, ifaceEvaluada, currWeight, output_interface,destination, time)"<<"("<<currAddr<<","<<it->theMainAddr<<","
		 <<it->interface<<","<<j+1<<","<<currWeight<<","<<output_interface<<","<<dstAddr<<","<<Simulator::Now()<<std::endl;
	      }*/
	      
              if (currWeight == maxWeight)   	               					  // insert the neighbor in the List of Candidate Neighbors
                {
                  if (maxWeight > 0.0)
                    {//now I can have negative values
                      theNeighSameWeight.push_back(&(*it));
                      eqNeighs++;
                    }
                }
              else if ( currWeight > maxWeight )						  // insert the first in the List of Candidate Neighbors
                { 
                  theNeighSameWeight.erase(theNeighSameWeight.begin(), theNeighSameWeight.end()); // delete Neighbors in the List of Candidate Neighbors
                  theNeighSameWeight.push_back(&(*it));
                  eqNeighs  = 1;
                  maxWeight = currWeight;
                }
	    } 	    
	}
	UpdateIfaceQlengthCentralized(id, it->theMainAddr);
     } //end neighbor loop
       
    if ( eqNeighs > 1 )
      {		
        //select the local interface with the lower queue occupancy.
	//First, we check if the current output interface is a valid one
	//first check if not some of the neighbours share the same interface, 
	//otherwise, we will not be able to split traffic between different neighbors
	
	bool allNeighSameInterface = false;
	/*uint32_t tmp_interface= theNeighSameWeight[0]->interface;
	for (uint32_t i=1; i<eqNeighs; i++)
	{
	  if (theNeighSameWeight[i]->interface == tmp_interface)
	    allNeighSameInterface = true;
	  else
	    tmp_interface = theNeighSameWeight[i]->interface;
	}*/
	
	if (m_ipv4->GetNInterfaces() > 2 && !allNeighSameInterface)
	{
	  //we follow a local criteria to serve the packet because we have
	  //multiple interfaces
	   for (uint32_t i=0; i<eqNeighs; i++)
	   {
	      if (theNeighSameWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
	      if (output_interface == theNeighSameWeight[i]->interface)
	        { //to avoid reenqueueing
	           iface = theNeighSameWeight[i]->interface;
	           return (theNeighSameWeight[i]->neighborMainAddr);
	        }
	   }
	  uint32_t nlen, index=0, queue_space=99999;
	  Ipv4Address tmp2("10.1.200.1"); //dummy direction
	  for (uint32_t i=0; i<eqNeighs; i++)
	    {
	      if (theNeighSameWeight[i]->theMainAddr.IsEqual(dstAddr))
		  continue;
	      nlen = GetVirtualQlengthCentralized(anode, tmp2 ,theNeighSameWeight[i]->interface-1); 
	      if ( nlen < queue_space)
	      {
	        queue_space = nlen;
	        index = i;
	      }
	    }
	   iface = theNeighSameWeight[index]->interface;
           return (theNeighSameWeight[index]->neighborMainAddr);
	}
	else
	{ 
	  //TODO:select the neighbor which less elements on its queue (leaving randomness)
	  /*uint32_t theId, minId, id; 
	  uint32_t length=255, index;
	  for (uint32_t i=0; i<theNeighSameWeight.size(); i++)
	    {
	      theId= theNeighSameWeight[i]->theMainAddr.Get();
	      minId= Ipv4Address("10.0.100.0").Get();
	      id = theId-minId-1;
	      Ipv4Address tmp("10.0.200.1"); //dummy address
	      for (uint32_t j=0; j<theNeighSameWeight[i]->n_interfaces; j++)
	        {
		  if (GetVirtualQlengthCentralized(id, tmp, j) <  length)
		  {
		    length = GetVirtualQlengthCentralized(id, tmp, j);
		    index= i;
		  }
	        }
	    }
	    iface = theNeighSameWeight[index]->interface;
	    return (theNeighSameWeight[index]->neighborMainAddr);*/
	  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
          //x->SetAttribute("Min", DoubleValue(0));
	  //x->SetAttribute("Max", DoubleValue(theNeighSameWeight.size()-1));
	  uint32_t aNode = x->GetInteger(0,theNeighSameWeight.size()-1);
          NS_LOG_DEBUG("[FindNextHop]: Several Neighbors can be next hop select one randomly "<<theNeighSameWeight[aNode]->neighborMainAddr);
	  iface = theNeighSameWeight[aNode]->interface;
          return (theNeighSameWeight[aNode]->neighborMainAddr);
	} 
      }//end if(eqNeighs>1)
    else if (eqNeighs==1)
      {											  // Just One Neighbor candidate as next-hop
        iface = theNeighSameWeight[0]->interface;
	return (theNeighSameWeight[0]->neighborMainAddr);
      }
    else 
      { 										  // No Candidates do not transmit the packet 
        iface = 0;
	return Ipv4Address ("127.0.0.1");
      }
}
//////////////////
//this is the function for the SANSA multi-radio case

void 
BackpressureState::InsertArpTuple(Ptr<ArpCache> a)
{
  m_hwSet.push_back (a);
}

Mac48Address
BackpressureState::GetHwAddress(Ipv4Address const addr)
{
  Mac48Address hwaddr;
  for (std::vector<Ptr<ArpCache> >::const_iterator i = m_hwSet.begin ();
       i != m_hwSet.end (); ++i)
    {
      ArpCache::Entry * entry = (*i)->Lookup (addr);
      //if (entry != 0 && entry->IsAlive () && !entry->IsExpired ())
      if (entry != 0)
        {
          hwaddr = Mac48Address::ConvertFrom (entry->GetMacAddress ());
          break;
        }
      else
        {
          NS_LOG_DEBUG(addr<<" entry not found");
        }
    }
  return hwaddr;
}

/*uint32_t
BackpressureState::GetShadow ()
{
  return m_shadow;
}*/

uint32_t
BackpressureState::GetShadow (Ipv4Address dstAddr)
{
  uint32_t virtual_cong=0;
  std::map<Ipv4Address,uint32_t>::iterator it;
  it= m_shadow.find(dstAddr);
  if ( it!=m_shadow.end())
    {
      virtual_cong = it->second;
      //std::cout<<"Eo, paso por aquí y la virtual_cong= "<<virtual_cong<< "para el destino: " <<dstAddr<<std::endl;
    }
  return virtual_cong;
}

Ipv4Address
BackpressureState::FindtheNeighbourAtThisInterface(uint32_t interface)
{
  for (NeighborSet::iterator it= m_neighborSet.begin(); it !=m_neighborSet.end(); it++)
    {
      if (it->interface == interface)
	return it->neighborMainAddr;
    }
    return Ipv4Address ("127.0.0.1");
}

NeighborTuple*
BackpressureState::FindNeighborTuple (Ipv4Address const &mainAddr)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (it->neighborMainAddr == mainAddr)
        return &(*it);
    }
  return NULL;
}

int32_t 
BackpressureState::FindNeighborMaxQueueDiff(uint32_t id)
{
  int32_t diff=-1000; 
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if ( ((int)it->qLength-(int)it->qLengthPrev) > diff )
        {
          diff = (int)it->qLength-(int)it->qLengthPrev;
        } 
    }
  return diff;
}

int32_t
BackpressureState::FindNeighborMaxQueue(uint32_t id)
{
  int32_t maxqueue=0;
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if ( (int)it->qLength > maxqueue )
        {
          maxqueue = (int)it->qLength;
        }
    }
  return maxqueue;
}

int32_t
BackpressureState::FindNeighborMaxQueueInterface(uint32_t id)
{
  int32_t maxqueue = 0;
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      for (uint32_t i=0; i< it->n_interfaces; i++)
	{
	  if ( it->qLength_eth[i] == 255)
	    continue;
	  else
	  {
	    if ((int) it->qLength_eth[i]  > maxqueue )
	      {
	        maxqueue = (int)it->qLength_eth[i];
	      }
	  }
	}
    }
  return maxqueue;
}

int32_t
BackpressureState::FindNeighborMinQueue(uint32_t id)
{
  int32_t minqueue=999999;
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if ( (int)it->qLength < minqueue )
        {
          minqueue = (int)it->qLength;
        }
    }
  return minqueue;
}

NeighborTuple*
BackpressureState::FindNeighborTuple (Ipv4Address const &mainAddr, uint8_t willingness)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (it->neighborMainAddr == mainAddr && it->willingness == willingness)
        return &(*it);
    }
  return NULL;
}

void
BackpressureState::EraseNeighborTuple (const NeighborTuple &tuple)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (*it == tuple)
        {
          m_neighborSet.erase (it);
          break;
        }
    }
}

void
BackpressureState::EraseNeighborTuple (const Ipv4Address &mainAddr)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (it->neighborMainAddr == mainAddr)
        {
          it = m_neighborSet.erase (it);
          break;
        }
    }
}

void
BackpressureState::InsertNeighborTuple (NeighborTuple const &tuple)
{
  for (NeighborSet::iterator it = m_neighborSet.begin ();
       it != m_neighborSet.end (); it++)
    {
      if (it->neighborMainAddr == tuple.neighborMainAddr)
        {
          // Update it
          *it = tuple;
          return;
        }
    }
  m_neighborSet.push_back (tuple);
}

void
BackpressureState::CheckTerrestrialNeighbor (NeighborTuple *neighborTuple, Time now)
{
  if ( (now.GetSeconds() - neighborTuple->lastHello.GetSeconds()) > m_neighValid.GetSeconds() )
    {
      //the neighbor was lost and we have received a new hello from it, so we proceed to update the flow table
      if (m_SatNode == true)
        {
	  if (neighborTuple->interface !=1)
	    {
	      //in a satellite node, the first interface is the one connected with the satellite,
	      //which is always available
	      m_flowSet.clear();
	    }
        }
      else 
        {
	  m_flowSet.clear();
	}
      
    }
}

/********** Interface Association Set Manipulation **********/

IfaceAssocTuple*
BackpressureState::FindIfaceAssocTuple (Ipv4Address const &ifaceAddr)
{
  for (IfaceAssocSet::iterator it = m_ifaceAssocSet.begin ();
       it != m_ifaceAssocSet.end (); it++)
    {
      if (it->ifaceAddr == ifaceAddr)
        return &(*it);
    }
  return NULL;
}

const IfaceAssocTuple*
BackpressureState::FindIfaceAssocTuple (Ipv4Address const &ifaceAddr) const
{
  for (IfaceAssocSet::const_iterator it = m_ifaceAssocSet.begin ();
       it != m_ifaceAssocSet.end (); it++)
    {
      if (it->ifaceAddr == ifaceAddr)
        return &(*it);
    }
  return NULL;
}

void
BackpressureState::EraseIfaceAssocTuple (const IfaceAssocTuple &tuple)
{
  for (IfaceAssocSet::iterator it = m_ifaceAssocSet.begin ();
       it != m_ifaceAssocSet.end (); it++)
    {
      if (*it == tuple)
        {
          m_ifaceAssocSet.erase (it);
          break;
        }
    }
}

void
BackpressureState::InsertIfaceAssocTuple (const IfaceAssocTuple &tuple)
{
  m_ifaceAssocSet.push_back (tuple);
}

std::vector<Ipv4Address>
BackpressureState::FindNeighborInterfaces (const Ipv4Address &neighborMainAddr) const
{
  std::vector<Ipv4Address> retval;
  for (IfaceAssocSet::const_iterator it = m_ifaceAssocSet.begin ();
       it != m_ifaceAssocSet.end (); it++)
    {
      if (it->mainAddr == neighborMainAddr)
        retval.push_back (it->ifaceAddr);
    }
  return retval;
}

void
BackpressureState::SetSatNodeValue ( bool value)
{
  m_SatNode = value;
}

bool
BackpressureState::GetSatNodeValue ()
{
  return m_SatNode;
}

void
BackpressureState::SetTerrGwNodeValue (bool value)
{
  m_TerrGwNode = value;
}

bool 
BackpressureState::GetTerrGwNodeValue ()
{
  return m_TerrGwNode;
}

} // namespace ns3
