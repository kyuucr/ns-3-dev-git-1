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
 * Author:  José Núñez  <jose.nunez@cttc.cat>
 *         
 */

#ifndef __BACKPRESSURE_AGENT_IMPL_H__
#define __BACKPRESSURE_AGENT_IMPL_H__
#define GRID_MULTIRADIO_MULTIQUEUE

#include "backpressure-header.h"
#include "ns3/test.h"
#include "backpressure-state.h"
#include "backpressure-repositories.h"
#include "routing-packet-queue.h"
#include "fifo-routing-packet-queue.h"
#include "lifo-routing-packet-queue.h"
#include "priority-routing-packet-queue.h"
#include "ring-routing-packet-queue.h"
#include "grid-routing-packet-queue.h"

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/event-garbage-collector.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wireless-ipv4-routing-protocol.h"
#include "ns3/routing-metric.h"
#include "ns3/etx-metric.h"
#include "ns3/vector.h"
#include "ns3/mobility-model.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/random-variable-stream.h"

#include <vector>
#include <map>


/*************FLOW TYPES***************************/
const uint8_t TCP_PROT_NUMBER = 6;
const uint8_t UDP_PROT_NUMBER = 17;
/***********Backpressure_V_Policy ******************/
#define FIXED_V		0
#define VARIABLE_V_SINGLE_RADIO    1
#define VARIABLE_V_MULTI_RADIO    2
/****************************************************/
/**********Queueing Policy***************************/
#define FIFO_RQUEUE    0
#define LIFO_RQUEUE    1
#define EDF_RQUEUE     2
#define RING_RQUEUE    3
#define GRID_RQUEUE    4
/****************************************************/
/***********Backpressure Local Strategy ************/
#define GREEDY_FORWARDING		0 //SP
//#define BACKPRESSURE_CENTRALIZED_PENA	1
//#define BACKPRESSURE_CENTRALIZED_PENB	2
//#define BACKPRESSURE_CENTRALIZED_PENC	3
#define BACKPRESSURE_CENTRALIZED_PACKET	1
#define BACKPRESSURE_CENTRALIZED_FLOW	2
#define BACKPRESSURE_DISTRIBUTED_PENA	4
#define BACKPRESSURE_DISTRIBUTED_PENB	5
#define BACKPRESSURE_DISTRIBUTED_PENC	6
#define BACKPRESSURE_PENALTY_RING      7
#define MP_FORWARDING		        8 //MP
/********** Miscellaneous constants **********/

namespace ns3 {


namespace backpressure {

class RoutingProtocol : public WirelessIpv4RoutingProtocol
{
private:
  std::set<uint32_t> m_interfaceExclusions;
public:
  static TypeId GetTypeId (void);

  std::set<uint32_t> GetInterfaceExclusions () const
  {
      return m_interfaceExclusions;
  }
  void SetInterfaceExclusions (std::set<uint32_t> exceptions);

  void SendHello (); //Public operation so that class rt_metric can access it
    
  RoutingProtocol ();
  virtual ~RoutingProtocol ();

  void SetMainInterface (uint32_t interface);
  Ptr<RoutingPacketQueue> GetQueuedData ();
  
  void QueueTx();
  void QueueTxPtoP(Address);
  void TxOpportunity(Address);
  ///fire txopportunities if possible
  void FireTxOpportunities (uint32_t iface);
  uint32_t GetDataQueueSize();
  uint32_t GetDataQueueSizePrev();
  uint32_t GetVirtualDataQueueSize(uint8_t);
  uint32_t GetShadow(Ipv4Address );
 
  void SetInterfaceDown (uint32_t iface); //for energy considerations put iface down
  void SetInterfaceUp (uint32_t iface);   //for energy considerations put iface up
  void SetNodeDown (); //for energy considerations put iface down
  void SetNodeUp ();   //for energy considerations put iface up
  
  void EraseFlowTable (); //to handle dynamicity, erase flow table after change in topology
  
  ///variable to store information about interfaces
  std::map< int,int> m_infoInterfaces;
  ///the key of maps are unique, as the iface_id, I do not have to check if it is in the map
  void SetInfoInterfaces (int iface_id, int value);
  ///Reallocate the packets that are in the buffer of a antenna that is going to be switch off
  void ReallocatePackets(uint32_t interface);

  void SetSatInterface (uint32_t iface);
  
  //virtual void DoStart (void);
  //virtual void DoInitialize(void);
protected:
  //virtual void DoStart (void);
  virtual void DoInitialize(void);
private:
  /// I assume only one SAT interface per node
  std::vector<uint32_t> m_satIface;

  EventGarbageCollector m_events;

  /// Address of the routing agent
  Ipv4Address m_routingAgentAddr;
	
  /// Packets sequence number counter
  uint16_t m_packetSequenceNumber;
  /// Messages sequence number counter
  uint16_t m_messageSequenceNumber;
  
  /// HELLO messages' emission interval
  Time m_helloInterval;

  /// V recalculation interval of the variable-V algorithm
  Time m_vcalcInterval;
  
  /// Queueing Policy
  uint8_t m_queuePolicy;

  /// Max Queue Length
  uint32_t m_maxQueueLen;
  
  ///  Willingness for forwarding packets on behalf of other nodes.
  uint8_t m_willingness;

  /// Type of backpressure routing policy
  uint8_t m_variant;     

  /// Specifies whether it's a fixed-V or a variable-V routing policy 
  uint8_t m_vpolicy;     

  /// Current value of V parameter
  uint32_t m_weightV;  
  
  /// Flag to inform if the network is hybrid or not
  uint32_t m_hybrid;
  
  /// Internal state with all needed data structs.
  BackpressureState m_state;

  /// Pointer to ipv4 
  Ptr<Ipv4> m_ipv4;
  
  /// Loopback device used to defer forwarding of data packets
  Ptr<NetDevice> m_lo;
  
  /// Elements to calculate the avg. size of the Queue throughout the simulation
  //uint32_t m_queueAvg;
  std::vector<uint32_t> m_queueSizes;
  Time m_queueAvgInterval;
 
  ///This variable indicates if there is requeueing in the node.
  bool m_requeue_loop;
  
  ///Variables to perform an histeresis process
  std::vector<bool> m_histeresis;
    
  ///Variable which contains the number of nodes in a row, to determine number of interfaces per node according to geo position
  uint32_t m_nodesRow;
  
  ///Variable which stores the number of active arrays in a node
  uint32_t m_arrays;
  
  ///flag to see whether if we are in reconfiguration change after a change
  bool m_reconfState; 


  /**
   * TracedCallback signature for getting Backpressure params of interest
   *
   * \param [in] the param of interest
   */
  typedef void (* TraceBackpressureParams)
    (const uint32_t);

  /**
   * TracedCallback signature for Packet transmit and receive events.
   *
   * \param [in] header
   * \param [in] messages
   */
  typedef void (* PacketTxRxTracedCallback)
    (const PacketHeader & header, const MessageList & messages);


private:

  void Clear ();

  // From WirelessIpv4RoutingProtocol
   virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
   virtual bool RouteInput  (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, 
                             UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                             LocalDeliverCallback lcb, ErrorCallback ecb);                       
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);

  void AddCache(uint32_t i);

  void DoDispose ();
  void PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const;
  void SendPacket (Ptr<Packet> packet, const MessageList &containedMessages);
  	
  /// Increments packet sequence number and returns the new value.
  inline uint16_t GetPacketSequenceNumber ();
  /// Increments message sequence number and returns the new value.
  inline uint16_t GetMessageSequenceNumber ();
	
  void RecvHelloBackpressure (Ptr<Socket> socket);

  Ipv4Address GetMainAddress (Ipv4Address iface_addr) const;

  // Timer handlers
  Timer m_helloTimer;
  Timer m_vcalcTimer;
  Timer m_avgqueueTimer;
  Timer m_reconfTimer; //timer to wait before starting transmission after a reconfiguration
  
  void HelloTimerExpire ();
  void VcalcTimerExpire ();
  void VcalcTimerExpire (uint32_t direction, bool loop);
  void AvgQueueTimerExpire ();
  void ReconfTimerExpire ();
  
  //get wifi net device: return null if wifi net device is not present
  Ptr<WifiNetDevice> IsWifi(Ptr<NetDevice> dev, uint32_t i); 
  //get wifi queue: return null if wifi queue not present
  Ptr<WifiMacQueue> GetWifiQueue(Ptr<NetDevice> dev, uint32_t i); 
  // get mac queue size 
  int GetMacQueueSize(Ptr<NetDevice> dev); 
  
  bool m_linkTupleTimerFirstTime;
  void IfaceAssocTupleTimerExpire (Ipv4Address ifaceAddr);

  /// A list of pending messages which are buffered awaiting for being sent.
  backpressure::MessageList m_queuedMessages;
  Ptr<RoutingPacketQueue> m_queuedData;
  Timer m_queuedMessagesTimer; // timer for throttling outgoing messages

  void QueueMessage (const backpressure::MessageHeader &message, Time delay);
  void QueueData (Ptr<const Packet> packet,const Ipv4Header &header,const Mac48Address &from, UnicastForwardCallback ucb);
  void QueueData (Ptr<const Packet> packet,const Ipv4Header &header,const Mac48Address &from, UnicastForwardCallback ucb, uint32_t direction, uint32_t requeued, Time time);
  
  void SendQueuedMessages ();
  void SendQueuedData (Ptr<Ipv4Route> route);
  
  void AddNeighborTuple (const NeighborTuple &tuple);
  void RemoveNeighborTuple (const NeighborTuple &tuple);
  void AddIfaceAssocTuple (const IfaceAssocTuple &tuple);
  void RemoveIfaceAssocTuple (const IfaceAssocTuple &tuple);

  void ProcessHello (const backpressure::MessageHeader &msg,
                     const Ipv4Address &receiverIface,
                     const Ipv4Address &senderIface);
  /// Create loopback route for given header
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header) const;

  void PopulateNeighborSet (const Ipv4Address &receiverIface,
                            const Ipv4Address &senderIface,
                            const backpressure::MessageHeader &msg,
                            const backpressure::MessageHeader::Hello &hello);
  /// Check that address is one of my interfaces
  bool IsMyOwnAddress (const Ipv4Address & a) const;

  /// Determine the output interface of a packet according to geolocation a)Ring b)Grid
   uint32_t OutputIfaceDeterminationGrid (Ptr<Ipv4> m_ipv4,uint8_t  ttl, Ipv4Address const &currAddr, Ipv4Address const &srcAddr, Ipv4Address const &dstAddr, uint32_t &check_requeue, Mac48Address from);
   uint32_t OutputIfaceDeterminationGridSANSALenaMR(Ptr<Ipv4>, Ipv4Address const &currAddr, Ipv4Address const &dstAddr, Mac48Address from, bool SatFlow, uint32_t &check_requeue);
  /// Map the Mac Address of a net device to the IP interface of this net device
  void MapMACAddressToIPinterface (Address MAC, uint32_t &interface);
  uint32_t AlternativeIfaceSelection (Ptr<Ipv4> , uint32_t, std::vector<double> &distances);
  uint32_t AlternativeIfaceSelectionSANSA (Ptr<Ipv4> , uint32_t, Ipv4Address ); //std::vector<uint32_t> &distances);
  ///Determine the source port and the destination port of an incoming/stored packet: currently UDP, TCP and GTP protocols supported
  void GetSourceAndDestinationPort(Ipv4Header header, Ptr<Packet> packet, uint16_t &s_port, uint16_t &d_port);
  ///Determine if a neighbor is valid or not in the SANSA scenario
  bool IsValidNeighborSANSA (Time lastHello, uint32_t interface, bool SatFlow, bool SatNode);

  Ipv4Address m_mainAddress;

  // One socket per interface, each bound to that interface's address
  // (reason: for BACKPRESSURE Link Sensing we need to know on which interface
  // HELLO messages arrive)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
  
  //Tracing for backpressure statistics

  TracedCallback <const PacketHeader &,
                  const MessageList &> m_rxPacketTrace;
  TracedCallback <const PacketHeader &,
                  const MessageList &> m_txPacketTrace;
  TracedCallback <uint32_t> m_routingTableChanged;
  TracedCallback <uint32_t, std::vector<uint32_t>, uint32_t > m_qLengthTrace;
  TracedCallback <uint32_t> m_vTrace;
  TracedCallback <uint32_t> m_qOverflowTrace;
  TracedCallback <uint32_t> m_expTTL;
  TracedCallback <uint32_t> m_queueAvg;
  //TracedCallback <const Ipv4Header &, Ptr<const Packet> > m_TxData;
  TracedCallback <const Ipv4Header &, Ptr<const Packet>, uint32_t > m_TxData;
  TracedCallback <Ptr<const Packet>, Ipv4Header, int64_t, uint32_t > m_TxDataAir;
  TracedCallback <Ipv4Address, Ipv4Address, Ptr<const Packet> > m_TxBeginData;
  TracedCallback <const Ipv4Header &, Ptr<const Packet> > m_TxEnd;
  
  ///this variable is needed to generate the jitter when sending the hello messages
  Ptr<UniformRandomVariable> m_uniformRandomVariable; 
  
  
};

} // namespace backpressure
} // namespace ns3

#endif
