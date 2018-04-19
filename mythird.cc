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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

// create custom application to replicate WiFi module firmware
class MyApp : public Application
{
public:
    MyApp (Ptr<Node> node, int id);
    virtual ~MyApp();

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleAssociation(void);
    void StartAssociation (void);

    Ptr<Node> m_node;
    int m_id;
    EventId m_sendEvent;
    bool m_running;
};

MyApp::MyApp (Ptr<Node> node, int id)
  : m_node(node),
    m_id(id),
    m_sendEvent (),
    m_running (false)
{

}

MyApp::~MyApp()
{
}

void
MyApp::StartApplication (void)
{
    m_running = true;
    ScheduleAssociation ();
}

void
MyApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
    {
        Simulator::Cancel (m_sendEvent);
    }
}

void
MyApp::StartAssociation (void)
{
    Ptr<WifiNetDevice> device = StaticCast<WifiNetDevice>(m_node->GetDevice(0));
    Ptr<StaWifiMac> mac = StaticCast<StaWifiMac>(device->GetMac());
//    mac->StartActiveAssociation ();
    mac->SetAttribute ("ActiveProbing",BooleanValue(true));
}


void
MyApp::ScheduleAssociation (void)
{
    if (m_running)
    {
        Time tNext (MilliSeconds(m_id*100));
        m_sendEvent = Simulator::Schedule (tNext, &MyApp::StartAssociation, this);
    }
}

int 
main (int argc, char *argv[])
{
  bool verbose = true;
//  uint32_t nCsma = 3;
  uint32_t nWifi = 3;
  bool tracing = true;

  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  if (nWifi > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
        LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
    }
  LogComponentEnable ("StaWifiMac", LOG_LEVEL_INFO);

  ns3::PacketMetadata::Enable();

  NodeContainer p2pNodes;
  p2pNodes.Create (1);

//  PointToPointHelper pointToPoint;
//  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
//  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

//  NetDeviceContainer p2pDevices;
//  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);


  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue(false));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;
  Ipv4InterfaceContainer wifiInterfaces;
  Ipv4InterfaceContainer apInterface;


  address.SetBase ("10.1.3.0", "255.255.255.0");
  wifiInterfaces = address.Assign (staDevices);
  apInterface = address.Assign (apDevices);

//  uint16_t port = 9999;
//  UdpServerHelper server(port);

//  ApplicationContainer serverApps = server.Install (wifiApNode);
//  serverApps.Start (Seconds (1));
//  serverApps.Stop (Seconds (10));

    Ptr<MyApp> app1 = CreateObject<MyApp> (wifiStaNodes.Get (0),1);
    wifiStaNodes.Get (0)->AddApplication (app1);
    app1->SetStartTime (Seconds (1));
    app1->SetStopTime (Seconds (20));

    Ptr<MyApp> app2 = CreateObject<MyApp> (wifiStaNodes.Get (1),2);
    wifiStaNodes.Get (1)->AddApplication (app2);
    app2->SetStartTime (Seconds (1));
    app2->SetStopTime (Seconds (20));

//  UdpClientHelper client1 (apInterface.GetAddress(0), port);
//  client1.SetAttribute ("MaxPackets", UintegerValue (1000));
//  client1.SetAttribute ("Interval", TimeValue (MilliSeconds (50)));
//  client1.SetAttribute ("PacketSize", UintegerValue (1024));

//  UdpClientHelper client2 (apInterface.GetAddress(0), port);
//  client2.SetAttribute ("MaxPackets", UintegerValue (1000));
//  client2.SetAttribute ("Interval", TimeValue (MilliSeconds (50)));
//  client2.SetAttribute ("PacketSize", UintegerValue (1024));

//  ApplicationContainer client1App = client1.Install (wifiStaNodes.Get(0));
//  client1App.Start (MilliSeconds (1000));
//  client1App.Stop (Seconds (10));

//  ApplicationContainer client2App = client2.Install (wifiStaNodes.Get(1));
//  client2App.Start (MilliSeconds (1050));
//  client2App.Stop (Seconds (10));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
//      pointToPoint.EnablePcapAll ("third");
//      phy.EnablePcap ("third", apDevices.Get (0));
//      csma.EnablePcap ("third", csmaDevices.Get (0), true);
    }

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
