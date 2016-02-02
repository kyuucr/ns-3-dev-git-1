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


#ifndef POINT_TO_POINT_RING_HELPER_H
#define POINT_TO_POINT_RING_HELPER_H

#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/integer.h"
#include <vector>

#define RATE_CHANGE_CLOCKWISE 1
#define RATE_CHANGE_COUNTERCLOCKWISE 2
#define RATE_CHANGE_BOTH 3


using namespace std;
namespace ns3 {
/**
* \defgroup pointtopointlayout Point-to-Point Layout Helpers
*
*/

/**
 * \ingroup pointtopointlayout
 *
 * \brief A helper to make it easier to create a ring topology
 * with PointToPoint links
 */
class PointToPointRingHelper{

public:
  /**
   * Create a PointToPointStarHelper in order to easily create
   * ring topologies using p2p links
   *
   * \param num_nodes the number of nodes present in the ring architecture
   *
   *
   * \param p2pHelper the link helper for p2p links, 
   *        used to link nodes together. They are connected in an incremental
   *        way, the first with the second, then the second with the third. Finally, the 
   *        last one is connected with the first one to close the ring
   */
  PointToPointRingHelper (uint32_t num_nodes,PointToPointHelper p2pHelper,bool full_clique = false,bool random_clique = false);
  PointToPointRingHelper (){};
  ~PointToPointRingHelper (){};

  /**
   * 
   * Sets up the node canvas location for every node in the ring.
   * 
   *  \param ulx upper left x value
   *  \param uly upper left y value
   *  \param lrx lower right x value
   *  \param lry lower right y value
   * 
   */
  void BoundingBox (double ulx, double uly, double lrx, double lry);

  /**
   * 
   *  Install the InternetStack in each node of the NodeContainer
   * 
   *  \param stack the InternetStackHelper object to install in each node of the NodeContainer
   * 
   */
  void InstallStack (InternetStackHelper stack);

  /**
   * 
   *  Assign an IP address in each node of the NodeContainer. 
   * 
   *  \param ip  the Ipv4AddressHelper object to assign an IP to each node of the NodeContainer
   * 
   */
  void AssignIpv4Addresses (Ipv4AddressHelper ip);

  /**
   *  \param index the identifier 
   * 
   *  \returns pointer to the node indicated by index 
   */
  Ptr <Node> Get (uint32_t index);

  /**
   *  \returns The number of nodes in the nodecontainer variable 
   * 
   */
  uint32_t GetN ();

  /**
   *  \returns pointer to the first node in the ring
   * 
   */
  Ptr <Node> GetFirst ();

  /**
   *  \returns pointer to the last node in the ring
   * 
   */
  Ptr <Node> GetLast ();

  /**
   *  \returns the nodecontainer of the ring nodes 
   * 
   */
  NodeContainer GetNodeContainer ();

  /**
   *  \returns the vector of NetDeviceContainer, useful to make loop using the iterator construction 
   * 
   */
  std::vector<NetDeviceContainer> GetNetDeviceContainer ();

  /**
   *  This function breaks the canvas of the node location in order to insert holes in the network.
   *
   *  \param nNodeHoles number of holes in the current ring topology
   *
   *  \param theHoles  vector with the index of the nodes which constitute a hole in the ring
   * 
   */
  void CreateHoles (uint32_t nNodeHoles, std::vector<string> theHoles);

  void RingDivision ( NetDeviceContainer &setDevices1, NetDeviceContainer &setDevices2);
  void BreakLink (uint32_t breakLink, uint32_t breakDirection, string breakLinkRate, NetDeviceContainer &setDevices1, NetDeviceContainer &setDevices2);

private:
  uint32_t m_num_nodes;
  NodeContainer m_ringContainer;
  std::vector <NetDeviceContainer> m_nd;
  void P2PConnectHelper (PointToPointHelper p2pHelper,bool full_clique = false,bool random_clique = false);
};


} // namespace ns3
#endif
