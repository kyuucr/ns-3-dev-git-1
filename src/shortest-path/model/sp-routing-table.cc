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

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "sp-routing-table.h"
#include "ns3/mobility-model.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/constant-rate-wifi-manager.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/ipv4.h"
#include <vector>
#include <boost/lexical_cast.hpp>

using namespace std;

NS_LOG_COMPONENT_DEFINE ("SpRoutingTable");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ShortestPathRoutingTable);

TypeId
ShortestPathRoutingTable::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ShortestPathRoutingTable")
    .SetParent<Object> ()
    .AddConstructor<ShortestPathRoutingTable> ()
    ;
  return tid;
}

ShortestPathRoutingTable::ShortestPathRoutingTable ()
{
  m_spNext = 0;
  m_spDist = 0;
  m_txRange = 0;
}
ShortestPathRoutingTable::~ShortestPathRoutingTable ()
{}

void 
ShortestPathRoutingTable::AddRoute (Ipv4Address srcAddr, Ipv4Address relayAddr, Ipv4Address dstAddr)
{
  NS_LOG_FUNCTION (srcAddr << relayAddr << dstAddr);
  SPtableEntry se;
  se.srcAddr = srcAddr;
  se.relayAddr = relayAddr;
  se.dstAddr = dstAddr;
  
  m_sptable.push_back (se);
}

void
ShortestPathRoutingTable::AddNode (Ptr<Node> node, Ipv4Address addr)
{
  NS_LOG_FUNCTION ("node: " << node << " addr: " << addr);
  SpNodeEntry sn;
  sn.node = node;
  sn.addr = addr;
  sn.eth = 1; // interfaces up
  m_nodeTable.push_back (sn);
}

void
ShortestPathRoutingTable::AddNodeHoles (Ptr<Node> node, Ipv4Address addr, bool ethUp)
{
  NS_LOG_FUNCTION ("node: " << node << " addr: " << addr << " eth: " << ethUp);
  SpNodeEntry sn;
  sn.node = node;
  sn.addr = addr;
  sn.eth = ethUp; // interfaces up
  m_nodeTable.push_back (sn);
}



Ipv4Address
ShortestPathRoutingTable::LookupRoute (Ipv4Address srcAddr, Ipv4Address dstAddr)
{
    uint16_t i = 0, j = 0;
    Ipv4Address relay;
    std::vector<SpNodeEntry>::iterator it = m_nodeTable.begin ();
    
    for (; it != m_nodeTable.end ();++it)
      {
        if (it->addr == srcAddr)
          {
            break;
          }
        i++;
      }
    it = m_nodeTable.begin ();
    for (; it != m_nodeTable.end ();++it)
      {
        if (it->addr == dstAddr)
          {
            break;
          }
        j++;
      }
    
    uint16_t k;
    do
      {
        k = j;
        j = m_spNext [i * m_nodeTable.size() + j];
        NS_LOG_INFO ("@@ " << i << " " << j << " " << k);
      }
    while (i != j);
    
    if (DistFromTable (i, k) > m_txRange)
      {
	if (DistFromTableSansaLena(i, k) !=1.0) 
	  { //this function checks if there is any interface with connection between i and k nodes
	    std::cout<<"No Path Exists!"<<std::endl;
            NS_LOG_DEBUG ("No Path Exists!");
	    return srcAddr;
	  }
	  else
	    return m_nodeTable.at(k).addr;
      }
    
    SpNodeEntry se = m_nodeTable.at (k);
    return se.addr;
}

// Find shortest paths for all pairs using Floyd-Warshall algorithm 

void 
ShortestPathRoutingTable::UpdateRoute (double txRange)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
        {
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTable (i, j);
              if (distance > 0 && distance <= txRange)
                {
	                //dist [i * n + j] = distance; // shortest distance
                  dist [i * n + j] = 1; // shortest hop
                }
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
        {
          for (j = 0; j < n; j++)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
                {
                  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
                }
            }
        }
    }
  
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
}
/////////////////////////////////////////////////////////////////////////////////
void 
ShortestPathRoutingTable::UpdateRouteETT (double txRange, NetDeviceContainer &wifiDevices)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;
  double packet_size = 1500;
  uint64_t rate;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = 0; i < n; i++)
    {
      Ptr<WifiNetDevice> aWifiDev = (wifiDevices.Get(i))->GetObject<WifiNetDevice> ();
      Ptr<WifiRemoteStationManager> manager = aWifiDev->GetRemoteStationManager();
      Ptr<ConstantRateWifiManager> constant = DynamicCast<ConstantRateWifiManager> (manager);
      rate = constant->GetDataMode().GetDataRate(aWifiDev->GetPhy()->GetChannelWidth (), 
          aWifiDev->GetPhy()->GetGuardInterval (), 1);
      for (j = 0; j < n; j++)
        {
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTable (i, j);
              if (distance > 0 && distance <= txRange)
                {
	                //dist [i * n + j] = distance; // shortest distance
                  //dist [i * n + j] = 1; // shortest hop
                  dist [i * n + j] = packet_size/((double) rate);
                }
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
        {
          for (j = 0; j < n; j++)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
                {
                  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
                }
            }
        }
    }
  
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
}

///////////////////The route is checked from the beginning of the table///////////
void 
ShortestPathRoutingTable::UpdateRouteBegin (double txRange)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
        {
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTable (i, j);
              if (distance > 0 && distance <= txRange)
                {
	                //dist [i * n + j] = distance; // shortest distance
                  dist [i * n + j] = 1; // shortest hop
                }
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
        {
          for (j = 0; j < n; j++)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
                {
                  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
                }
            }
        }
    }
  
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
}
////////////////////////The condition if the interfaces is activated is checked ///////////////
void 
ShortestPathRoutingTable::UpdateRouteBeginGrid (double txRange)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = 0; i < n; i++)
    { //rows
      for (j = 0; j < n; j++)
        {//columns
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTable (i, j);
              if (distance > 0 && distance <= txRange)
	      //if(distance >=0 && distance <= txRange)
                {
	                //dist [i * n + j] = distance; // shortest distance
                  dist [i * n + j] = 1; // shortest hop
                }
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
        {
          for (j = 0; j < n; j++)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
	      //if (distance < dist[i * n + j])
                {
		  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
		  /*if (m_nodeTable.at(i).node->GetId()==0)
		  {
		    std::cout<<"Nodo i: "<<(m_nodeTable.at(i).node)->GetObject<MobilityModel> ()->GetPosition () <<"Nodo j: "<< (m_nodeTable.at(j).node)->GetObject<MobilityModel> ()->GetPosition ()<<std::endl;
		    std::cout<<"la distancia es: "<<dist[i * n + j]<<"el pred es: "<<pred[i * n + j]<<std::endl;
		  }*/
	  
                }
            }
        }
    }
  
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
    //exit(-1);
}

////////////////////////The condition if the interfaces is activated is checked ///////////////
void 
ShortestPathRoutingTable::UpdateRouteSansaGrid (double txRange)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = 0; i < n; i++)
    { //rows
      for (j = 0; j < n; j++)
        {//columns
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTableSansa (i, j);
              if (distance > 0 && distance <= txRange)
	      //if(distance >=0 && distance <= txRange)
                {
	                //dist [i * n + j] = distance; // shortest distance
                  dist [i * n + j] = 1; // shortest hop
                }
                //for sansa
              else if (distance == 0)
		  dist [i* n + j] = 0; 
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
        {
          for (j = 0; j < n; j++)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
	      //if (distance < dist[i * n + j])
                {
		  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
		  /*if (m_nodeTable.at(i).node->GetId()==0)
		  {
		    std::cout<<"Nodo i: "<<(m_nodeTable.at(i).node)->GetObject<MobilityModel> ()->GetPosition () <<"Nodo j: "<< (m_nodeTable.at(j).node)->GetObject<MobilityModel> ()->GetPosition ()<<std::endl;
		    std::cout<<"la distancia es: "<<dist[i * n + j]<<"el pred es: "<<pred[i * n + j]<<std::endl;
		  }*/
	  
                }
            }
        }
    }
  
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
}

//////////////////////////////////////////////////////////////////////////
void 
ShortestPathRoutingTable::UpdateRouteSansaLena (double txRange)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = 0; i < n; i++)
    { //rows
      for (j = 0; j < n; j++)
        {//columns
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTableSansaLena (i, j);
              if (distance > 0 && distance <= txRange)
	        {
                  dist [i * n + j] = 1; // shortest hop
                }
                //for sansa
              else if (distance == 0)
		  dist [i* n + j] = 0; 
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
        {
          for (j = 0; j < n; j++)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
                {
		  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
	  
                }
            }
        }
    }
  
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
    //exit(-1);
}



///////////////////The route is checked from the end of the table///////////

void 
ShortestPathRoutingTable::UpdateRouteEnd (double txRange)
{
  NS_LOG_FUNCTION ("");

  m_txRange = txRange;
  uint16_t n = m_nodeTable.size(); // number of nodes
  int16_t i, j, k; //loop counters
  double distance;

  //initialize data structures
  if (m_spNext != 0)
    {
      delete m_spNext;
    }
  if (m_spDist != 0)
    {
      delete m_spDist;
    }

  double* dist = new double [n * n];
  uint16_t* pred = new uint16_t [n * n];

  //algorithm initialization
  for (i = n-1; i >=0 ; i--)
    {
      //std::cout<<"El valor de i es: "<<i<<std::endl;
      for (j = n-1; j >= 0; j--)
        {
	  //std::cout<<"El valor de j es: "<<j<<std::endl;
          if (i == j)
            {
              dist [i * n + j] = 0;
            }
          else
            {
              distance = DistFromTable (i, j);
              if (distance > 0 && distance <= txRange)
                {
	                //dist [i * n + j] = distance; // shortest distance
                  dist [i * n + j] = 1; // shortest hop
                }
              else
                {
	                dist [i * n + j] = HUGE_VAL;
                }
              pred [i * n + j] = i;
            }
        }
    }
    
  // Main loop of the algorithm
  for (k = n-1; k >=0; k--)
    {
      for (i = n-1; i>=0; i--)
        {
          for (j = n-1; j>=0; j--)
            {
              distance = std::min (dist[i * n + j], dist[i * n + k] + dist[k * n + j]);
              if (distance != dist[i * n + j])
                {
                  dist[i * n + j] = distance;
                  pred[i * n + j] = pred[k * n + j];
                }
            }
        }
    }
    
  m_spNext = pred; // predicate matrix, useful in reconstructing shortest routes
  m_spDist = dist;
  
  string str;
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
      {
        str.append (boost::lexical_cast<string>( pred[i * n + j] ));
        str.append (" ");
      }
      NS_LOG_INFO (str);
      str.erase (str.begin(), str.end());
    }
}


// Get direct-distance between two nodes
double
ShortestPathRoutingTable::GetDistance (Ipv4Address srcAddr, Ipv4Address dstAddr)
{
    uint16_t i = 0, j = 0;
    uint16_t n = m_nodeTable.size(); // number of nodes
    Ipv4Address relay;
    std::vector<SpNodeEntry>::iterator it = m_nodeTable.begin ();
    
    for (; it != m_nodeTable.end ();++it)
      {
        if (it->addr == srcAddr)
          {
            break;
          }
        i++;
      }
    it = m_nodeTable.begin ();
    for (; it != m_nodeTable.end ();++it)
      {
        if (it->addr == dstAddr)
          {
            break;
          }
        j++;
      }
    
    return m_spDist [i * n + j];
}

double 
ShortestPathRoutingTable::DistFromTable (uint16_t i, uint16_t j)
{
  Vector pos1, pos2;
  Ptr<Ipv4> m_ipv4_1, m_ipv4_2;
  
  SpNodeEntry se1,se2;
  double dist;
  uint8_t buff_i[4], buff_j[4];
  int32_t diffNet;
  uint32_t k,l;
  
  se1 = m_nodeTable.at (i);
  pos1 = (se1.node)->GetObject<MobilityModel> ()->GetPosition ();
  m_ipv4_1 = (se1.node)->GetObject<Ipv4>();
  se2 = m_nodeTable.at (j);
  pos2 = (se2.node)->GetObject<MobilityModel> ()->GetPosition ();
  m_ipv4_2 = (se2.node)->GetObject<Ipv4>();
    
  for (k=1; k<m_ipv4_1->GetNInterfaces(); k++)
    { //0 is the loopback interface
      m_ipv4_1->GetAddress(k,0).GetLocal().Serialize(buff_i);
      for (l=1; l<m_ipv4_2->GetNInterfaces(); l++)
        {
	  m_ipv4_2->GetAddress(l,0).GetLocal().Serialize(buff_j);
	  diffNet=(int32_t)buff_i[2] - (int32_t)buff_j[2];
	  if (diffNet==0)
	    {
	      if (m_ipv4_1->IsUp(k) && m_ipv4_2->IsUp(l))
	        {
	          dist = pow (pos1.x - pos2.x, 2.0) + pow (pos1.y - pos2.y, 2.0) + pow (pos1.z - pos2.z, 2.0);
		  //std::cout<<"Ip : "<<se1.addr<<" conectada a: "<<se2.addr<<std::endl;
		  return sqrt(dist);
	        }
	    }
	  
	}
    }
  return 999999999.9;
}
  
double 
ShortestPathRoutingTable::DistFromTableSansa (uint16_t i, uint16_t j)
{
  Vector pos1, pos2;
  Ptr<Ipv4> m_ipv4_1, m_ipv4_2;
  
  SpNodeEntry se1,se2;
  uint8_t buff_i[4], buff_j[4];
  int32_t diffNet;
  uint32_t k,l;
  
  se1 = m_nodeTable.at (i);
  pos1 = (se1.node)->GetObject<MobilityModel> ()->GetPosition ();
  m_ipv4_1 = (se1.node)->GetObject<Ipv4>();
  se2 = m_nodeTable.at (j);
  pos2 = (se2.node)->GetObject<MobilityModel> ()->GetPosition ();
  m_ipv4_2 = (se2.node)->GetObject<Ipv4>();
    
  for (k=1; k<m_ipv4_1->GetNInterfaces(); k++)
    { //0 is the loopback interface
      m_ipv4_1->GetAddress(k,0).GetLocal().Serialize(buff_i);
      for (l=1; l<m_ipv4_2->GetNInterfaces(); l++)
        {
	  m_ipv4_2->GetAddress(l,0).GetLocal().Serialize(buff_j);
	  diffNet=(int32_t)buff_i[2] - (int32_t)buff_j[2];
	  if (diffNet==0)
	    {
	      if (m_ipv4_1->IsUp(k) && m_ipv4_2->IsUp(l))
	        {
	          return sqrt(1.0);
	        }
	    }
	  
	}
    }
  return 999999999.9;
}
  
double 
ShortestPathRoutingTable::DistFromTableSansaLena (uint16_t i, uint16_t j)
{
  Ptr<Ipv4> m_ipv4_1, m_ipv4_2;
  
  SpNodeEntry se1,se2;
  uint8_t buff_i[4], buff_j[4];
  int32_t diffNet;
  uint32_t k,l;
  
  se1 = m_nodeTable.at (i);
  m_ipv4_1 = (se1.node)->GetObject<Ipv4>();
  se2 = m_nodeTable.at (j);
  m_ipv4_2 = (se2.node)->GetObject<Ipv4>();
    
  for (k=1; k<m_ipv4_1->GetNInterfaces(); k++)
    { //0 is the loopback interface
      m_ipv4_1->GetAddress(k,0).GetLocal().Serialize(buff_i);
      for (l=1; l<m_ipv4_2->GetNInterfaces(); l++)
        {
	  m_ipv4_2->GetAddress(l,0).GetLocal().Serialize(buff_j);
	  diffNet=(int32_t)buff_i[3] - (int32_t)buff_j[3];
	  if (diffNet==1 || diffNet==-1)
	    {
	      if (m_ipv4_1->IsUp(k) && m_ipv4_2->IsUp(l))
	        {
		  //std::cout<<"IPsrc: "<<m_ipv4_1->GetAddress(1,0).GetLocal()<<", IPdst: "<<m_ipv4_2->GetAddress(1,0).GetLocal()<<" connected"<<std::endl;
	          return sqrt(1.0);
	        }
	    }
	  
	}
    }
  return 999999999.9;
}
  
uint32_t 
ShortestPathRoutingTable::CheckRoute(Ipv4Address srcAddr, Ipv4Address dstAddr)
{
  Ipv4Address src, relay;
  uint32_t route_valid;
  src = srcAddr;
  
  while (src !=dstAddr)
    {
      relay = LookupRoute(src, dstAddr);
      if (relay == src)
        {
	  //there is no path from the original src to the destination
	  //std::cout<<"There is no path from the original src to the destination"<<std::endl;
	  route_valid = 0;
	  return route_valid;
        }
      else if (relay == dstAddr)
        {
	  //there is one path from the original src to the destination
	  //std::cout<<"There is a path"<<std::endl;
	  route_valid = 1;
	  return route_valid;
        }
      else
	{
	  src = relay;
	  route_valid = 0;
	}
    }
    //std::cout<<"se llega aquí??"<<std::endl;
    return route_valid;
}



void
ShortestPathRoutingTable::Print (Ptr<OutputStreamWrapper> stream) const
{
  NS_LOG_FUNCTION ("");
  // not implemented
}

} // namespace ns3
