/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Authors: 
 *  Jose Núñez-Martínez <jose.nunez@cttc.cat>,
 */

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/location-list.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LocationList");

std::map<Ipv4Address, Vector> LocationList::m_locations;
std::map<Ipv4Address, uint32_t> LocationList::m_ids;
std::map<Ipv4Address, uint32_t> LocationList::m_Gws;
std::map<Ipv4Address, uint32_t> LocationList::m_SatGws;
Ptr<ShortestPathRoutingTable> LocationList::m_sprt;
Ptr<ShortestPathRoutingTable> LocationList::m_sprtsat;
bool LocationList::m_reset;


LocationList::~LocationList ()
{  
}

void
LocationList::Add (Ipv4Address addr, Vector pos)
{
  NS_LOG_DEBUG("the address is "<<addr);
  NS_LOG_DEBUG("the x position is "<<pos.x<<" the y position is "<<pos.y);
  LocationList::m_locations.insert(std::make_pair (addr, pos));  
}

void 
LocationList::AddNodeId (Ipv4Address addr, uint32_t id)
{
  NS_LOG_DEBUG("the address is "<<addr);
  NS_LOG_DEBUG("the node id is: "<<id);
  LocationList::m_ids.insert(std::make_pair (addr, id));
}

void 
LocationList::AddGw (Ipv4Address addr, uint32_t gw_id)
{
  NS_LOG_DEBUG("the address is "<<addr);
  NS_LOG_DEBUG("the id of the node connected to the gw is "<<gw_id);
  LocationList::m_Gws.insert(std::make_pair (addr, gw_id));
}

void 
LocationList::AddSatGw (Ipv4Address addr, uint32_t gw_id)
{
  NS_LOG_DEBUG("the address is "<<addr);
  NS_LOG_DEBUG("the id of the node connected to the gw is "<<gw_id);
  LocationList::m_SatGws.insert(std::make_pair (addr, gw_id));
}

LocationList::Iterator 
LocationList::Begin (void) 
{
  return m_locations.begin ();
}

LocationList::Iterator 
LocationList::End (void)
{
  return m_locations.end ();
}

Vector
LocationList::GetLocation (Ipv4Address addr)
{
  return m_locations[addr];
}

uint32_t
LocationList::GetGwId (Ipv4Address addr)
{
  return m_Gws[addr];
}

uint32_t
LocationList::GetSatGwId (Ipv4Address addr)
{
  return m_SatGws[addr];
}

uint32_t
LocationList::GetNodeId (Ipv4Address addr)
{
  return m_ids[addr];
}

void
LocationList::SetRoutingTable (Ptr<ShortestPathRoutingTable> srt)
{
  m_sprt = srt;
}

void
LocationList::SetRoutingTableSat (Ptr<ShortestPathRoutingTable> srt)
{
  m_sprtsat = srt;
}

uint32_t
LocationList::GetHops (Ipv4Address src_addr, Ipv4Address dst_addr)
{
  //std::cout<<"La distancia entre src:"<<src_addr<< ", y dst: "<<dst_addr<< " es: "<<m_sprt->GetDistance(src_addr, dst_addr)<<std::endl;
  return uint32_t (m_sprt->GetDistance(src_addr, dst_addr));
}

uint32_t
LocationList::GetHops (Ipv4Address src_addr, Ipv4Address dst_addr, bool SatFlow)
{
  
  if (SatFlow)
    {
      //std::cout<<"La distancia con satelite entre src:"<<src_addr<< ", y dst: "<<dst_addr<< " es: "<<m_sprtsat->GetDistance(src_addr, dst_addr)<<std::endl;
      return uint32_t (m_sprtsat->GetDistance(src_addr, dst_addr));
    }
  else
    {
      //std::cout<<"La distancia terrestre entre src:"<<src_addr<< ", y dst: "<<dst_addr<< " es: "<<m_sprt->GetDistance(src_addr, dst_addr)<<std::endl;
      return uint32_t (m_sprt->GetDistance(src_addr, dst_addr));
    }
}

/*void
LocationList::PrintLocationMap ()
{
  std::cout << "Address\t\tCoordinates\n";
}*/

void
LocationList::PrintNodeIds ()
{
  std::cout << "Address\t\tId\n";
  for (std::map<Ipv4Address, uint32_t>::iterator it = m_ids.begin();
       it != m_ids.end (); it++)
    {
      std::cout << it->first << "\t\t";
      std::cout << it->second << "\n";
    }
}

void
LocationList::PrintGws ()
{
  std::cout << "GW Address\t\tId\n";
  for (std::map<Ipv4Address, uint32_t>::iterator it = m_Gws.begin();
       it != m_Gws.end (); it++)
    {
      std::cout << it->first << "\t\t";
      std::cout << it->second << "\n";
    }
}

void
LocationList::SetResetLocationList( bool value)
{
  m_reset = value;
}

bool
LocationList::GetResetLocationList ()
{
  return m_reset;
}

}
