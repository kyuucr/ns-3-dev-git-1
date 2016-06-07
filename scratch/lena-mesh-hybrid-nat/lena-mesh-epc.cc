/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jose Núñez <jose.nunez@cttc.cat>
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"
#include "ns3/netanim-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/antenna-module.h"
#include "ns3/propagation-module.h"
#include "ns3/olsr-module.h"

//Configure for tracing
#include "ns3/config.h"

//Backpressure routing protocol
#include "ns3/backpressure.h"
#include "ns3/backpressure-helper.h"
#include "ns3/backpressure-tracing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

//Location Table
#include "ns3/location-list.h"

#include "ns3/gnuplot.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("LenaBackpressureIntegration");

typedef std::map<uint32_t, Gnuplot2dDataset*> UintDataSet;
typedef std::pair<uint32_t, Gnuplot2dDataset*> UintDataPair;
typedef std::map<uint32_t, std::string> StringDataSet;
typedef std::pair<uint32_t, std::string> StringDataPair;


uint64_t totalBytes = 0;
uint32_t activeSocket = 0;
uint32_t errorSocket = 0;

static void
FreeUintDataSet (UintDataSet &set)
{
  for (UintDataSet::iterator it = set.begin(); it != set.end(); ++it)
    {
      Gnuplot2dDataset *set = it->second;
      delete set;
      set = 0;
    }

  set.erase (set.begin(), set.end());
}


struct PtrForPrinting
{
  StringDataSet *idToName;
  std::string prefix;
  std::string postfix;
  bool shouldAggregate;
};

enum AppMode
{
  CBR,
  FTP,
  FTP_SAT,
  CONVERSATIONAL,
  INTERACTIVE
};

class NulStreambuf : public std::streambuf
{
  char                dummyBuffer[ 64 ];
protected:
  virtual int         overflow ( int c )
  {
    setp ( dummyBuffer, dummyBuffer + sizeof( dummyBuffer ) );
    return (c == traits_type::eof ()) ? '\0' : c;
  }
};

class NulOStream : private NulStreambuf,
    public std::ostream
{
public:
  NulOStream () : std::ostream ( this )
  {
  }
  const NulStreambuf* rdbuf () const
  {
    return this;
  }
};


class AppProperties
{
public:
  Ptr<Node> from;
  Ptr<Node> to;
  Time start;
  Time end;
  std::string socketType;
  AppMode mode;
  std::string rate;
};


void
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at " << pos.x << "," << pos.y << " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at " << pos.x << "," << pos.y
                      << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

static void
TraceGoodput (UintDataSet *goodputSet, bool shouldAggregate,
              Ptr<const Packet> p, const Address &addr)
{
  //Ipv4Address from = InetSocketAddress::ConvertFrom (addr).GetIpv4 ();
  uint16_t port = InetSocketAddress::ConvertFrom (addr).GetPort ();

  // Per-flow stats
  UintDataSet::iterator it = goodputSet->find (port);
  Gnuplot2dDataset *set;
  NS_ASSERT (it != goodputSet->end ());

  set = it->second;
  set->Add (Simulator::Now ().GetSeconds (), p->GetSize ());

  if (! shouldAggregate)
    {
      return;
    }

  // Aggregate stats
  it = goodputSet->find (0);
  NS_ASSERT (it != goodputSet->end ());

  set = it->second;
  set->Add (Simulator::Now ().GetSeconds (), p->GetSize ());

  totalBytes -= p->GetSize();
  if (totalBytes == 0)
    {
      Simulator::Stop ();
    }
}

static void
TraceRtt (Gnuplot2dDataset *set, Time oldRtt, Time newRtt)
{
  set->Add (Simulator::Now ().GetSeconds (), newRtt.GetSeconds ());
}

static void
TraceCwnd (Gnuplot2dDataset *set, uint32_t oldValue, uint32_t newValue)
{
  set->Add (Simulator::Now().GetSeconds(), newValue);
}

static void
VTrace (Gnuplot2dDataset *set, uint32_t v)
{
  set->Add (Simulator::Now().GetSeconds(), v);
}

static void
QueueTrace (UintDataSet *queueSet, uint32_t length,
            std::vector<uint32_t> IfacesSizes, uint32_t n_interfaces)
{
  if (queueSet->size() == 0)
    {
      for (uint32_t i=0; i<n_interfaces; ++i)
        {
          Gnuplot2dDataset *dataset = new Gnuplot2dDataset ();
          queueSet->insert(UintDataPair(i, dataset));
        }
    }

  NS_ASSERT_MSG (queueSet->size() == n_interfaces,
                 "Interface number changed. uff");

  for (uint32_t i=0; i<n_interfaces; ++i)
    {
      queueSet->at(i)->Add (Simulator::Now().GetSeconds(), IfacesSizes.at(i));
    }
}

static void
NormalCloseCb (Ptr<Socket> s)
{
  NS_ASSERT (activeSocket > 0);
  activeSocket -= 1;

  if (activeSocket == 0)
    {
      Simulator::Stop();
    }
}

static void
ErrorCloseCb (Ptr<Socket> s)
{
  NS_ASSERT (activeSocket > 0);
  activeSocket -= 1;
  errorSocket += 1;
}

static Ptr<Socket>
CreateConnectedSocket (Ptr<Node> node, const std::string &socketType,
                       Gnuplot2dDataset *rttData = 0, Gnuplot2dDataset *cwndData = 0)
{
  NS_ASSERT (node != 0);
  Ptr<Socket> s = Socket::CreateSocket (node, TypeId::LookupByName (socketType));

  if (socketType.compare ("ns3::TcpSocketFactory") == 0)
    {
      if (rttData != 0)
        {
          s->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&TraceRtt, rttData));
        }
      if (cwndData != 0)
        {
          s->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&TraceCwnd, cwndData));
        }
    }

  s->SetCloseCallbacks (MakeCallback (&NormalCloseCb),
                        MakeCallback (&ErrorCloseCb));

  return s;
}


static void
InstallSinks (NodeContainer &nodes, uint16_t port, const std::string &transportProt,
              double stop_time, UintDataSet *goodputSet = 0, bool shouldAggregate = true)
{
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper (transportProt, sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes);
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (stop_time));
  for (ApplicationContainer::Iterator it = sinkApp.Begin (); it != sinkApp.End (); ++it)
    {
      Ptr<Application> app = (*it);
      if (goodputSet != 0)
        {
          app->TraceConnectWithoutContext ("Rx",
                                           MakeBoundCallback (&TraceGoodput, goodputSet, shouldAggregate));
        }
    }
}

void
PopulateArpCache (NodeContainer nodes)
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Years (1));
  for (NodeContainer::Iterator i = nodes.Begin (); i!= nodes.End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip !=0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for(ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); ++j)
        {
          Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
          NS_ASSERT (ipIface != 0);
          Ptr<NetDevice> device = ipIface->GetDevice ();
          NS_ASSERT (device != 0);
          Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress ());
          for(uint32_t k = 0; k < ipIface->GetNAddresses (); ++k)
            {
              Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal ();
              if(ipAddr == Ipv4Address::GetLoopback () || ipAddr.IsEqual ("7.0.0.1") )
                continue;
              ArpCache::Entry * entry = arp->Add (ipAddr);
              entry->SetMacAddresss (addr);
              entry->MarkPermanent ();
            }
        }
    }

  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip !=0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for(ObjectVectorValue::Iterator j = interfaces.Begin (); j !=
          interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp));
        }
    }
}

void
PopulateLocationMapThroughPos (NodeContainer nodes)
{
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv4>   theIpv4  = node->GetObject<Ipv4>();
      Ipv4Address theAddr  = theIpv4->GetAddress (1, 0).GetLocal ();
      Ptr<ConstantPositionMobilityModel> theMobility = node->GetObject<ConstantPositionMobilityModel>();
      Vector pos = theMobility->GetPosition ();
      LocationList::Add (theAddr, pos);
    }
}

void
PopulateLocationMapThroughHops (NodeContainer enbNodes, std::vector<bool> theGws,
                                std::vector<bool> theSatGws, double distance)
{
  Ptr<ShortestPathRoutingTable> srt = CreateObject<ShortestPathRoutingTable> ();
  Ptr<ShortestPathRoutingTable> srtsat = CreateObject<ShortestPathRoutingTable> ();
  LocationList::SetRoutingTable (srt);
  LocationList::SetRoutingTableSat (srtsat);
  //we need also the epc node to establish the logical link. Epc node is the
  //first element of the node list
  NodeContainer backhaul;
  backhaul.Add (NodeList::GetNode (0)); //epc node
  backhaul.Add (NodeList::GetNode (1)); //remoteHost
  backhaul.Add (enbNodes); //if there is satellite, there is an extra element
  uint32_t nNodes = backhaul.GetN ();
  NS_LOG_DEBUG ( " Number of nodes in the enb Container (+epc node & remoteHost):" << nNodes);
  Ptr<Node> node;
  Ptr<Ipv4> theIpv4;
  Ipv4Address theAddr;
  uint32_t i,j,k;
  //0.1-adding the terrestrial nodes connected to the epc
  //first check if it is not empty, if it has an element, erase it

  if (!(LocationList::m_Gws.empty ()))
    {
      LocationList::m_Gws.clear ();
    }

  for (k=0; k<theGws.size (); k++)
    {
      if (theGws[k])
        {
          NS_LOG_UNCOND ( " Terrestrial GW Node:" << k+1);
          node = enbNodes.Get (k);
          theIpv4 = node->GetObject<Ipv4>();
          NS_LOG_UNCOND ( " Ip address Terrestrial GW Node: " << theIpv4->GetAddress (1,0).GetLocal () );
          LocationList::AddGw (theIpv4->GetAddress (1,0).GetLocal (), node->GetId ());
        }
    }

  //0.2-adding the terrestrial nodes connected to the satellite gw (extra enodeB)
  //first check if it is not empty, if it has an element, erase it
  if (!(LocationList::m_SatGws.empty ()))
    {
      LocationList::m_SatGws.clear ();
    }

  int SatGW = std::count (theSatGws.begin (), theSatGws.end (), 1);

  for (k=0; k<theSatGws.size (); k++)
    {
      if (theSatGws[k])
        {
          NS_LOG_UNCOND ( " Terrestrial node with satellite interface:" << k+1);
          node = enbNodes.Get (k);
          theIpv4 = node->GetObject<Ipv4>();
          NS_LOG_UNCOND ( " Ip address terrestrial node with satellite interface: " << theIpv4->GetAddress (1,0).GetLocal () );
          LocationList::AddSatGw (theIpv4->GetAddress (1,0).GetLocal (), node->GetId ());
        }
    }

  //1-populating the table
  //first check if it is not empty, if it has an element, erase it
  if (!(LocationList::m_ids.empty ()))
    {
      LocationList::m_ids.clear ();
    }

  for (i=0; i< nNodes; i++)
    { //the first  node is the "EPC" node
      node = backhaul.Get (i);
      theIpv4 = node->GetObject<Ipv4>();
      for (j=1; j<theIpv4->GetNInterfaces (); j++)
        {  //starts from one because interface 0 is the loopback
          theAddr = theIpv4->GetAddress (j,0).GetLocal ();
          LocationList::AddNodeId (theAddr, node->GetId ());
          //in the satellite table we put all the nodes
          srtsat->AddNodeHoles (node, theAddr, theIpv4->IsUp (j));
          //in the terrestrial table we put all the nodes, except the specialEnodeb if exists
          if (SatGW!=0)
            {
              if (i!=(nNodes-1))
                {
                  srt->AddNodeHoles (node, theAddr, theIpv4->IsUp (j));
                }
            }
          else
            {
              srt->AddNodeHoles (node, theAddr, theIpv4->IsUp (j));
            }

        }
    }
  //2-initializing the routes in the table (the distance value can be passed as a parameter)
  srt->UpdateRouteSansaLena (distance + 0.2);
  srtsat->UpdateRouteSansaLena (distance + 0.2);
}

static void
StringSplit (std::string str, std::string delim, std::vector<std::string> &results)
{
  uint32_t cutAt;
  while( (cutAt = str.find_first_of (delim)) < str.length () )
    {
      if(cutAt > 0)
        {
          results.push_back (str.substr (0,cutAt));
        }
      str = str.substr (cutAt+1);
    }
  if(str.length () > 0)
    {
      results.push_back (str);
    }
}

static std::vector<bool>
readVector (std::string terr_epc_file_name)
{
  std::ifstream terr_epc_file;
  terr_epc_file.open (terr_epc_file_name.c_str (), std::ios::in);
  if (terr_epc_file.fail ())
    {
      NS_FATAL_ERROR ("File " << terr_epc_file_name.c_str () << " not found");
    }
  std::vector<bool> array;

  std::string line;
  getline (terr_epc_file, line);

  std::istringstream iss (line);
  bool element;
  std::vector<bool> row;

  while (iss >> element)
    {
      array.push_back (element);
    }
  terr_epc_file.close ();
  return array;

}

static std::vector<std::vector <double> >
readCoordinates (std::string topo_coords_file_name, uint32_t limit = 0)
{
  std::ifstream topo_coords_file;
  topo_coords_file.open (topo_coords_file_name.c_str (), std::ios::in);
  if (topo_coords_file.fail () )
    {
      NS_FATAL_ERROR ("File " << topo_coords_file_name.c_str () << " not found");
    }
  std::vector<std::vector<double> > array;
  uint32_t i = 0;
  while (!topo_coords_file.eof ())
    {
      std::string line;
      getline (topo_coords_file, line);

      if (line == "" || line.at (0) == '/')
        {
          NS_LOG_WARN ("WARNING: Ignoring blank row in the array: " << array.size () <<
                       ": " << line);
          continue;
        }

      std::istringstream iss (line);
      double element;
      std::vector<double> coords;
      int j = 0;

      while (iss >> element)
        {
          coords.push_back (element);
          j++;
        }
      if (j != 2)
        {
          //we are missing one coordinate
          NS_FATAL_ERROR ("ERROR: Number of elements in line " << array.size () <<
                          ": " << j << " not equal to 2, missing a coordinate");
        }
      else
        {
          array.push_back (coords);
        }
      ++i;

      if (limit != 0 && limit == i)
        {
          break;
        }
    }

  topo_coords_file.close ();
  return array;
}


static std::vector<std::vector<int> >
readNxNMatrix (std::string adj_mat_file_name)
{
  std::ifstream adj_mat_file;
  adj_mat_file.open (adj_mat_file_name.c_str (), std::ios::in);
  if (adj_mat_file.fail ())
    {
      NS_FATAL_ERROR ("File " << adj_mat_file_name.c_str () << " not found");
    }
  std::vector<std::vector<int> > array;
  int i = 0;
  int n_nodes = 0;

  while (!adj_mat_file.eof ())
    {
      std::string line;
      getline (adj_mat_file, line);
      if (line == "")
        {
          NS_LOG_WARN ("WARNING: Ignoring blank row in the array: " << i << line);
          break;
        }

      std::istringstream iss (line);
      int element;
      std::vector<int> row;
      int j = 0;

      while (iss >> element)
        {
          row.push_back (element);
          j++;
        }

      if (i == 0)
        {
          n_nodes = j;
        }

      if (j != n_nodes )
        {
          NS_LOG_ERROR ("ERROR: Number of elements in line " << i << ": " << j << " not equal to number of elements in line 0: " << n_nodes);
          NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
        }
      else
        {
          array.push_back (row);
        }
      i++;
    }

  if (i != n_nodes)
    {
      NS_LOG_ERROR ("There are " << i << " rows and " << n_nodes << " columns.");
      NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
    }

  adj_mat_file.close ();
  return array;
}

static void
PrintNodeInfo (Ptr<Node> n)
{
  Ptr<ConstantPositionMobilityModel> loc;
  loc = DynamicCast<ConstantPositionMobilityModel> (n->GetObject<ConstantPositionMobilityModel>());
  if (loc != 0)
    {
      NS_LOG_UNCOND ("Node " << n->GetId () << " has name " << Names::FindName (n) <<
                     " and position " << loc->GetPosition ());
    }
  else
    {
      NS_LOG_UNCOND ("Node " << n->GetId () << " has name " << Names::FindName (n) <<
                     " and position empty");
    }
}

static void
SetNodeProperties (Ptr<Node> node, double x, double y, double z, const std::string &name)
{
  Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel> ();
  if (loc == 0)
    {
      loc = CreateObject<ConstantPositionMobilityModel> ();
      node->AggregateObject (loc);
    }
  loc->SetPosition (Vector3D (x, y, z));  // 5, 2
  Names::Add (name, node);
}

/**
 * app = [app_def]#[app_def]# ...
 * [app_def] = [from|to|start|end|type|socket]
 *
 * from = UE id (starting from 1) or remote
 * to   = UE id (starting from 1) to remote
 * start = start time
 * end   = end time
 * type  = cbr (more to come)
 * socket = tcp or udp
 */
static void
ParseApp (const std::string &app, std::vector<AppProperties*> &definitions)
{
  std::vector<std::string> appVector;
  StringSplit (app, "#", appVector);

  NS_ASSERT (appVector.size() > 0);

  std::vector<std::string>::iterator it;
  for (it = appVector.begin(); it != appVector.end(); ++it)
    {
      std::string app_def = (*it);
      std::vector<std::string> app_properties;
      StringSplit (app_def, "|", app_properties);

      NS_ABORT_MSG_UNLESS (app_properties.size() == 6, "App properties should be 6");

      AppProperties *p = new AppProperties;

      // remove extra '[' at the beginning
      std::string from = app_properties.at(0).substr(1);
      std::string to = app_properties.at(1);

      p->from = Names::Find<Node> (from);
      NS_ABORT_MSG_UNLESS (p->from != 0, "Node " << to << " does not exists");

      p->to   = Names::Find<Node> (to);
      NS_ABORT_MSG_UNLESS (p->to != 0, "Node " << to << " does not exists");

      p->start = Time (app_properties.at(2));
      p->end   = Time (app_properties.at(3));

      if (app_properties.at(4).substr(0,3).compare ("cbr") == 0)
        {
          p->mode = CBR;
          p->rate = app_properties.at(4).substr(3,std::string::npos);
        }
      else if (app_properties.at(4).compare ("conversational") == 0)
        {
          p->mode = CONVERSATIONAL;
        }
      else if (app_properties.at(4).compare ("interactive") == 0)
        {
          p->mode = INTERACTIVE;
        }
      else if (app_properties.at(4).compare ("ftp") == 0)
        {
          p->mode = FTP;
        }
      else if (app_properties.at(4).compare ("ftp_sat") == 0)
        {
          p->mode = FTP_SAT;
        }
      else
        {
          NS_FATAL_ERROR ("Mode " << app_properties.at(4) << " not recognized."
                          "Valid values are cbr, conversational, interactive");
        }

      // Remove extra ']' at the end
      p->socketType = app_properties.at(5).substr(0, app_properties.at(5).size()-1);

      definitions.push_back(p);
    }
}

static void
OutputGnuplot (Gnuplot2dDataset *dataset, const std::string &fileName,
               bool shouldAggregate = false)
{
  std::ofstream dataFile;
  dataFile.open (fileName.c_str(), std::fstream::out | std::fstream::app);

  if (shouldAggregate)
    {
      uint32_t n = 0;
      double sum = 0;

      dataset->SumYValues (sum, n);
      double x = dataset->GetLastX ();

      if (x == 0.0)
        {
          x = Simulator::Now().GetSeconds();
        }

      dataFile << x << " " << sum << std::endl;
    }
  else
    {
      Gnuplot plot;
      plot.AddDataset (*dataset);
      NulOStream str;
      plot.GenerateOutput (str, dataFile, fileName);
    }

  dataFile.close ();
}

void
PrintUintDataStats (UintDataSet &data, PtrForPrinting ptr)
{
  for (UintDataSet::iterator it = data.begin(); it != data.end(); ++it)
    {
      StringDataSet::iterator it_name = ptr.idToName->find (it->first);

      if (it_name != ptr.idToName->end ())
        {
          OutputGnuplot ((it->second), ptr.prefix + it_name->second + ptr.postfix,
                         ptr.shouldAggregate);
          it->second->Clear();
        }
      else if (it->first == 0)
        {
          OutputGnuplot ((it->second), ptr.prefix + "agg" + ptr.postfix,
                         ptr.shouldAggregate);
          it->second->Clear();
        }
      else
        {
          NS_FATAL_ERROR ("FLOW to port " << it->first << " NOT RECOGNIZED ");
        }
    }
}

static void
PeriodicFreeUintDataStats (UintDataSet &data, PtrForPrinting ptr)
{
  PrintUintDataStats(data, ptr);
  Simulator::Schedule (Seconds (1.0), &PeriodicFreeUintDataStats, data, ptr);
}

/*
static std::string
ToString (Ipv4Address addr)
{
  std::stringstream string;
  addr.Print (string);
  return string.str ();
}
*/

const std::string CurrentDateTime()
{
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

  return buf;
}

static void
PrintTime (const std::string &name)
{
  std::ofstream dataFile;
  dataFile.open (name.c_str (), std::ios_base::out | std::ios_base::app);
  dataFile << Simulator::Now().GetSeconds() << " " << CurrentDateTime ()
           << " still to transmit: " << totalBytes << " active socket: "
           << activeSocket << " error socket: " << errorSocket << std::endl;
  dataFile.close();

  Simulator::Schedule (Seconds (1.0), &PrintTime, name);
}

void ConfigureBp (BackpressureRoutingHelper &backpressure,
                  uint32_t theV, uint32_t localStrategy, uint32_t algV)
{
  backpressure.Set("HelloInterval", TimeValue(Seconds(0.1)));
  backpressure.Set("VcalInterval", TimeValue(Seconds(0.01)));
#ifdef GRID_MULTIRADIO_MULTIQUEUE
  backpressure.Set("QPolicy", EnumValue(GRID_RQUEUE));
#else
  backpressure.Set("QPolicy", EnumValue(FIFO_RQUEUE));
#endif

  backpressure.Set ("MaxQueueLen", UintegerValue (theV));
  backpressure.Set("RowNodes", UintegerValue(2));
  switch (localStrategy)
    {
    case (0):
      {
        backpressure.Set("LocalStrategy", EnumValue(GREEDY_FORWARDING));
        break;
      }
    case (1):
      {
        backpressure.Set("LocalStrategy", EnumValue(BACKPRESSURE_PENALTY_RING));
        break;
      }
    case (2):
      {
        backpressure.Set("LocalStrategy", EnumValue(BACKPRESSURE_CENTRALIZED_PACKET));
        break;
      }
    case (3):
      {
        backpressure.Set("LocalStrategy", EnumValue(BACKPRESSURE_CENTRALIZED_FLOW));
        break;
      }
    case (4):
      {
        backpressure.Set("LocalStrategy", EnumValue(BACKPRESSURE_DISTRIBUTED_PENA));
        break;
      }
    case (5):
      {
        backpressure.Set("LocalStrategy", EnumValue(BACKPRESSURE_DISTRIBUTED_PENB));
        break;
      }
    case (6):
      {
        backpressure.Set("LocalStrategy", EnumValue(BACKPRESSURE_DISTRIBUTED_PENC));
        break;
      }
    default:
      break;
    }
  switch (algV)
    {
    case (0)://fixed-v
      {
        backpressure.Set("VStrategy", EnumValue(FIXED_V));
        break;
      }
    case (1)://var-v for Single Radio
      {
        backpressure.Set("VStrategy", EnumValue(VARIABLE_V_SINGLE_RADIO));
        break;
      }
    case (2)://var-v for Multi Radio
      {
        backpressure.Set("VStrategy", EnumValue(VARIABLE_V_MULTI_RADIO));
        break;
      }
    default:
      break;
    }
  backpressure.Set("V", UintegerValue(theV));
}

/*
ns3::PacketMetadata::Enable();
ns3::Packet::EnableChecking();
*/

int
main (int argc, char *argv[])
{
  // Logging
  // LogComponentEnable ("MeshEpcHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LenaBackpressureIntegration", LOG_LEVEL_INFO);

  //Setting buffers to BDP (bandwidth delay product)
  Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue (375000));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (375000));
  //configuring tcp socket
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (5000000));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (5000000));
  Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (0));
  Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (1));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (2));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
  Config::SetDefault ("ns3::olsr::RoutingProtocol::HelloInterval", TimeValue (MilliSeconds (100)));

  Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (LteEnbRrc::RLC_UM_ALWAYS));

  uint32_t mtu_bytes=1400;
  uint32_t tcp_adu_size = mtu_bytes;

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));

  double simTime = 50;
  double distance = 60.0;
  uint32_t replication = 1;
  std::string dirResultsFile ("./out/");
  std::string app ("[remote|UE1|4.0|15.0|cbr]");
  std::string tcpCong ("ns3::TcpNewReno");
  uint32_t ftpBytes (0);
  uint32_t initialSsTh (UINT32_MAX);
  uint32_t meshQ (0);
  bool rem = false;
  bool netAnim = false;
  bool bpStat = false;
  std::string routingProtocol ("bp");
  uint32_t scenario = 0; // 0 queue-rtt, 1 scalability

  uint32_t algV=0, localStrategy=2, theV=200;
  uint16_t numberOfUEs = 0;

  // Command line arguments
  NS_LOG_INFO ("[Main]: parsing line command arguments");
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("replication", "The replication number of the experiment",replication);
  cmd.AddValue ("num-ues", "Number of UEs", numberOfUEs);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  //backpressure params
  cmd.AddValue ("localstrategy", "the routing policy", localStrategy);
  cmd.AddValue ("algV", "V algorithm. 0=FIXED, 1=VAR_SINGLE_RADIO, 2=VAR_MULTI_RADIO", algV);
  cmd.AddValue ("theV", "the V parameter",theV);
  //traffic generation
  cmd.AddValue ("app", "App management: [[from]|[to]|[start]|[end]|[type]]:...", app);
  cmd.AddValue ("tcp", "Tcp congestion control", tcpCong);
  cmd.AddValue ("out", "Output dir", dirResultsFile);
  cmd.AddValue ("ftpBytes", "Bytes to transmit in FTP", ftpBytes);
  cmd.AddValue ("routing", "Routing protocol (bp, olsr)", routingProtocol);
  cmd.AddValue ("netanim", "Enable or disable netanim", netAnim);
  cmd.AddValue ("bpStat", "Enable or disable bp statistics", bpStat);
  cmd.AddValue ("initSsTh", "Initial slow start threshold", initialSsTh);
  cmd.AddValue ("scenario", "Scenario", scenario);
  cmd.AddValue ("meshQ", "Queue value in the mesh (packets)", meshQ);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  cmd.Parse (argc, argv);

  std::string adj_mat_file_name, terr_epc_file_name, terr_sat_file_name,
      enb_coords_file_name, ue_coords_file_name;

  if (scenario == 0)
    {
      // connectivity matrix file
      adj_mat_file_name = "scratch/lena-mesh-hybrid-nat/adjacency_matrix_3x3a.txt";
      // connectivity terrestrial node to EPC
      terr_epc_file_name = "scratch/lena-mesh-hybrid-nat/terr_epc_3x3.txt";
      // connectivity terrestrial node to Sat
      terr_sat_file_name = "scratch/lena-mesh-hybrid-nat/terr_sat_3x3.txt";
      // ENB coordinates
      enb_coords_file_name = "scratch/lena-mesh-hybrid-nat/enb_coords_3x3a.txt";
      // UE coordinates
      ue_coords_file_name = "scratch/lena-mesh-hybrid-nat/ue_coords_3x3a.txt";
    }
  else if (scenario == 1 || scenario == 3)
    {
      // connectivity matrix file
      adj_mat_file_name = "scratch/lena-mesh-hybrid-nat/adjacency_matrix_5x5.txt";
      // connectivity terrestrial node to EPC
      terr_epc_file_name = "scratch/lena-mesh-hybrid-nat/terr_epc_5x5.txt";
      // connectivity terrestrial node to Sat
      terr_sat_file_name = "scratch/lena-mesh-hybrid-nat/terr_sat_5x5.txt";
      // ENB coordinates
      enb_coords_file_name = "scratch/lena-mesh-hybrid-nat/enb_coords_5x5.txt";
      // UE coordinates
      ue_coords_file_name = "scratch/lena-mesh-hybrid-nat/ue_coords_5x5.txt";
    }
  else if (scenario == 2)
    {
      // connectivity matrix file
      adj_mat_file_name = "scratch/lena-mesh-hybrid-nat/adjacency_matrix_10x10.txt";
      // connectivity terrestrial node to EPC
      terr_epc_file_name = "scratch/lena-mesh-hybrid-nat/terr_epc_10x10.txt";
      // connectivity terrestrial node to Sat
      terr_sat_file_name = "scratch/lena-mesh-hybrid-nat/terr_sat_10x10.txt";
      // ENB coordinates
      enb_coords_file_name = "scratch/lena-mesh-hybrid-nat/enb_coords_10x10.txt";
      // UE coordinates
      std::stringstream uecoordsfilename;
      uecoordsfilename << "scratch/lena-mesh-hybrid-nat/ue_coords_10x10_" <<
                          numberOfUEs << ".txt";
      ue_coords_file_name = uecoordsfilename.str ();
    }
  else
    {
      NS_FATAL_ERROR ("Scenario " << scenario << " not known");
    }

  //we have to parse the file with the position of the nodes
  std::vector< std::vector<double> > enb_coords = readCoordinates (enb_coords_file_name);
  uint16_t numberOfENBs = enb_coords.size ();

  std::vector< std::vector<double> > ue_coords = readCoordinates (ue_coords_file_name, numberOfUEs);
  NS_ASSERT (numberOfUEs == ue_coords.size ());

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (tcpCong));
  Config::SetDefault ("ns3::TcpSocket::InitialSlowStartThreshold", UintegerValue (initialSsTh));
  Config::SetDefault ("ns3::TcpSocketBase::SACK", BooleanValue (true));

  //initialize connectivity matrix of terrestrial enbs 
  std::vector<std::vector<int> > terrestrialConnectivityMat = readNxNMatrix (adj_mat_file_name);
  //initialize vector of terrestrial links connecting directly to the EPC
  std::vector<bool> terrestrialEpc = readVector (terr_epc_file_name);
  //initialize vector of satellite links
  std::vector<bool> terrestrialSat = readVector (terr_sat_file_name);

  //check that we are not missing coordinates or elements in the terrestrial epc or sat file
  NS_ASSERT (terrestrialEpc.size () == numberOfENBs);
  NS_ASSERT (terrestrialSat.size () == numberOfENBs);
  NS_ASSERT (terrestrialConnectivityMat.size () == numberOfENBs);

  uint32_t SatGW = std::count (terrestrialSat.begin (), terrestrialSat.end (), 1);

  // Create the EPC node
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<MeshEpcHelper>  epcHelper = CreateObject<MeshEpcHelper> ();
  if (scenario == 0)
    {
      epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate("5Mbps")));
    }
  else if (scenario == 1 || scenario == 2 || scenario == 3)
    {
      epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate("500Mbps")));
    }

  epcHelper->SetAttribute("S1uLinkDataRateSatellite", DataRateValue (DataRate ("10Mbps")));
  epcHelper->SetAttribute("S1uLinkDelaySatellite", TimeValue (MilliSeconds (350)));

  lteHelper->SetEpcHelper (epcHelper);

  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (1575));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (19575));
  lteHelper->SetUeAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (1575));
  lteHelper->SetPathlossModelType ("ns3::OkumuraHataPropagationLossModel");
  lteHelper->SetPathlossModelAttribute ("Environment", EnumValue (UrbanEnvironment));
  lteHelper->SetPathlossModelAttribute ("CitySize", EnumValue (MediumCity));
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  //Adding a position to the epc_node, required for the backpressure routing protocol to work
  SetNodeProperties (pgw, distance*5, distance * 2, 2, "pgw");

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  //add a position to the Remote Host node
  SetNodeProperties (remoteHost, distance*7, distance*2, 2, "remote");

  // Create the Internet
  PointToPointHelper p2ph;
  DataRate internetRate = DataRate ("10Gbps");
  Time internetDelay = MilliSeconds (5);
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  Ptr<RateErrorModel> errorModel = CreateObject <RateErrorModel> ();
  errorModel->SetRandomVariable (uv);
  errorModel->SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  errorModel->SetRate (0.0);

  if (scenario == 3)
    {
      internetRate = DataRate ("10Mbps");
      internetDelay = MilliSeconds (350);
      errorModel->SetRate (0.0001);
      Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    }

  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (internetRate));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (internetDelay));
  p2ph.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (errorModel));

  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ptr<PointToPointNetDevice> pgwEth = DynamicCast<PointToPointNetDevice> (internetDevices.Get(0));
  Ptr<PointToPointNetDevice> remEth = DynamicCast<PointToPointNetDevice> (internetDevices.Get(1));

  pgwEth->GetQueue()->SetAttribute("MaxBytes", UintegerValue ((internetRate.GetBitRate() / 8)*
                                                              2*internetDelay.GetSeconds ()));
  remEth->GetQueue()->SetAttribute("MaxBytes", UintegerValue ((internetRate.GetBitRate() / 8)*
                                                              2*internetDelay.GetSeconds ()));

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->SetDefaultRoute (internetIpIfaces.GetAddress (0), 1);

  // Create the ENBs and set position
  NodeContainer enbNodes;
  enbNodes.Create (numberOfENBs);
  for (uint32_t i = 0; i < numberOfENBs; ++i)
    {
      std::stringstream n;
      n << "ENB" << i+1;
      SetNodeProperties (enbNodes.Get (i), enb_coords[i][0]*distance,
                         enb_coords[i][1]*distance, 2, n.str ());
    }

  // Create the UEs and set position
  NodeContainer ueNodes;
  ueNodes.Create (numberOfUEs);
  for (uint32_t i = 0; i < numberOfUEs; ++i)
    {
      std::stringstream n;
      n << "UE" << i+1;
      SetNodeProperties (ueNodes.Get (i), ue_coords[i][0]*distance,
                         ue_coords[i][1]*distance, 2, n.str ());
    }

  if (SatGW > 0)
    {
      std::stringstream n;
      n << "ENB_SAT";
      Ptr<Node> node = CreateObject<Node> ();
      SetNodeProperties(node, 3000.0, 3000.0, 3000.0, n.str());
      enbNodes.Add (node);
    }

  PrintNodeInfo (pgw);
  PrintNodeInfo (remoteHost);
  for (NodeContainer::Iterator it = enbNodes.Begin (); it != enbNodes.End (); ++it)
    {
      PrintNodeInfo (*it);
    }

  for (NodeContainer::Iterator it = ueNodes.Begin (); it != ueNodes.End (); ++it)
    {
      PrintNodeInfo (*it);
    }

  // Configure Mesh Backhaul Backpressure Routing
  BackpressureRoutingHelper backpressure;
  ConfigureBp(backpressure, theV, localStrategy, algV);

  Ipv4ListRoutingHelper list;
  OlsrHelper olsr;
  if (routingProtocol.compare ("bp") == 0)
    {
      list.Add (backpressure, 10);
    }
  else if (routingProtocol.compare ("olsr") == 0)
    {
      list.Add (olsr, 10);
    }
  else
    {
      NS_FATAL_ERROR ("Routing prot " << routingProtocol << " not recognized");
    }

  //Install the routing protocol in the epc node
  Ipv4StaticRoutingHelper staticRouting;
  list.Add (staticRouting, 5);
  Ptr <Ipv4> pgw_ipv4 = pgw->GetObject<Ipv4>();
  Ptr<Ipv4RoutingProtocol> ipv4Routing = list.Create (pgw);
  pgw_ipv4->SetRoutingProtocol (ipv4Routing);

  // Interconnect enbs amongst them
  epcHelper->AddHybridMeshBackhaul (enbNodes, terrestrialConnectivityMat,
                                    terrestrialEpc, terrestrialSat, list, meshQ);

  // Install LTE Devices to the nodes which have traffic actually
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  NetDeviceContainer::Iterator itNetDev;
  for (itNetDev = enbLteDevs.Begin (); itNetDev != enbLteDevs.End (); ++itNetDev)
    {
      Ptr<LteEnbNetDevice> dev = DynamicCast<LteEnbNetDevice> (*itNetDev);
      NS_ASSERT (dev != 0);

      dev->GetPhy ()->SetTxPower (23.0);
      dev->GetRrc ()->SetSrsPeriodicity (80);
    }

  for (itNetDev = ueLteDevs.Begin (); itNetDev != ueLteDevs.End (); ++itNetDev)
    {
      Ptr<LteUeNetDevice> dev = DynamicCast<LteUeNetDevice> (*itNetDev);

      dev->GetPhy ()->SetTxPower (23.0);
    }


  if (rem)
    {
      static Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
      PrintGnuplottableEnbListToFile ("out/enbs.txt");
      PrintGnuplottableUeListToFile ("out/ues.txt");
      remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/15"));
      remHelper->SetAttribute ("OutputFile", StringValue ("out/rem.txt"));
      remHelper->SetAttribute ("XMin", DoubleValue (-100.0));
      remHelper->SetAttribute ("XMax", DoubleValue (300.0));
      remHelper->SetAttribute ("YMin", DoubleValue (-100.0));
      remHelper->SetAttribute ("YMax", DoubleValue (300.0));
      remHelper->SetAttribute ("Earfcn", UintegerValue (1575));
      remHelper->SetAttribute ("Z", DoubleValue (2));
      remHelper->Install ();

      Simulator::Stop (Seconds (simTime));
      Simulator::Run ();
      Simulator::Destroy ();

      return 0;
    }


  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  // Assign default route to UEs
  for (NodeContainer::Iterator it = ueNodes.Begin (); it != ueNodes.End (); ++it)
    {
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
      NS_ASSERT (*it != 0);
    }

  // Attach UE to the closest eNodeB
  lteHelper->AttachToClosestEnb (ueLteDevs, enbLteDevs);

  // Fulfill class Location List which contains the mapping of the ids (i.e.,ip addresses) to geographic position
  PopulateLocationMapThroughPos (enbNodes);

  // Initialize GWs
  PopulateLocationMapThroughHops (enbNodes, terrestrialEpc, terrestrialSat, distance);

  // Populate Arp Cache Tables of all enodes + epc + remote host
  NodeContainer arp_cache;
  arp_cache.Add (enbNodes);
  arp_cache.Add (pgw);
  arp_cache.Add (remoteHost);
  PopulateArpCache (arp_cache);

  // Comment to enable PCAP tracing
  //p2ph.EnablePcap (dirResultsFile + "/lte-mesh", internetDevices);
  //p2ph.EnablePcapAll (dirResultsFile + "/lte-mesh");

  // Applications!

  uint16_t terr_port = 2001;
  uint16_t sat_port = 2000;

  std::vector<AppProperties*> appDefinitions;
  ParseApp (app, appDefinitions);

  UintDataSet goodputSet;
  UintDataSet cWndSet;
  UintDataSet rttSet;

  StringDataSet idToName;
  goodputSet.insert (goodputSet.end(), UintDataPair (0, new Gnuplot2dDataset ()));

  for (std::vector<AppProperties*>::iterator it = appDefinitions.begin();
       it != appDefinitions.end(); ++it)
    {
      AppProperties *p = (*it);
      //Ipv4Address srcAddr = p->from->GetObject<Ipv4>()->GetAddress (1,0).GetLocal ();
      Ipv4Address dstAddr = p->to->GetObject<Ipv4>()->GetAddress (1,0).GetLocal ();
      uint32_t id; // represents the current port of the application. Remember
                   // to increase it by 2 for each application (each application
                   // will have different ports)

      Ptr<Socket> s;
      NodeContainer to (p->to);

      if (p->mode == FTP || p->mode == FTP_SAT)
        {
          NS_ABORT_MSG_UNLESS (p->rate.empty(), "Rate should not be specified with TCP");
          BulkSendHelper ftp (p->socketType, Address ());

          ftp.SetAttribute ("SendSize", UintegerValue (1000));
          ftp.SetAttribute ("MaxBytes", UintegerValue (ftpBytes));


          if (p->mode == FTP)
            {
              ftp.SetAttribute ("Remote",
                                AddressValue (InetSocketAddress (dstAddr, terr_port)));
              ftp.SetAttribute ("Local",
                                AddressValue (InetSocketAddress (Ipv4Address::GetAny (), terr_port+10000)));
              InstallSinks (to, terr_port, "ns3::TcpSocketFactory", simTime, &goodputSet);
              id = terr_port + 10000;
              terr_port+=2;
            }
          else if (p->mode == FTP_SAT)
            {
              ftp.SetAttribute ("Remote", AddressValue (InetSocketAddress (dstAddr, sat_port)));
              ftp.SetAttribute ("Local",
                                AddressValue (InetSocketAddress (Ipv4Address::GetAny (), sat_port+10000)));
              InstallSinks (to, sat_port, "ns3::TcpSocketFactory", simTime, &goodputSet);
              id = sat_port + 10000;
              sat_port+=2;
            }

          idToName.insert(StringDataPair (id, Names::FindName(p->from)+"-"+Names::FindName(p->to)));

          Gnuplot2dDataset *rttData = new Gnuplot2dDataset ();
          Gnuplot2dDataset *cwndData = new Gnuplot2dDataset ();
          Gnuplot2dDataset *goodData = new Gnuplot2dDataset ();

          rttSet.insert (rttSet.end (), UintDataPair (id, rttData));
          cWndSet.insert (cWndSet.end (), UintDataPair (id, cwndData));
          goodputSet.insert (goodputSet.end(), UintDataPair (id, goodData));

          ApplicationContainer sourceApp = ftp.Install (p->from);
          sourceApp.Start (p->start);
          sourceApp.Stop (p->end);

          Ptr<BulkSendApplication> app = DynamicCast<BulkSendApplication> (sourceApp.Get (0));
          s = CreateConnectedSocket (p->from, p->socketType, rttData, cwndData);
          app->SetSocket (s);
        }
      else if (p->mode == CBR)
        {
          NS_ABORT_IF (p->rate.empty());
          OnOffHelper onOffHelper (p->socketType, Address ());
          id = terr_port+10000;

          idToName.insert(StringDataPair (id, Names::FindName(p->from)+"-"+Names::FindName(p->to)));

          onOffHelper.SetConstantRate(DataRate (p->rate), 1000);

          onOffHelper.SetAttribute ("Remote",
                                    AddressValue (InetSocketAddress (dstAddr, terr_port)));
          onOffHelper.SetAttribute ("Local",
                                    AddressValue (InetSocketAddress (Ipv4Address::GetAny (), id)));
          onOffHelper.SetAttribute ("Protocol",
                                    TypeIdValue (TypeId::LookupByName (p->socketType.c_str ())));

          ApplicationContainer sourceApp = onOffHelper.Install (p->from);
          sourceApp.Start (p->start);
          sourceApp.Stop (p->end);

          Ptr<OnOffApplication> app = DynamicCast<OnOffApplication> (sourceApp.Get (0));

          Gnuplot2dDataset *goodData = new Gnuplot2dDataset ();

          goodputSet.insert (goodputSet.end(), UintDataPair (id, goodData));
          s = CreateConnectedSocket (p->from, p->socketType);

          // These goodput stat will not be inserted in the aggregate, neither
          // will stop simulation in any case
          InstallSinks (to, terr_port, p->socketType, simTime, &goodputSet, false);

          app->SetSocket (s);
          terr_port += 2;
        }
      else
        {
          NS_FATAL_ERROR ("Unsupported APP!");
        }

      delete p;
    }

  appDefinitions.erase (appDefinitions.begin(), appDefinitions.end());

  //Packet::EnablePrinting();

  UintDataSet vData;
  std::map<std::string, UintDataSet*> queueData;

  if (bpStat && routingProtocol.compare("bp") == 0)
    {
      BackpressureTracingHelper bpStats;
      NodeContainer nodesToMonitor (pgw, enbNodes);
      NetDeviceContainer NetDevicesToMonitor;

      for (uint16_t i=0; i<nodesToMonitor.GetN() ; i++)
        {
          Ptr<Node> node = nodesToMonitor.Get(i);

          for (uint16_t j=1; j< node->GetObject<Ipv4>()->GetNInterfaces(); j++)
            {
              NetDevicesToMonitor.Add(node->GetObject<Ipv4>()->GetNetDevice(j));
            }

          std::string name = Names::FindName(node);
          Gnuplot2dDataset *vSet = new Gnuplot2dDataset ();

          vData.insert(UintDataPair (node->GetId (), vSet));

          UintDataSet *queueSet = new UintDataSet ();
          queueData.insert(std::pair<std::string, UintDataSet*>(name, queueSet));

          Ptr<backpressure::RoutingProtocol> r = node->GetObject<backpressure::RoutingProtocol> ();
          NS_ASSERT (r != 0);

          r->TraceConnectWithoutContext ("Vvariable",
                                         MakeBoundCallback (VTrace, vSet));
          r->TraceConnectWithoutContext ("QLength",
                                         MakeBoundCallback (QueueTrace, queueSet));
        }

      bpStats.EnableBackpressure(dirResultsFile + "/" + "backpressure",
                                 NetDevicesToMonitor);
    }

  LocationList::SetResetLocationList (false);

  if (netAnim)
    {
      // Create the animation object and configure for specified output
      AnimationInterface anim ("sansa-animation.xml", numberOfENBs, 0);

      anim.EnableIpv4RouteTracking ("routing.xml", Seconds (0.0),
                                    Seconds (2.0), Seconds (1));
    }

  PtrForPrinting p;
  p.idToName = &idToName;
  p.prefix = dirResultsFile + "/report-";
  p.shouldAggregate = true;

  p.postfix = "-goodput.data";
  Simulator::Schedule (Seconds (1.0), &PeriodicFreeUintDataStats, goodputSet, p);

  p.postfix = "-cwnd.data";
  p.shouldAggregate = false;
  Simulator::Schedule (Seconds (1.0), &PeriodicFreeUintDataStats, cWndSet, p);

  p.postfix = "-rtt.data";
  Simulator::Schedule (Seconds (1.0), &PeriodicFreeUintDataStats, rttSet, p);

  activeSocket = numberOfUEs;
  totalBytes = ftpBytes * numberOfUEs;

  Simulator::Schedule (Seconds (1.0), &PrintTime, p.prefix+"curr-time.txt");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();

  if (totalBytes != 0)
    {
      std::ofstream dataFile;
      dataFile.open ((dirResultsFile + "/ERROR.txt").c_str (), std::fstream::out | std::fstream::app);
      dataFile << "Not Received all bytes. Missing: " << totalBytes << std::endl;
      dataFile << "Number of socket with errors: " << errorSocket << std::endl;
      dataFile.close ();
    }
  else
    {
      std::ofstream dataFile;
      dataFile.open ((p.prefix+"curr-time.txt").c_str(), std::fstream::out | std::fstream::app);
      dataFile << Simulator::Now().GetSeconds() << " finish. "
                  "Number of sockets with errors: " << errorSocket << std::endl;
    }

  p.postfix = "-goodput.data";
  p.shouldAggregate = true;
  PrintUintDataStats (goodputSet, p);
  FreeUintDataSet(goodputSet);

  p.postfix = "-cwnd.data";
  PrintUintDataStats (cWndSet, p);
  FreeUintDataSet(cWndSet);

  p.postfix = "-rtt.data";
  PrintUintDataStats (rttSet, p);
  FreeUintDataSet (rttSet);

  if (bpStat && routingProtocol.compare ("bp") == 0)
    {
      for (UintDataSet::iterator it = vData.begin(); it != vData.end(); ++it)
        {
          Ptr<Node> node = 0;
          NodeContainer glob (ueNodes, enbNodes, remoteHostContainer, pgw);
          for (NodeContainer::Iterator j = glob.Begin(); j != glob.End(); ++j)
            {
              node = *j;
              if (node->GetId() == it->first)
                {
                  break;
                }
              else
                {
                  node = 0;
                }
            }

          NS_ASSERT (node != 0);
          NS_ASSERT (node->GetId() == it->first);

          Gnuplot2dDataset *dataset = it->second;
          std::stringstream fileName;

          fileName << dirResultsFile << "/lte-multiradio-" << Names::FindName(node) << "-v.data";

          OutputGnuplot (dataset, fileName.str ());
        }
      FreeUintDataSet (vData);

      for (std::map<std::string, UintDataSet*>::iterator it = queueData.begin(); it != queueData.end(); ++it)
        {
          std::string name = it->first;
          UintDataSet *queueNode = it->second;

          for (UintDataSet::iterator i=queueNode->begin(); i!=queueNode->end(); ++i)
            {
              std::stringstream fileName;
              fileName << dirResultsFile << "/lte-multiradio-" << name << "-queue-" << i->first << ".data";
              OutputGnuplot (i->second, fileName.str ());
            }
          FreeUintDataSet (*queueNode);
          queueNode = 0;
        }
    }

  return 0;
}
