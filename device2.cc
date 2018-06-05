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

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1 --> our "server"
//                   point-to-point

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

// create custom application to replicate WiFi module firmware
class MyApp : public Application
{
public:
    MyApp (Ptr<Node> node, int id);
    virtual ~MyApp();
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleAssociation(void);
    void StartAssociation (void);
    void ScheduleTx(void);
    void SendPacketTimed (void);

    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    uint32_t        m_packetsSent;
    Ptr<Node> m_node;
    int m_id;
    EventId m_sendEvent;
    bool m_running;
};

MyApp::MyApp (Ptr<Node> node, int id)
  :
    m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_packetsSent (0),
    m_node(node),
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

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    { //change time value
      //Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      Time tNext (MilliSeconds (50));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacketTimed, this);
    }
}


void
MyApp::SendPacketTimed (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);
  //change this if statement: create packetcnt var check if  >0 and keep schedule
  if (m_nPackets>0 && m_packetsSent++<1)
    {
      ScheduleTx ();
    }
  m_packetsSent=0;
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

    }
  //LogComponentEnable ("StaWifiMac", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  ns3::PacketMetadata::Enable();

  // Create p2p nodes: AP
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  // WiFi nodes
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

  // Install STA devices
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);


  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue(false));

  // Install AP device
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  // Mobility models
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

  // Install Internet Stack
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);
  stack.Install(p2pNodes);
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer wifiInterfaces;
  Ipv4InterfaceContainer apInterface;


  address.SetBase ("10.1.3.0", "255.255.255.0");
  wifiInterfaces = address.Assign (staDevices);
  apInterface = address.Assign (apDevices);

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

//  uint16_t port = 9999;
//  UdpServerHelper server(port);

//  ApplicationContainer serverApps = server.Install (wifiApNode);
//  serverApps.Start (Seconds (1));
//  serverApps.Stop (Seconds (10));

    // Create socket and source destination
    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (apInterface.GetAddress (1), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (p2pNodes.Get (1));
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (20.));

    //Install socket on all transmitter devices
    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (wifiStaNodes.Get (0), UdpSocketFactory::GetTypeId ());
    Ptr<Socket> ns3UdpSocket2 = Socket::CreateSocket (wifiStaNodes.Get (1), UdpSocketFactory::GetTypeId ());


    // Apps install with setup
    Ptr<MyApp> app1 = CreateObject<MyApp> (wifiStaNodes.Get (0),1);
    app1->Setup(ns3UdpSocket, sinkAddress, 200, 1, DataRate ("1Mbps"));
    wifiStaNodes.Get (0)->AddApplication (app1);
    app1->SetStartTime (Seconds (1));
    app1->SetStopTime (Seconds (20));

    Ptr<MyApp> app2 = CreateObject<MyApp> (wifiStaNodes.Get (1),2);
    app1->Setup(ns3UdpSocket2, sinkAddress, 200, 1, DataRate ("1Mbps"));
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
