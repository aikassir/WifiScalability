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
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"


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
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint32_t nWifi);
    void SetSlotTime(double tslot);

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleAssociation(void);
    void StartAssociation (void);
    void SendPacketTimed (void);
    void ScheduleTx(void);
    void SetRate(void);

    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    uint32_t        m_packetsSent;
    Ptr<Node> m_node;
    uint32_t m_id;
    double m_tslot;
    uint32_t m_nWifi;
    EventId m_sendEvent;
    bool m_running;
};

MyApp::MyApp (Ptr<Node> node, int id)
  : m_peer (),
    m_packetSize (0),
    m_nPackets (1),
    m_dataRate (0),
    m_packetsSent (0),
    m_node(node),
    m_id(id),
    m_tslot(0),
    m_nWifi(0),
    m_sendEvent (),
    m_running (false)

{
  //m_socket=Socket::CreateSocket (node, UdpSocketFactory::GetTypeId ());

}

MyApp::~MyApp()
{
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint32_t nWifi)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_nWifi= nWifi;
}


void
MyApp::StartApplication (void)
{
    m_running = true;
    m_packetsSent = 0;
    ScheduleAssociation ();
    Time tstart = MilliSeconds(m_id*m_tslot);
    Simulator::Schedule(tstart,&MyApp::SendPacketTimed,this);

}

void
MyApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
    {
        Simulator::Cancel (m_sendEvent);
    }

    if (m_socket)
      {
        m_socket->Close ();
      }
}

void
MyApp::SetSlotTime (double tslot)
{
  m_tslot = tslot;
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
        Time tNext (MilliSeconds(2));
        m_sendEvent = Simulator::Schedule (tNext, &MyApp::StartAssociation, this);
    }
//    NS_LOG_UNCOND("Association done");
}

void
MyApp::ScheduleTx (void)
{
    if (m_running)
    {
        Time tNext (MilliSeconds(m_nWifi*m_tslot));
        m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacketTimed,this);
    }
}

void
MyApp::SendPacketTimed (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);  
  m_socket->Bind ();
  m_socket->SendTo(packet,0,m_peer);

  if (++m_packetsSent<m_nPackets)
    {
      ScheduleTx ();

    }
}

//void SetRate (void)
//{
//  Config::Set("NodeList/*/DeviceList/*$ns3::WifiNetDevice/DataRate",StringValue(DataRate("3Mbps")));
//}
 //static void RxPacket(Ptr<const Packet> p){
 //  NS_LOG_UNCOND("Received packet at: "<< Simulator::Now().GetMilliSeconds());

 //}

//static void TxDrop(Ptr<const Packet> p){
//  NS_LOG_UNCOND("Packet queued for TX at: "<<Simulator::Now().GetSeconds());
//}


//static void ApRx(Ptr<const Packet> p){
//  NS_LOG_UNCOND("Drop at AP at: "<<Simulator::Now().GetSeconds());
//}


int 
main (int argc, char *argv[])
{
  bool verbose = true;

  uint32_t nWifi = 501;
  //bool tracing = true;

//  CommandLine cmd;
//  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
//  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
//  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

//  cmd.Parse (argc,argv);

  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
//  if (nWifi > 250)
//    {
//      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
//      return 1;
//    }

  if (verbose)
    {

    }
  //LogComponentEnable ("StaWifiMac", LOG_LEVEL_INFO);
  //LogComponentEnable ("ApWifiMac", LOG_LEVEL_INFO);
  //LogComponentEnable ("Socket", LOG_LEVEL_INFO);
   LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  ns3::PacketMetadata::Enable();

//  // Create p2p nodes: AP
//  NodeContainer p2pNodes;
//  p2pNodes.Create (2);

//  PointToPointHelper pointToPoint;
//  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
//  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

//  NetDeviceContainer p2pDevices;
//  p2pDevices = pointToPoint.Install (p2pNodes);

  // WiFi nodes
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi-1);
  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  /* OLD MAC AND PHY */

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false),"MaxMissedBeacons",UintegerValue(1000));

  // Install STA devices
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);


  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue(false),"BeaconInterval",TimeValue(Days(1)));

  // Install AP device
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);


  // Mobility models
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (0.5),
                                 "DeltaY", DoubleValue (1.0),
                                 "GridWidth", UintegerValue (20),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0,1000, 0, 1000)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);


  // Install Internet Stack
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);
//  stack.Install(p2pNodes.Get(1));
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer wifiInterfaces;
  Ipv4InterfaceContainer apInterface;


  address.SetBase ("10.1.0.0", "255.255.248.0");
  wifiInterfaces = address.Assign (staDevices);
  apInterface = address.Assign (apDevices);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
//  address.SetBase ("10.1.1.0", "255.255.255.0");
//  Ipv4InterfaceContainer p2pInterfaces;
//  p2pInterfaces = address.Assign (p2pDevices);

    //Tracing callbacks
//    Ptr<PointToPointNetDevice> server_dev = StaticCast<PointToPointNetDevice>(p2pDevices.Get (1));
//    server_dev->TraceConnectWithoutContext("MacRx",MakeCallback(&RxPacket));
//    Ptr<WifiNetDevice> stawifidev = StaticCast<WifiNetDevice>(staDevices.Get (0));
//    Ptr<WifiMac> nodemac1 = stawifidev->GetMac();
//    nodemac1->TraceConnectWithoutContext("MacTx",MakeCallback(&TxDrop));
//    Ptr<WifiNetDevice> apwifidev = StaticCast<WifiNetDevice>(apDevices.Get (0));
//    Ptr<WifiMac> apmac = apwifidev->GetMac();
//    apmac->TraceConnectWithoutContext("MacPromiscRx",MakeCallback(&ApRx));


  /**** OLD APPLICATION ****/

    double tslot = 10; //

    // Create packet sink on destination
    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (apInterface.GetAddress (0), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (apInterface.GetAddress (0), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (wifiApNode.Get (0));
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (60.));

    //Create socket to be installed in app on setup on all transmitter devices
//    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (wifiStaNodes.Get (0), UdpSocketFactory::GetTypeId ());

    for (uint32_t i=0; i<nWifi-1; i++)
      {
        Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (wifiStaNodes.Get (i), UdpSocketFactory::GetTypeId ());
        Ptr<MyApp> app1 = CreateObject<MyApp> (wifiStaNodes.Get (i),i+1);
        app1->Setup(ns3UdpSocket, sinkAddress, 200, 2, DataRate ("1Mbps"), nWifi-1);
        wifiStaNodes.Get (i)->AddApplication (app1);
        app1->SetSlotTime(tslot);
        app1->SetStartTime (MilliSeconds (1000+i));
        app1->SetStopTime (Seconds (60));
      }

//    for (uint32_t i=0; i<nWifi-1; i++)
//      {
//        Ptr<MyApp> app = wifiStaNodes.Get(i)->GetApplication(0);
//        app->SetRate();
//      }

//  if (Simulator::Schedule(MilliSeconds(3*nWifi*tslot), &SetRate))
//    {
//      NS_LOG_UNCOND("DataRate = 3Mbps");
//    }

  Simulator::Stop (Seconds (61.0));

//    if (tracing == true)
//      {
//        //      pointToPoint.EnablePcapAll ("third");
//        //      phy.EnablePcap ("third", apDevices.Get (0));
//        //      csma.EnablePcap ("third", csmaDevices.Get (0), true);
//      }

    Simulator::Run ();
    Simulator::Destroy ();

    uint64_t totalPacketsThrough = DynamicCast<PacketSink> (sinkApps.Get(0))->GetTotalRx ();
    std::cout<< std::endl<<totalPacketsThrough<<std::endl;
    return 0;
}
