/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 * ns-3 upstreaming process: Natale Patriciello <natale.patriciello@gmail.com>
 */
#include "yans-wifi-helper-ad.h"
#include "multi-band-net-device.h"
#include "ns3/log.h"

namespace ns3 {

YansWifiPhyHelperAd::YansWifiPhyHelperAd()
  : YansWifiPhyHelper (),
    m_enableAntenna (false),
    m_directionalAntenna (false)
{

}

void YansWifiPhyHelperAd::SetAntenna (std::string name,
				    std::string n0, const AttributeValue &v0,
				    std::string n1, const AttributeValue &v1,
				    std::string n2, const AttributeValue &v2,
				    std::string n3, const AttributeValue &v3,
				    std::string n4, const AttributeValue &v4,
				    std::string n5, const AttributeValue &v5,
				    std::string n6, const AttributeValue &v6,
				    std::string n7, const AttributeValue &v7)
{
  m_antenna = ObjectFactory ();
  m_antenna.SetTypeId (name);
  m_antenna.Set (n0, v0);
  m_antenna.Set (n1, v1);
  m_antenna.Set (n2, v2);
  m_antenna.Set (n3, v3);
  m_antenna.Set (n4, v4);
  m_antenna.Set (n5, v5);
  m_antenna.Set (n6, v6);
  m_antenna.Set (n7, v7);
}

void
YansWifiPhyHelperAd::EnableAntenna (bool value, bool directional)
{
  m_enableAntenna = value;
  m_directionalAntenna = directional;
}

void
YansWifiPhyHelperAd::EnableMultiBandPcap (std::string prefix, Ptr<NetDevice> nd, Ptr<WifiPhy> phy)
{
  //All of the Pcap enable functions vector through here including the ones
  //that are wandering through all of devices on perhaps all of the nodes in
  //the system. We can only deal with devices of type WifiNetDevice.
  Ptr<MultiBandNetDevice> device = nd->GetObject<MultiBandNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("YansWifiHelper::EnablePcapInternal(): Device " << &device << " not of type ns3::WifiNetDevice");
      return;
    }
  NS_ABORT_MSG_IF (phy == 0, "YansWifiPhyHelper::EnablePcapInternal(): Phy layer in MultiBandNetDevice must be set");

  PcapHelper pcapHelper;

  std::string filename;
  filename = pcapHelper.GetFilenameFromDevice (prefix, device);

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, m_pcapDlt);

  phy->TraceConnectWithoutContext ("MonitorSnifferTx", MakeBoundCallback (&YansWifiPhyHelper::PcapSniffTxEvent, file));
  phy->TraceConnectWithoutContext ("MonitorSnifferRx", MakeBoundCallback (&YansWifiPhyHelper::PcapSniffRxEvent, file));
}

Ptr<WifiPhy>
YansWifiPhyHelperAd::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<YansWifiPhy> phy = m_phy.Create<YansWifiPhy> ();
  if (m_enableAntenna)
    {
      if (m_directionalAntenna)
        {
          Ptr<DirectionalAntenna> antenna = m_antenna.Create<DirectionalAntenna> ();
          phy->SetDirectionalAntenna (antenna);
        }
      else
        {
          Ptr<AbstractAntenna> antenna = m_antenna.Create<AbstractAntenna> ();
          phy->SetAntenna (antenna);
        }
    }

  return YansWifiPhyHelper::CreateWithPhy(node, device, phy);
}

} // namespace ns3
