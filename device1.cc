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

#include <string>
#include <list>
#include <boost/lexical_cast.hpp>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("device1");

class apApp : public Application
{
public:
    apApp();
    ~apApp(){}

private:

    virtual void StartApplication (void);
    virtual void StopApplication (void){}

    Ptr<Socket> m_socket;

    Ptr<WifiNetDevice> m_device;
    Ptr<ApWifiMac> m_mac;
    std::set<int> ids;

    bool ConnectionRequested(Ptr<Socket> socket, const Address& address);
    void ConnectionAccepted(Ptr<Socket> socket, const Address& address);

    void RequestId(Ptr<Socket> socket);
};

apApp::apApp ()
{
}

void apApp::StartApplication ()
{
    m_device = StaticCast<WifiNetDevice>(m_node->GetDevice(0));
    m_mac = StaticCast<ApWifiMac>(m_device->GetMac());
    uint16_t apPort = 9998;
    Address apAddress (InetSocketAddress (Ipv4Address::GetAny (), apPort));
    m_socket=Socket::CreateSocket (m_node, UdpSocketFactory::GetTypeId ());
    m_socket->SetRecvCallback (MakeCallback(&apApp::RequestId, this));
    m_socket->Bind (apAddress);
}

void apApp::RequestId (Ptr<Socket> socket)
{
    Address addr;
    Ptr<Packet> receivedPacket;
    receivedPacket = socket->RecvFrom(addr);
//    Packet::EnablePrinting ();
//    receivedPacket->Print(std::cout);
//    std::string s;
//    s.resize (receivedPacket->GetSize());
//    receivedPacket->CopyData((uint8_t*)s.data (), receivedPacket->GetSize ());

//    for(unsigned int i=0; i<s.size (); i++)
//    {
//        std::cout << (unsigned int)s[i] << std::endl;
//    }
//    Ptr<Packet> copiedPacket = receivedPacket->Copy ();
//    Ipv4Header iph;
//    copiedPacket->RemoveHeader (iph);
//    Address addr(InetSocketAddress(iph.GetSource (), 9997));

    // get next id
    int id=1;
    while(ids.find(id)!=ids.end())
    {
        id++;
    }

    // add id to set
    ids.insert(id);

    // send id
    std::string sid=boost::lexical_cast<std::string>(id);
    Ptr<Packet> packet = Create<Packet> ((const uint8_t*)sid.c_str (),sid.size());
    socket->SendTo(packet,0,addr);
}

// create custom application to replicate WiFi module firmware
class staApp : public Application
{
public:
    staApp (Ptr<Node> node, int id);
    virtual ~staApp(){}

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleAssociation(void);
    void StartAssociation (void);
    void StopAssociation (void);

//    void Connect();
    void ScheduleRequestId();
    void RequestId();
    void UpdateId(Ptr<Socket> socket);

    Ptr<Node> m_node;
    Ptr<Socket> m_socket;
    int m_id;
    EventId m_sendEvent;
    bool m_running;

    Ptr<WifiNetDevice> m_device;
    Ptr<StaWifiMac> m_mac;
};

staApp::staApp (Ptr<Node> node, int id)
  : m_node(node),
    m_id(id),
    m_sendEvent (),
    m_running (false)
{
    m_device = StaticCast<WifiNetDevice>(m_node->GetDevice(0));
    m_mac = StaticCast<StaWifiMac>(m_device->GetMac());   
    m_mac->SetLinkUpCallback (MakeCallback(&staApp::ScheduleRequestId,this)); // setup callback for requesting id
    m_socket=Socket::CreateSocket (node, UdpSocketFactory::GetTypeId ());
    m_socket->SetRecvCallback(MakeCallback(&staApp::UpdateId,this));
    uint16_t staPort = 9997;
    Address staAddress (InetSocketAddress (Ipv4Address::GetAny (), staPort));
    m_socket->Bind(staAddress);
}

void
staApp::StartApplication (void)
{
    m_running = true;
    ScheduleAssociation ();
}

void
staApp::StopApplication (void)
{
    m_running = false;
    if (m_sendEvent.IsRunning ())
    {
        Simulator::Cancel (m_sendEvent);
    }
}

void staApp::StartAssociation (void)
{
    m_mac->SetAttribute ("ActiveProbing",BooleanValue(true));
}

void staApp::ScheduleAssociation (void)
{
    if (m_running)
    {
        Time tNext (MilliSeconds(m_id*100));
        m_sendEvent = Simulator::Schedule (tNext, &staApp::StartAssociation, this);
    }
}

//void staApp::Connect()
//{
//    // connect
//    uint16_t apPort = 9998;
//    Address apAddress (InetSocketAddress ("192.168.1.1", apPort));
//    m_socket->Connect (apAddress);
//}

void staApp::ScheduleRequestId()
{
    Time tNext (Seconds(2));
    Simulator::Schedule (tNext, &staApp::RequestId, this);
}

void staApp::RequestId ()
{
    Ptr<Packet> packet = Create<Packet> (64);
    uint16_t apPort = 9998;
    Address apAddress (InetSocketAddress ("192.168.1.1", apPort));
    m_socket->SendTo(packet,0,apAddress);
}

void staApp::UpdateId(Ptr<Socket> socket)
{
    Ptr<Packet> packet=socket->Recv ();
    std::ostringstream ostr;
    packet->CopyData(&ostr,packet->GetSize ());
    m_id=boost::lexical_cast<int>(ostr.str ());
}

int main (int argc, char *argv[])
{
    bool verbose = true;
    uint32_t nWifi = 2;
    bool tracing = true;

    CommandLine cmd;
    cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

    cmd.Parse (argc,argv);

    Packet::EnablePrinting ();

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
        LogComponentEnable ("StaWifiMac", LOG_LEVEL_INFO);
        LogComponentEnable ("device1", LOG_LEVEL_INFO);
    }

    ns3::PacketMetadata::Enable();

//  PointToPointHelper pointToPoint;
//  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
//  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

//  NetDeviceContainer p2pDevices;
//  p2pDevices = pointToPoint.Install (p2pNodes);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");
    mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);


    mac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid),"BeaconGeneration", BooleanValue(false),"BeaconInterval", TimeValue(Days(1)));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNode);

    // mobility configuration
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

    // for routing
    InternetStackHelper stack;
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
    Ipv4InterfaceContainer wifiInterfaces;
    Ipv4InterfaceContainer apInterface;


    address.SetBase ("192.168.1.0", "255.255.255.0");
    apInterface = address.Assign (apDevices);
    wifiInterfaces = address.Assign (staDevices);

    //  uint16_t port = 9999;
    //  UdpServerHelper server(port);

    //  ApplicationContainer serverApps = server.Install (wifiApNode);
    //  serverApps.Start (Seconds (1));
    //  serverApps.Stop (Seconds (10));

    Ptr<apApp> apApp1 = CreateObject<apApp>();
    wifiApNode.Get(0)->AddApplication(apApp1);
    apApp1->SetStartTime(Seconds(1));
    apApp1->SetStopTime(Seconds(20));

    Ptr<staApp> app1 = CreateObject<staApp> (wifiStaNodes.Get (0),1);
    wifiStaNodes.Get (0)->AddApplication (app1);
    app1->SetStartTime (Seconds (1));
    app1->SetStopTime (Seconds (20));

    Ptr<staApp> app2 = CreateObject<staApp> (wifiStaNodes.Get (1), 2);
    wifiStaNodes.Get (1)->AddApplication (app2);
    app2->SetStartTime (Seconds (1));
    app2->SetStopTime (Seconds (20));

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
