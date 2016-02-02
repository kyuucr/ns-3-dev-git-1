/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
* Author: John Abraham <john.abraham@gatech.edu>
* Tuned by: Jordi Baranda <jorge.baranda@cttc.es>
*/

#include "ns3/point-to-point-helper.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/log.h"
#include "ns3/point-to-point-ring.h"
#include "ns3/constant-position-mobility-model.h"

NS_LOG_COMPONENT_DEFINE ("PointToPointRingHelper");

namespace ns3 {
PointToPointRingHelper::PointToPointRingHelper (uint32_t num_nodes,PointToPointHelper p2pHelper,bool full_clique,bool random_clique)
{
  m_ringContainer.Create (num_nodes);
  P2PConnectHelper (p2pHelper,full_clique, random_clique);
}
void PointToPointRingHelper::P2PConnectHelper (PointToPointHelper p2pHelper,bool full_clique,bool random_clique)
{
  if(!(full_clique ||random_clique))
    {
      for(uint32_t i = 0; i < m_ringContainer.GetN ()-1; i++)
        {
          NS_LOG_UNCOND ("connect node "<<i<<" with "<<(i+1));
          m_nd.push_back (p2pHelper.Install (m_ringContainer.Get (i), m_ringContainer.Get (i+1)));
        }
      NS_LOG_UNCOND ("connect node 0 with "<<(m_ringContainer.GetN ()-1));
      m_nd.push_back (p2pHelper.Install (m_ringContainer.Get (m_ringContainer.GetN ()-1), m_ringContainer.Get (0)));
    }
  else
    {
      for(uint32_t i = 0; i < m_ringContainer.GetN ()-1; i++)
        {
          for(uint32_t j = 0; j < m_ringContainer.GetN (); j++)
            {
              if(j == i) continue;
              uint32_t lowerbound = full_clique ? 1 : 0;
              Ptr <UniformRandomVariable> v = CreateObject<UniformRandomVariable> ();
              v->SetAttribute ("Min", IntegerValue (lowerbound));
              v->SetAttribute ("Max", IntegerValue (1));
              if(v->GetInteger ()) {
                  m_nd.push_back (p2pHelper.Install (m_ringContainer.Get (i), m_ringContainer.Get (j)));
                }
              else
                {
                  std::cout<<"Not connecting"<<std::endl;
                }
            } //For 2
        } //for 1
    }
}

Ptr <Node> PointToPointRingHelper::Get (uint32_t index)
{
  NS_ASSERT (index < m_ringContainer.GetN ());
  return m_ringContainer.Get (index);
}
 
uint32_t PointToPointRingHelper::GetN ()
{
  return m_ringContainer.GetN ();
}

Ptr <Node> PointToPointRingHelper::GetFirst ()
{
  return m_ringContainer.Get (0);
}

Ptr <Node> PointToPointRingHelper::GetLast ()
{
  return m_ringContainer.Get (m_ringContainer.GetN ()-1);
}
 
NodeContainer PointToPointRingHelper::GetNodeContainer ()
{
  return m_ringContainer;
}
 
std::vector<NetDeviceContainer> PointToPointRingHelper::GetNetDeviceContainer ()
{
  return m_nd;
}

 
void PointToPointRingHelper::CreateHoles (uint32_t nNodeHoles, std::vector<string> theHoles)
{
  for (uint32_t i=0; i<nNodeHoles; i++)                 // if holes in the ring
    {
      uint32_t theNodeHole = atoi (theHoles[i].c_str ());
      NS_LOG_INFO ("adding a node hole "<<theNodeHole);
      Ptr<Node> node = m_ringContainer.Get (theNodeHole);
      Ptr<ConstantPositionMobilityModel> theMobility1 =  node->GetObject<ConstantPositionMobilityModel>();
      theMobility1->SetPosition (Vector (1000,1000,0));
    }
}


void PointToPointRingHelper::InstallStack (InternetStackHelper stack)
{
  stack.Install (m_ringContainer);
}

void PointToPointRingHelper::AssignIpv4Addresses (Ipv4AddressHelper ip)
{
  for (uint32_t i = 0; i < m_nd.size (); i++)
    {
      ip.Assign (m_nd[i]);
      ip.NewNetwork ();
    }
}

void PointToPointRingHelper::BoundingBox (double ulx, double uly, double lrx, double lry)
{
  double yDist;
  double xDist;
  double centerX,centerY,radiusX,radiusY;
  if (lrx > ulx)
    {
      xDist = lrx - ulx;
    }
  else
    {
      xDist = ulx - lrx;
    }
  if (lry > uly)
    {
      yDist = lry - uly;
    }
  else
    {
      yDist = uly - lry;
    }
  centerX = xDist/2;
  centerY = yDist/2;
  radiusX = centerX;
  radiusY = centerY;
  double angle = (2 * M_PI)/ m_ringContainer.GetN ();
  for (uint32_t l = 0; l < m_ringContainer.GetN (); l++)
    {
      Ptr<Node> n = m_ringContainer.Get (l);
      Ptr<ConstantPositionMobilityModel> loc = n->GetObject<ConstantPositionMobilityModel> ();
      if (loc == 0)
        {
          loc = CreateObject<ConstantPositionMobilityModel> ();
          n->AggregateObject (loc);
        }
      Vector vec (centerX + radiusX * cos (angle * l), centerY + radiusY * sin (angle * l), 0);
      loc->SetPosition (vec);
    }

}
 
void PointToPointRingHelper::RingDivision ( NetDeviceContainer &setDevices1, NetDeviceContainer &setDevices2)
{
  uint8_t set=1;
  for (std::vector<NetDeviceContainer>::iterator it = m_nd.begin (); it != m_nd.end (); ++it)
    {
      if (set==1)
        {
          setDevices1.Add (*it);
          set=2;
        }
      else
        {
          setDevices2.Add (*it);
          set=1;
        }
    }
}
 
void PointToPointRingHelper::BreakLink (uint32_t breakLink, uint32_t breakDirection, string breakLinkRate, NetDeviceContainer &setDevices1, NetDeviceContainer &setDevices2)
{
  if (breakLink > 0)
    {
      Ptr<NetDevice> nd1,nd2;
      if (breakLink % 2 ==0)
        {
          NS_LOG_DEBUG ("link number: "<<breakLink<<" is on setDevices 2");
          nd1 = setDevices2.Get (breakLink-1);
          nd2 = setDevices2.Get (breakLink-2);
        }
      else //odd link number
        {
          NS_LOG_DEBUG ("link number: "<<breakLink<<" is on setDevices 1");
          nd1 = setDevices1.Get (breakLink);
          nd2 = setDevices1.Get (breakLink-1);
        }
      Ptr<PointToPointNetDevice> p2pnd1,p2pnd2;
      p2pnd1 = nd1->GetObject<PointToPointNetDevice> ();
      p2pnd2 = nd2->GetObject<PointToPointNetDevice> ();
      if (breakDirection==RATE_CHANGE_CLOCKWISE)
        {
          p2pnd1->SetDataRate (DataRate (breakLinkRate));
          NS_LOG_DEBUG ("change rate on clockwise direction");
        }
      else if (breakDirection==RATE_CHANGE_COUNTERCLOCKWISE) 
        {
          NS_LOG_UNCOND ("change rate counter clocwise");
          p2pnd2->SetDataRate (DataRate (breakLinkRate));
        }
      else
        { //CHANGE_BOTH_RATES
          NS_LOG_DEBUG ("change both");
          std::cout<<"Lo cambiamos todo"<<std::endl;
          p2pnd1->SetDataRate (DataRate (breakLinkRate));
          p2pnd2->SetDataRate (DataRate (breakLinkRate));
        }
    }
}

} // namespace ns3
