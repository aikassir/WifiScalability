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

#include <string>
#include <list>
#include <boost/lexical_cast.hpp>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("device1");

class apApp : public Application
{
public:
//    apApp(){}
//    ~apApp(){}
    void SetCycle(uint32_t Tcycle);
    void SetTs0(double Ts0);
private:

    virtual void StartApplication (void);
    virtual void StopApplication (void){}

    Ptr<Socket> m_socket;

    Ptr<WifiNetDevice> m_device;
    Ptr<ApWifiMac> m_mac;
    std::set<int> ids;
    uint32_t m_Tcycle;
    double m_Ts0; //shortest possible Tslot for 200 Bytes data. predefined through 802.11n minimum rate and SetTs0() function

//    bool ConnectionRequested(Ptr<Socket> socket, const Address& address);
//    void ConnectionAccepted(Ptr<Socket> socket, const Address& address);

    void RequestId(Ptr<Socket> socket);
};

void apApp::StartApplication ()
{
    m_device = StaticCast<WifiNetDevice>(m_node->GetDevice(0));
    m_mac = StaticCast<ApWifiMac>(m_device->GetMac());
    uint16_t apPort = 9996;
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

    // get next id
    int id=1;
    while(ids.find(id)!=ids.end())
    {
        id++;
    }

    std::string sid=boost::lexical_cast<std::string>(id);
    Ptr<Packet> packet = Create<Packet> ((const uint8_t*)sid.c_str (),sid.size());
    socket->SendTo(packet,0,addr);

  /* THIS PART NEEDS TO CHANGE */

//    int maxSlotNum= (m_Tcycle/m_Ts0);
//    // add id to set
//    ids.insert(id);
//    int channNum=1;
//    if (id> maxSlotNum)
//      {
//        channNum=2;
//      }
//    else if (id>2*maxSlotNum)
//        {
//            channNum=3;
//        }
//    // send id & channNum
//    std::string schannNum = boost::lexical_cast<std::string>(channNum);
//    // Must make them into one packet: put both in one buffer then at Rx read a specific length and store it as id, the rest is channNum
//    //id packet
//    std::vector<std::string> data;
//    data.push_back(sid);
//    data.push_back(schannNum);
//    std::string buff;
//    for (auto& s : data)
//        {
//            buff +=s;
//        }

//    std::cout<<buff;
//    Ptr<Packet> packet = Create<Packet> ((const uint8_t*)buff.c_str(),buff.size());
//    socket->SendTo(packet,0,addr);
}

void apApp::SetCycle(uint32_t Tcycle)
{
    m_Tcycle=Tcycle;
}

void apApp::SetTs0(double Ts0)
{
    m_Ts0=Ts0;
}


// create custom application to replicate WiFi module firmware
class staApp : public Application
{
public:
    staApp (Ptr<Node> node, Ipv4Address addr,Ipv4Address addr1, uint32_t id,  uint32_t packetSize, uint32_t nPackets, uint32_t nWifi);
    void SetSlotTime(double tslot);
    void SetCycle (uint32_t Tcycle);
//    virtual ~staApp(){}

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleAssociation(int device);
    void StartAssociation (int device);
    void StopAssociation (void);

    void ScheduleRequestId();
    void RequestId(); // funtion requesting id from AP
    void UpdateId(Ptr<Socket> socket); // callback receiving id from AP

    void SendPacket(void);
    void ScheduleTx(void);

    Ptr<Node> m_node;
    std::vector< Ptr<Socket> > m_sockets;
    Ipv4Address m_peer;
    Ipv4Address m_peer1;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    uint32_t m_packetsSent;
    uint32_t m_id;
    double m_tslot;
    uint32_t m_nWifi;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_Tcycle;
    uint32_t m_channNum;

    std::vector< Ptr<WifiNetDevice> > m_devices;
    std::vector< Ptr<StaWifiMac> > m_macs;
};

staApp::staApp (Ptr<Node> node, Ipv4Address addr,Ipv4Address addr1,uint32_t id, uint32_t packetSize, uint32_t nPackets, uint32_t nWifi)
  : m_node(node),
    m_peer(addr),
    m_peer1(addr1),
    m_packetSize(packetSize),
    m_nPackets(nPackets),
    m_packetsSent(0),
    m_id(id),
    m_tslot(0),
    m_nWifi(nWifi),
    m_sendEvent(),
    m_running(false)
{
    m_devices.push_back( StaticCast<WifiNetDevice>(m_node->GetDevice(0)) );
    m_devices.push_back( StaticCast<WifiNetDevice>(m_node->GetDevice(1)) );
    m_macs.push_back (StaticCast<StaWifiMac>(m_devices[0]->GetMac()));
    m_macs.push_back (StaticCast<StaWifiMac>(m_devices[1]->GetMac()));

    m_macs[0]->SetLinkUpCallback (MakeCallback(&staApp::ScheduleRequestId,this)); // setup callback for requesting id

    Ipv4Address staIpv4Address0=m_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    uint16_t staPort = 9996;
    Address staAddress0 (InetSocketAddress (staIpv4Address0, staPort));
    m_sockets.push_back(Socket::CreateSocket (m_node, UdpSocketFactory::GetTypeId ()));

    m_sockets[0]->SetRecvCallback(MakeCallback(&staApp::UpdateId,this));
    m_sockets[0]->Bind(staAddress0);

    m_macs[1]->SetLinkUpCallback (MakeCallback(&staApp::ScheduleTx,this));
    Ipv4Address staIpv4Address1=m_node->GetObject<Ipv4>()->GetAddress (2,0).GetLocal();
    staPort = 9998;
    Address staAddress1 (InetSocketAddress (staIpv4Address1, staPort));
    m_sockets.push_back (Socket::CreateSocket (m_node, UdpSocketFactory::GetTypeId ()));
    m_sockets[1]->Bind (staAddress1);
}

void
staApp::StartApplication (void)
{
    //RequestId ();
    ScheduleAssociation (0);
//    ScheduleRequestId ();
//    Time tstart = MilliSeconds(m_id*m_tslot);
//    Simulator::Schedule(tstart,&staApp::SendPacket,this);

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

void staApp::StartAssociation (int device)
{
    m_macs[device]->SetAttribute ("ActiveProbing",BooleanValue(true));
//    if (device ==1)
//      {
//        PeriodicTx ();
//      }
}

void staApp::ScheduleAssociation (int device)
{
//    Time tNow=Simulator::Now ();
//    double tSec=std::round(tNow.GetSeconds ());
//    Time tNext(Seconds(tSec+1)); // start on the next second
//    tNext+=MilliSeconds(m_id*m_tslot);
//    Simulator::Schedule (tNext - tNow, &staApp::StartAssociation, this, device);
//    Time tNext (MilliSeconds(2));
//    m_sendEvent = Simulator::Schedule (tNext, &staApp::StartAssociation, this,device);

    m_sendEvent = Simulator::Schedule (MilliSeconds(m_id*m_tslot), &staApp::StartAssociation, this,device);

//    if (device ==1)
//      {
//        Time tNext (MilliSeconds(m_id+2));
//        m_sendEvent = Simulator::Schedule (tNext, &staApp::StartAssociation, this,device);
//      }
}

void staApp::ScheduleRequestId()
{
//    Time tNow=Simulator::Now ();
//    double tSec=std::round(tNow.GetSeconds ());
//    Time tNext(Seconds(tSec+1)); // start on the next second
//    tNext+=MilliSeconds(m_id*m_tslot);
//    Simulator::Schedule (tNext - tNow, &staApp::RequestId, this);
  Simulator::Schedule (MilliSeconds(m_id*m_tslot), &staApp::RequestId, this);

}

void staApp::RequestId ()
{
    Ptr<Packet> packet = Create<Packet> (64);
    uint16_t apPort = 9996;
    Address apAddress (InetSocketAddress (m_peer, apPort));
    m_sockets[0]->SendTo(packet,0,apAddress);
}

void staApp::UpdateId(Ptr<Socket> socket)
{
  /* THIS PART CAUSES CRASH */
  Ptr<Packet> packet=socket->Recv ();
  //std::vector<std::string> data;
  std::ostringstream ostr;
  packet->CopyData(&ostr, packet->GetSize());
  m_id=boost::lexical_cast<uint32_t>(ostr.str ()[0]);

//  m_channNum = boost::lexical_cast<uint32_t>(ostr.str()[1]);
  // once id is updated, start association on second device
  // set phy channel number to m_channNum
//  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
//  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
//  phy.SetChannel (channel.Create ());
//  phy.Set("ChannelNumber",UintegerValue(m_channNum));
//  Ptr<YansWifiPhy> phy = StaticCast<YansWifiPhy>(m_macs[1]->GetWifiPhy());
//  phy->SetChannelNumber(m_channNum);
//  m_macs[1]->SetWifiPhy((const Ptr<WifiPhy>) phy);

  ScheduleAssociation (1);
}


void staApp::ScheduleTx (void)
{
//    Time tNow=Simulator::Now ();
//    double tSec=std::round(tNow.GetSeconds ());
//    Time tNext(Seconds(tSec+1)); // start on the next second
//    tNext+=MilliSeconds(m_nWifi*m_tslot );
//    m_sendEvent = Simulator::Schedule (tNext - tNow, &staApp::SendPacket,this);
  if (m_packetsSent==0)
      {
          Time tNext =MilliSeconds(m_id*m_tslot );
          m_sendEvent = Simulator::Schedule (tNext, &staApp::SendPacket,this);
      }
  else
      {
          Time tNext(Seconds(m_Tcycle ));
          m_sendEvent = Simulator::Schedule (tNext, &staApp::SendPacket,this);
      }
}

void staApp::SendPacket (void)
{
    Ptr<Packet> packet = Create<Packet> (m_packetSize);
    uint16_t apPort = 9998;
    Address apAddress (InetSocketAddress (m_peer1, apPort));
    m_sockets[1]->SendTo(packet,0,apAddress);
    if (++m_packetsSent<m_nPackets)
    {
        ScheduleTx ();
    }
}

void staApp::SetSlotTime (double tslot)
{
  m_tslot = tslot;
}

void staApp::SetCycle (uint32_t Tcycle)
{
    m_Tcycle=Tcycle;
}

int nDropTx = 0;

static void ApPhyRxDrop(Ptr<const Packet> p)
{
  nDropTx++;
}


int main (int argc, char *argv[])
{
    uint32_t nWifi[39];
    for (int i=0; i<39; i++)
      {
        nWifi[i]= 100+50*i;
      }

    uint32_t Tcycle[] = {1,5,10,20,30,45,60};

//    CommandLine cmd;
//    cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
//    cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
//    cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

//    cmd.Parse (argc,argv);

    Packet::EnablePrinting ();


    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    ns3::PacketMetadata::Enable();

//////////////////////////////////////////
    //  START TESTS //
//////////////////////////////////////////

    std::ofstream ofs;
    ofs.open("packet-drop-percent.txt");

for (int i=0; i<39; i++)
{
        for (int j=0; j<7; j++)
            {
                nDropTx =0;
                // Nodes and containers
                NodeContainer wifiStaNodes;
                wifiStaNodes.Create (nWifi[i]);
                NodeContainer wifiApNode;
                wifiApNode.Create(1);

                /////////////////
                // PHYs and MACs
                /////////////////

                // Assoc Channel
                YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
                YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
                phy.SetChannel (channel.Create ());
                phy.Set("ChannelNumber",UintegerValue(0));

                // Connection Chann1
                YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default ();
                YansWifiPhyHelper phy1 = YansWifiPhyHelper::Default ();
                phy1.SetChannel (channel1.Create ());
                phy1.Set("ChannelNumber",UintegerValue(1));

                // Connection Chann2
                YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
                YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default ();
                phy2.SetChannel (channel2.Create ());
                phy2.Set("ChannelNumber",UintegerValue(2));

                //Connection Chann3
                YansWifiChannelHelper channel3 = YansWifiChannelHelper::Default ();
                YansWifiPhyHelper phy3 = YansWifiPhyHelper::Default ();
                phy3.SetChannel (channel3.Create ());
                phy3.Set("ChannelNumber",UintegerValue(3));

                WifiHelper wifi;
                wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

                WifiMacHelper mac;
                Ssid ssid = Ssid ("ns-3-ssid");
                mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));

                NetDeviceContainer staDevices0;
                staDevices0 = wifi.Install (phy, mac, wifiStaNodes);

                NetDeviceContainer staDevices1;
                staDevices1 = wifi.Install (phy1, mac, wifiStaNodes);


                mac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid),"BeaconGeneration", BooleanValue(false),"BeaconInterval", TimeValue(Days(1)));

                NetDeviceContainer apDevices, apDevices1, apDevices2, apDevices3;
                apDevices = wifi.Install (phy, mac, wifiApNode);
                apDevices1 = wifi.Install(phy1,mac,wifiApNode);
            //    apDevices2 = wifi.Install(phy2,mac,wifiApNode);
            //    apDevices3 = wifi.Install(phy3,mac,wifiApNode);

                // mobility configuration
                MobilityHelper mobility;
                mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                             "MinX", DoubleValue (0.0),
                                             "MinY", DoubleValue (0.0),
                                             "DeltaX", DoubleValue (0.5),
                                             "DeltaY", DoubleValue (0.5),
                                             "GridWidth", UintegerValue (20),
                                             "LayoutType", StringValue ("RowFirst"));

                mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                         "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
                mobility.Install (wifiStaNodes);

                mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
                mobility.Install (wifiApNode);


                ///////////////////////
                // IPs and interfaces
                ///////////////////////

                InternetStackHelper stack;
                stack.Install (wifiApNode);
                stack.Install (wifiStaNodes);

                Ipv4AddressHelper address;
                Ipv4InterfaceContainer wifiInterfaces0;
                Ipv4InterfaceContainer wifiInterfaces1;
                Ipv4InterfaceContainer apInterface;
                Ipv4InterfaceContainer apInterface1;

                address.SetBase ("192.168.0.0", "255.255.248.0");
                apInterface = address.Assign (apDevices);
                wifiInterfaces0 = address.Assign (staDevices0);
                address.SetBase ("10.1.0.0", "255.255.248.0");
                apInterface1 = address.Assign (apDevices1);
                wifiInterfaces1 = address.Assign (staDevices1);

                // app for request id on first channel

                Ptr<apApp> apApp1 = CreateObject<apApp>();
                apApp1->SetCycle(Tcycle[j]);
                apApp1->SetTs0(0.247e-6);
                wifiApNode.Get(0)->AddApplication(apApp1);
                apApp1->SetStartTime(Seconds(0));
                apApp1->SetStopTime(Seconds(200));

                // Packet sink application for data tx on second channel
                uint16_t sinkPort = 9998;
                Address sinkAddress (InetSocketAddress (apInterface1.GetAddress (0), sinkPort));
                PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddress);
                ApplicationContainer sinkApps = packetSinkHelper.Install (wifiApNode.Get (0));
                sinkApps.Start (Seconds (0));
                sinkApps.Stop (Seconds (201));
                //

                Ptr<WifiNetDevice> apwifidev = StaticCast<WifiNetDevice>(apDevices1.Get (0));
                Ptr<WifiPhy> apPhy = apwifidev->GetMac()->GetWifiPhy();
                apPhy->TraceConnectWithoutContext("PhyRxDrop",MakeCallback(&ApPhyRxDrop));



                for (uint32_t k=0; k<nWifi[i]; k++)
                  {
                    Ptr<staApp> app1 = CreateObject<staApp> (wifiStaNodes.Get (k), apInterface.GetAddress(0), apInterface1.GetAddress(0), k, 200, 2, nWifi[i]);
                    app1->SetCycle(Tcycle[j]);
                    wifiStaNodes.Get (k)->AddApplication (app1);
                    double tslot=Tcycle[j]/nWifi[i];
                    app1->SetSlotTime(tslot);
                    app1->SetStartTime (MilliSeconds (1000));
                    app1->SetStopTime (Seconds (200));
                  }

                Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

                Simulator::Stop (Seconds (201.0));

                Simulator::Run ();
                Simulator::Destroy ();

                uint64_t totalPacketsThrough = DynamicCast<PacketSink> (sinkApps.Get(0))->GetTotalRx ();
            //    std::cout<< DynamicCast<PacketSink> (sinkApps.Get(0))->GetAcceptedSockets().size()<<std::endl;
                std::cout<< "For nWifi="<<nWifi[i]<< " Tc="<<Tcycle[j]<<std::endl;
                std::cout<<totalPacketsThrough<< " Total Rx packets"<<std::endl;
                std::cout<< nDropTx<<" Dropped packets at Phy"<<std::endl;
                ofs<<((double)nDropTx/(2*nWifi[i]))*100.0<<" ";
            }
        ofs<<std::endl;
    }
    ofs.close();
    return 0;
}

