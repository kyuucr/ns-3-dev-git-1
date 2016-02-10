/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Jośe Núñez <jose.nunez@cttc.cat>
 */
#include "backpressure-helper.h"
#include "ns3/backpressure.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3 {

BackpressureRoutingHelper::BackpressureRoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::backpressure::RoutingProtocol");
}

BackpressureRoutingHelper::BackpressureRoutingHelper (const BackpressureRoutingHelper &o)
  : m_agentFactory (o.m_agentFactory)
{
}

BackpressureRoutingHelper* 
BackpressureRoutingHelper::Copy (void) const 
{
  return new BackpressureRoutingHelper (*this); 
}

void
BackpressureRoutingHelper::ExcludeInterface (Ptr<Node> node, uint32_t interface)
{
  std::map< Ptr<Node>, std::set<uint32_t> >::iterator it = m_interfaceExclusions.find (node);

  if(it == m_interfaceExclusions.end ())
    {
      std::set<uint32_t> interfaces;
      interfaces.insert (interface);

      m_interfaceExclusions.insert (std::make_pair (node, std::set<uint32_t> (interfaces) ));
    }
  else
    {
      it->second.insert (interface);
    }
}

Ptr<Ipv4RoutingProtocol> 
BackpressureRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<backpressure::RoutingProtocol> agent = m_agentFactory.Create<backpressure::RoutingProtocol> ();

  std::map<Ptr<Node>, std::set<uint32_t> >::const_iterator it = m_interfaceExclusions.find (node);

  if (it != m_interfaceExclusions.end ())
  {
    agent->SetInterfaceExclusions (it->second);
  }
  node->AggregateObject (agent);
  return agent;
}

void 
BackpressureRoutingHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

} // namespace ns3
