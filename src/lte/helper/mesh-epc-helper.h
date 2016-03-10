/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jose Nuñez <jose.nunez@cttc.es>
 */

#ifndef MESH_EPC_HELPER_H
#define MESH_EPC_HELPER_H

#include <vector>
//#include <ns3/object.h>
#include <ns3/config.h>
#include <ns3/simulator.h>
#include <ns3/names.h>
#include <ns3/node.h>
#include <ns3/node-container.h>
#include "ns3/ipv4-list-routing-helper.h"
#include <ns3/ipv4-address-helper.h>
#include <ns3/data-rate.h>
#include <ns3/epc-tft.h>
#include <ns3/eps-bearer.h>
#include <ns3/epc-helper.h>
#include <ns3/error-model.h>

namespace ns3 {
//class Node;
class NetDevice;
class VirtualNetDevice;
class EpcSgwPgwApplication;
class EpcX2;
class EpcMme;

/**
 * \ingroup lte
 * \brief Create an EPC network with Mesh links
 *
 * This Helper will create an EPC network topology comprising of a
 * single node that implements both the SGW and PGW functionality, and
 * an MME node. The S1-U, X2-U and X2-C interfaces are realized over
 * Mesh network amongst SCs. 
 */
class MeshEpcHelper : public EpcHelper
{
public:

  /** 
   * Constructor
   */
  MeshEpcHelper ();

  /** 
   * Destructor
   */
  virtual ~MeshEpcHelper ();

  // inherited from Object
  /**
   *  Register this type.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  // inherited from EpcHelper
  virtual void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId);
  virtual void AddUe (Ptr<NetDevice> ueLteDevice, uint64_t imsi);
  virtual void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);
  virtual uint8_t ActivateEpsBearer (Ptr<NetDevice> ueLteDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer);
  virtual Ptr<Node> GetPgwNode ();
  virtual Ipv4InterfaceContainer AssignUeIpv4Address (NetDeviceContainer ueDevices);
  virtual Ipv4Address GetUeDefaultGatewayAddress ();

  // creation of mesh topology amongst enbs and satellite nodes
  void AddHybridMeshBackhaul (NodeContainer enbs, std::vector<std::vector<int> > terrestrialConMatrix, std::vector<bool> terrestrialEPC, std::vector<bool> terrestrialSat, Ipv4ListRoutingHelper routingList /*NodeContainer satellites*/);

  static void TraceAndDebug (Ptr<NetDevice> first, Ptr<NetDevice> second);

private:

  /** 
   * helper to assign addresses to UE devices as well as to the TUN device of the SGW/PGW
   */
  Ipv4AddressHelper m_ueAddressHelper; 

  /**
   * SGW-PGW network element
   */
  Ptr<Node> m_sgwPgw; 

  /**
   * SGW-PGW application
   */
  Ptr<EpcSgwPgwApplication> m_sgwPgwApp;

  /**
   * TUN device implementing tunneling of user data over GTP-U/UDP/IP
   */
  Ptr<VirtualNetDevice> m_tunDevice;

  /**
   * MME network element
   */
  Ptr<EpcMme> m_mme;

  /**
   * S1-U interfaces
   */

  /** 
   * helper to assign addresses to S1-U NetDevices 
   */
  Ipv4AddressHelper m_s1uIpv4AddressHelper; 

  /**
   * The data rate to be used for the next S1-U link to be created
   */
  DataRate m_s1uLinkDataRateTerrestrial;
  DataRate m_s1uLinkDataRateSatellite;

  /**
   * The delay to be used for the next S1-U link to be created
   */
  Time     m_s1uLinkDelayTerrestrial;
  Time     m_s1uLinkDelaySatellite;

  /**
   * The MTU of the next S1-U link to be created. Note that,
   * because of the additional GTP/UDP/IP tunneling overhead,
   * you need a MTU larger than the end-to-end MTU that you
   * want to support.
   */
  uint16_t m_s1uLinkMtuTerrestrial;
  uint16_t m_s1uLinkMtuSatellite;

  /**
   * UDP port where the GTP-U Socket is bound, fixed by the standard as 2152
   */
  uint16_t m_gtpuUdpPort;

  /**
   * Map storing for each IMSI the corresponding eNB NetDevice
   */
  std::map<uint64_t, Ptr<NetDevice> > m_imsiEnbDeviceMap;

  /** 
   * helper to assign addresses to X2 NetDevices 
   */
  Ipv4AddressHelper m_x2Ipv4AddressHelper;

  /**
   * The data rate to be used for the next X2 link to be created
   */
  DataRate m_x2LinkDataRate;

  /**
   * The delay to be used for the next X2 link to be created
   */
  Time     m_x2LinkDelay;

  /**
   * The MTU of the next X2 link to be created. Note that,
   * because of some big X2 messages, you need a big MTU.
   */
  uint16_t m_x2LinkMtu;
};




} // namespace ns3

#endif // MESH_EPC_HELPER_H
