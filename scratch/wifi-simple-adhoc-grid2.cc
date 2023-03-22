/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

//
// This program configures a grid (default 5x5) of nodes on an
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n20  n21  n22  n23  n24
// n15  n16  n17  n18  n19
// n10  n11  n12  n13  n14
// n5   n6   n7   n8   n9
// n0   n1   n2   n3   n4
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc-grid --distance=500"
// ./waf --run "wifi-simple-adhoc-grid --distance=1000"
// ./waf --run "wifi-simple-adhoc-grid --distance=1500"
//
// The source node and sink node can be changed like this:
//
// ./waf --run "wifi-simple-adhoc-grid --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-stack-helper.h"

#include <vector>
#include <iostream>

#include <random>
#include <time.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"




using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid2");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

//static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
//                             uint32_t pktCount, Time pktInterval )
//{
//  if (pktCount > 0)
//    {
//      socket->Send (Create<Packet> (pktSize));
//      Simulator::Schedule (pktInterval, &GenerateTraffic,
//                           socket, pktSize,pktCount - 1, pktInterval);
//    }
//  else
//    {
//      socket->Close ();
//    }
//}




int main (int argc, char *argv[])
{
  std::string phyMode ("ErpOfdmRate54Mbps");
  double distance = 200;  // m
  uint32_t packetSize = 1024; // bytes
  uint32_t numPackets = 0;
  uint32_t numNodes = 25;  // by default, 5x5
  uint32_t sinkNode = 0;
  uint32_t sourceNode = 7;
  double interval = 1.0; // seconds
  bool verbose = false;
  bool tracing = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);
  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (numNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  YansWifiPhyHelper wifiPhy;
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard (WIFI_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (sqrt(numNodes)),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;


  Ipv4ListRoutingHelper list;
  //　OlsrHelperのlistにルーチングプロトコルを追加
  OlsrHelper list2;
  list.Add (staticRouting, 0);
  list.Add(olsr, 10);
  //list2.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 70);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (sourceNode), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 70);
  source->Connect (remote);



  UdpEchoServerHelper echoServer (80);

  //自分で付け加えた(植田)

  int resource_table[numNodes];
  int resource2_table[numNodes];
  uint32_t node_number[5];
  int over_count=0;
  uint32_t origin = 0;
  uint32_t destination = 0;
  int n;

  //int flow1_number[200],flow2_number[200],flow3_number[200],flow4_number[200],flow5_number[200];
  //int flow,flow1_sum=0,flow2_sum=0,flow3_sum=0,flow4_sum=0,flow5_sum=0,flow6_sum=0;
  //Time flow1_num_sum=Seconds(0.0),flow2_num_sum=Seconds(0.0),flow3_num_sum=Seconds(0.0),flow4_num_sum=Seconds(0.0),flow5_num_sum=Seconds(0.0);
  //Time minus_sum1=Seconds(0.0),minus_sum2=Seconds(0.0),minus_sum3=Seconds(0.0),minus_sum4=Seconds(0.0),minus_sum5=Seconds(0.0);
/*
  for(int i=0;i<200;i++){
	  flow1_number[i]=0;
	  flow2_number[i]=0;
	  flow3_number[i]=0;
	  flow4_number[i]=0;
	  flow5_number[i]=0;
  }

*/

  Time average_list[1040];
  for(int i=0;i<1040;i++){
	  average_list[i]=Seconds(0.0);
  }


  std::vector <int> average_list_over;

  std::vector <Time> average_list_proposal;
  std::vector <Time> average_list_over_time;


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();



  for(double z=0;z<1040;z++){







  for(uint32_t i=0;i<numNodes;i++){
	  srand(z*7+i*4+3);
 	  resource_table[i]=rand()%101;
  }



  std::map<int,std::vector <int>> core_node_number;

  std::map<int,std::vector <int>> core_node_number_neighbor;

  std::vector <int> non_core_node;

  core_node_number.insert(std::make_pair(0,0));

  core_node_number_neighbor.insert(std::make_pair(0,0));

  std::vector <int> core_1={1,2,3,5,6,7,11};
    std::vector <int> core_2={1,2,3,4,6,7,8,12};
    std::vector <int> core_3={1,2,3,4,7,8,9,13};
    std::vector <int> core_4={2,3,4,8,9,14};
    std::vector <int> core_5={1,5,6,7,10,11,15};
    std::vector <int> core_6={1,2,5,6,7,8,10,11,12,16};
    std::vector <int> core_7={1,2,3,5,6,7,8,9,11,12,13,17};
    std::vector <int> core_8={2,3,4,6,7,8,9,12,13,14,18};
    std::vector <int> core_9={3,4,7,8,9,13,14,19};
    std::vector <int> core_10={5,6,10,11,12,15,16,20};
    std::vector <int> core_11={1,5,6,7,10,11,12,13,15,16,17,21};
    std::vector <int> core_12={2,6,7,8,10,11,12,13,14,16,17,18,22};
    std::vector <int> core_13={3,7,8,9,11,12,13,14,17,18,19,23};
    std::vector <int> core_14={4,8,9,12,13,14,18,19,24};
    std::vector <int> core_15={5,10,11,15,16,17,20,21};
    std::vector <int> core_16={6,10,11,12,15,16,17,18,20,21,22};
    std::vector <int> core_17={7,11,12,13,15,16,17,18,19,21,22,23};
    std::vector <int> core_18={8,12,13,14,16,17,18,19,22,23,24};
    std::vector <int> core_19={9,13,14,17,18,19,23,24};
    std::vector <int> core_20={10,15,16,20,21,22};
    std::vector <int> core_21={11,15,16,17,20,21,22,23};
    std::vector <int> core_22={12,16,17,18,20,21,22,23,24};
    std::vector <int> core_23={13,17,18,19,21,22,23,24};
    std::vector <int> core_24={14,18,19,22,23,24};
   /* std::vector <int> core_25={13,18,19,20,23,24,25,26,27,30,31,32};
    std::vector <int> core_26={14,19,20,21,24,25,26,27,28,31,32,33};
    std::vector <int> core_27={15,20,21,22,25,26,27,28,29,32,33,34};
    std::vector <int> core_28={16,21,22,23,26,27,28,29,33,34,35};
    std::vector <int> core_29={17,22,23,27,28,29,34,35};
    std::vector <int> core_30={18,24,25,30,31,32};
    std::vector <int> core_31={19,24,25,26,30,31,32,33};
    std::vector <int> core_32={20,25,26,27,30,31,32,33,34};
    std::vector <int> core_33={21,26,27,28,31,32,33,34,35};
    std::vector <int> core_34={22,27,28,29,32,33,34,35};
    std::vector <int> core_35={23,28,29,33,34,35};
    std::vector <int> core_36={22,28,29,30,35,36,37,38,42,43,44};
    std::vector <int> core_37={23,29,30,31,35,36,37,38,39,43,44,45};
    std::vector <int> core_38={24,30,31,32,36,37,38,39,40,44,45,46};
    std::vector <int> core_39={25,31,32,33,37,38,39,40,41,45,46,47};
    std::vector <int> core_40={26,32,33,34,38,39,40,41,46,47,48};
    std::vector <int> core_41={27,33,34,39,40,41,47,48};
    std::vector <int> core_42={28,35,36,42,43,44};
    std::vector <int> core_43={29,35,36,37,42,43,44,45};
    std::vector <int> core_44={30,36,37,38,42,43,44,45,46};
    std::vector <int> core_45={31,37,38,39,43,44,45,46,47};
    std::vector <int> core_46={32,38,39,40,44,45,46,47,48};
    std::vector <int> core_47={33,39,40,41,45,46,47,48};
    std::vector <int> core_48={34,40,41,46,47,48};
  */
    std::vector <int> core_1_neighbor={1,2,6};
    std::vector <int> core_2_neighbor={1,2,3,7};
    std::vector <int> core_3_neighbor={2,3,4,8};
    std::vector <int> core_4_neighbor={3,4,9};
    std::vector <int> core_5_neighbor={5,6,10};
    std::vector <int> core_6_neighbor={1,5,6,7,11};
    std::vector <int> core_7_neighbor={2,6,7,8,12};
    std::vector <int> core_8_neighbor={3,7,8,9,13};
    std::vector <int> core_9_neighbor={4,8,9,14};
    std::vector <int> core_10_neighbor={5,10,11,15};
    std::vector <int> core_11_neighbor={6,10,11,12,16};
    std::vector <int> core_12_neighbor={7,11,12,13,17};
    std::vector <int> core_13_neighbor={8,12,13,14,18};
    std::vector <int> core_14_neighbor={9,13,14,19};
    std::vector <int> core_15_neighbor={10,15,16,20};
    std::vector <int> core_16_neighbor={11,15,16,17,21};
    std::vector <int> core_17_neighbor={12,16,17,18,22};
    std::vector <int> core_18_neighbor={13,17,18,19,23};
    std::vector <int> core_19_neighbor={14,18,19,24};
    std::vector <int> core_20_neighbor={15,20,21};
    std::vector <int> core_21_neighbor={16,20,21,22};
    std::vector <int> core_22_neighbor={17,21,22,23};
    std::vector <int> core_23_neighbor={18,22,23,24};
    std::vector <int> core_24_neighbor={19,23,24};
   /* std::vector <int> core_25_neighbor={19,24,25,26,31};
    std::vector <int> core_26_neighbor={20,25,26,27,32};
    std::vector <int> core_27_neighbor={21,26,27,28,33};
    std::vector <int> core_28_neighbor={22,27,28,29,34};
    std::vector <int> core_29_neighbor={23,28,29,35};
    std::vector <int> core_30_neighbor={24,30,31};
    std::vector <int> core_31_neighbor={25,30,31,32};
    std::vector <int> core_32_neighbor={26,31,32,33};
    std::vector <int> core_33_neighbor={27,32,33,34};
    std::vector <int> core_34_neighbor={28,33,34,35};
    std::vector <int> core_35_neighbor={29,34,35};
    std::vector <int> core_36_neighbor={29,35,36,37,43};
    std::vector <int> core_37_neighbor={30,36,37,38,44};
    std::vector <int> core_38_neighbor={31,37,38,39,45};
    std::vector <int> core_39_neighbor={32,38,39,46};
    std::vector <int> core_40_neighbor={33,39,40,41,47};
    std::vector <int> core_41_neighbor={34,40,41,48};
    std::vector <int> core_42_neighbor={35,42,43};
    std::vector <int> core_43_neighbor={36,42,43,44};
    std::vector <int> core_44_neighbor={37,43,44,45};
    std::vector <int> core_45_neighbor={38,44,45,46};
    std::vector <int> core_46_neighbor={39,45,46,47};
    std::vector <int> core_47_neighbor={40,46,47,48};
    std::vector <int> core_48_neighbor={41,47,48};
  */


      core_node_number.insert(std::make_pair(1,core_1));
      core_node_number.insert(std::make_pair(2,core_2));
      core_node_number.insert(std::make_pair(3,core_3));
      core_node_number.insert(std::make_pair(4,core_4));
      core_node_number.insert(std::make_pair(5,core_5));
      core_node_number.insert(std::make_pair(6,core_6));
      core_node_number.insert(std::make_pair(7,core_7));
      core_node_number.insert(std::make_pair(8,core_8));
      core_node_number.insert(std::make_pair(9,core_9));
      core_node_number.insert(std::make_pair(10,core_10));
      core_node_number.insert(std::make_pair(11,core_11));
      core_node_number.insert(std::make_pair(12,core_12));
      core_node_number.insert(std::make_pair(13,core_13));
      core_node_number.insert(std::make_pair(14,core_14));
      core_node_number.insert(std::make_pair(15,core_15));
      core_node_number.insert(std::make_pair(16,core_16));
      core_node_number.insert(std::make_pair(17,core_17));
      core_node_number.insert(std::make_pair(18,core_18));
      core_node_number.insert(std::make_pair(19,core_19));
      core_node_number.insert(std::make_pair(20,core_20));
      core_node_number.insert(std::make_pair(21,core_21));
      core_node_number.insert(std::make_pair(22,core_22));
      core_node_number.insert(std::make_pair(23,core_23));
      core_node_number.insert(std::make_pair(24,core_24));
//      core_node_number.insert(std::make_pair(25,core_25));
//      core_node_number.insert(std::make_pair(26,core_26));
//      core_node_number.insert(std::make_pair(27,core_27));
//      core_node_number.insert(std::make_pair(28,core_28));
//      core_node_number.insert(std::make_pair(29,core_29));
//      core_node_number.insert(std::make_pair(30,core_30));
//      core_node_number.insert(std::make_pair(31,core_31));
//      core_node_number.insert(std::make_pair(32,core_32));
//      core_node_number.insert(std::make_pair(33,core_33));
//      core_node_number.insert(std::make_pair(34,core_34));
//      core_node_number.insert(std::make_pair(35,core_35));
//      core_node_number.insert(std::make_pair(36,core_36));
//      core_node_number.insert(std::make_pair(37,core_37));
//      core_node_number.insert(std::make_pair(38,core_38));
//      core_node_number.insert(std::make_pair(39,core_39));
//      core_node_number.insert(std::make_pair(40,core_40));
//      core_node_number.insert(std::make_pair(41,core_41));
//      core_node_number.insert(std::make_pair(42,core_42));
//      core_node_number.insert(std::make_pair(43,core_43));
//      core_node_number.insert(std::make_pair(44,core_44));
//      core_node_number.insert(std::make_pair(45,core_45));
//      core_node_number.insert(std::make_pair(46,core_46));
//      core_node_number.insert(std::make_pair(47,core_47));
//      core_node_number.insert(std::make_pair(48,core_48));
//      core_node_number.insert(std::make_pair(49,core_49));
//      core_node_number.insert(std::make_pair(50,core_50));
//      core_node_number.insert(std::make_pair(51,core_51));
//      core_node_number.insert(std::make_pair(52,core_52));
//      core_node_number.insert(std::make_pair(53,core_53));
//      core_node_number.insert(std::make_pair(54,core_54));
//      core_node_number.insert(std::make_pair(55,core_55));
//      core_node_number.insert(std::make_pair(56,core_56));
//      core_node_number.insert(std::make_pair(57,core_57));
//      core_node_number.insert(std::make_pair(58,core_58));
//      core_node_number.insert(std::make_pair(59,core_59));
//      core_node_number.insert(std::make_pair(60,core_60));
//      core_node_number.insert(std::make_pair(61,core_61));
//      core_node_number.insert(std::make_pair(62,core_62));
//      core_node_number.insert(std::make_pair(63,core_63));
//      core_node_number.insert(std::make_pair(64,core_64));
//      core_node_number.insert(std::make_pair(65,core_65));
//      core_node_number.insert(std::make_pair(66,core_66));
//      core_node_number.insert(std::make_pair(67,core_67));
//      core_node_number.insert(std::make_pair(68,core_68));
//      core_node_number.insert(std::make_pair(69,core_69));
//      core_node_number.insert(std::make_pair(70,core_70));
//      core_node_number.insert(std::make_pair(71,core_71));
//      core_node_number.insert(std::make_pair(72,core_72));
//      core_node_number.insert(std::make_pair(73,core_73));
//      core_node_number.insert(std::make_pair(74,core_74));
//      core_node_number.insert(std::make_pair(75,core_75));
//      core_node_number.insert(std::make_pair(76,core_76));
//      core_node_number.insert(std::make_pair(77,core_77));
//      core_node_number.insert(std::make_pair(78,core_78));
//      core_node_number.insert(std::make_pair(79,core_79));
//      core_node_number.insert(std::make_pair(80,core_80));
//      core_node_number.insert(std::make_pair(81,core_81));
//      core_node_number.insert(std::make_pair(82,core_82));
//      core_node_number.insert(std::make_pair(83,core_83));
//      core_node_number.insert(std::make_pair(84,core_84));
//      core_node_number.insert(std::make_pair(85,core_85));
//      core_node_number.insert(std::make_pair(86,core_86));
//      core_node_number.insert(std::make_pair(87,core_87));
//      core_node_number.insert(std::make_pair(88,core_88));
//      core_node_number.insert(std::make_pair(89,core_89));
//      core_node_number.insert(std::make_pair(90,core_90));
//      core_node_number.insert(std::make_pair(91,core_91));
//      core_node_number.insert(std::make_pair(92,core_92));
//      core_node_number.insert(std::make_pair(93,core_93));
//      core_node_number.insert(std::make_pair(94,core_94));
//      core_node_number.insert(std::make_pair(95,core_95));
//      core_node_number.insert(std::make_pair(96,core_96));
//      core_node_number.insert(std::make_pair(97,core_97));
//      core_node_number.insert(std::make_pair(98,core_98));
//      core_node_number.insert(std::make_pair(99,core_99));


      core_node_number_neighbor.insert(std::make_pair(1,core_1_neighbor));
      core_node_number_neighbor.insert(std::make_pair(2,core_2_neighbor));
      core_node_number_neighbor.insert(std::make_pair(3,core_3_neighbor));
      core_node_number_neighbor.insert(std::make_pair(4,core_4_neighbor));
      core_node_number_neighbor.insert(std::make_pair(5,core_5_neighbor));
      core_node_number_neighbor.insert(std::make_pair(6,core_6_neighbor));
      core_node_number_neighbor.insert(std::make_pair(7,core_7_neighbor));
      core_node_number_neighbor.insert(std::make_pair(8,core_8_neighbor));
      core_node_number_neighbor.insert(std::make_pair(9,core_9_neighbor));
      core_node_number_neighbor.insert(std::make_pair(10,core_10_neighbor));
      core_node_number_neighbor.insert(std::make_pair(11,core_11_neighbor));
      core_node_number_neighbor.insert(std::make_pair(12,core_12_neighbor));
      core_node_number_neighbor.insert(std::make_pair(13,core_13_neighbor));
      core_node_number_neighbor.insert(std::make_pair(14,core_14_neighbor));
      core_node_number_neighbor.insert(std::make_pair(15,core_15_neighbor));
      core_node_number_neighbor.insert(std::make_pair(16,core_16_neighbor));
      core_node_number_neighbor.insert(std::make_pair(17,core_17_neighbor));
      core_node_number_neighbor.insert(std::make_pair(18,core_18_neighbor));
      core_node_number_neighbor.insert(std::make_pair(19,core_19_neighbor));
      core_node_number_neighbor.insert(std::make_pair(20,core_20_neighbor));
      core_node_number_neighbor.insert(std::make_pair(21,core_21_neighbor));
      core_node_number_neighbor.insert(std::make_pair(22,core_22_neighbor));
      core_node_number_neighbor.insert(std::make_pair(23,core_23_neighbor));
      core_node_number_neighbor.insert(std::make_pair(24,core_24_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(25,core_25_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(26,core_26_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(27,core_27_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(28,core_28_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(29,core_29_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(30,core_30_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(31,core_31_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(32,core_32_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(33,core_33_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(34,core_34_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(35,core_35_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(36,core_36_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(37,core_37_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(38,core_38_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(39,core_39_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(40,core_40_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(41,core_41_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(42,core_42_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(43,core_43_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(44,core_44_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(45,core_45_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(46,core_46_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(47,core_47_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(48,core_48_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(49,core_49_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(50,core_50_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(51,core_51_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(52,core_52_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(53,core_53_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(54,core_54_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(55,core_55_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(56,core_56_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(57,core_57_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(58,core_58_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(59,core_59_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(60,core_60_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(61,core_61_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(62,core_62_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(63,core_63_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(64,core_64_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(65,core_65_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(66,core_66_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(67,core_67_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(68,core_68_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(69,core_69_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(70,core_70_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(71,core_71_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(72,core_72_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(73,core_73_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(74,core_74_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(75,core_75_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(76,core_76_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(77,core_77_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(78,core_78_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(79,core_79_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(80,core_80_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(81,core_81_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(82,core_82_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(83,core_83_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(84,core_84_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(85,core_85_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(86,core_86_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(87,core_87_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(88,core_88_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(89,core_89_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(90,core_90_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(91,core_91_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(92,core_92_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(93,core_93_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(94,core_94_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(95,core_95_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(96,core_96_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(97,core_97_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(98,core_98_neighbor));
//      core_node_number_neighbor.insert(std::make_pair(99,core_99_neighbor));






  int sum_neighbor;
  int sum_resource_neighbor[numNodes];
  int neighbor_size=core_node_number_neighbor.size();
  int ite=0;



  for(double k=0;k<5;k++){



  for(int i=0;i<neighbor_size;i++){
	  std::vector <int> x=core_node_number_neighbor.at(i);
	  sum_neighbor=0;
	  for (int j : x) {
	      sum_neighbor+=resource_table[j];
	   }
	  sum_resource_neighbor[i]=sum_neighbor;
   }
  int max_neighbor=sum_resource_neighbor[0];
  for(int i=0;i<neighbor_size;i++){
	  if(max_neighbor<sum_resource_neighbor[i]){
		  max_neighbor=sum_resource_neighbor[i];
		  ite=i;
	  }
  }





  std::vector <int> list1=core_node_number.at(ite);

  std::vector <int> list2;
  	list2=list1;

  	for (auto it = list2.begin(); it != list2.end();) {
  	  		    // 条件一致した要素を削除する
  	  		    if (*it == ite) {
  	  		      // 削除された要素の次を指すイテレータが返される。
  	  		      list2.erase(it);
  	  		      break;
  	  		    }
  	  		    // 要素削除をしない場合に、イテレータを進める
  	  		    else {
  	  		      ++it;
  	  		    }
  	  		  }



  	int size=list2.size();






  	int max,max2,max3,max4;
  	int max_ite,max2_ite,max3_ite,max4_ite;

  		max = resource_table[list2[0]];
  		max_ite = list2[0];
  		for(int i=0;i<size;i++){
  			if(max<resource_table[list2[i]]){
  				max=resource_table[list2[i]];
  				max_ite = list2[i];
  			}
  		}




  		for (auto it = list2.begin(); it != list2.end();) {
  		    // 条件一致した要素を削除する
  		    if (*it == max_ite) {
  		      // 削除された要素の次を指すイテレータが返される。
  		      list2.erase(it);
  		      break;
  		    }
  		    // 要素削除をしない場合に、イテレータを進める
  		    else {
  		      ++it;
  		    }
  		  }



  		size=list2.size();
  		max2 = resource_table[list2[0]];
  		max2_ite = list2[0];
  		for(int i=0;i<size;i++){
  			if(max2<resource_table[list2[i]]){
  				max2=resource_table[list2[i]];
  				max2_ite=list2[i];
  			}
  		}





  		for (auto it = list2.begin(); it != list2.end();) {
  		  		    // 条件一致した要素を削除する
  		  		    if (*it == max2_ite) {
  		  		      // 削除された要素の次を指すイテレータが返される。
  		  		      list2.erase(it);
  		  		      break;
  		  		    }
  		  		    // 要素削除をしない場合に、イテレータを進める
  		  		    else {
  		  		      ++it;
  		  		    }
  		  		  }

  		size=list2.size();
  		max3 = resource_table[list2[0]];
  		max3_ite = list2[0];
  		for(int i=0;i<size;i++){
  			if(max3<resource_table[list2[i]]){
  				max3=resource_table[list2[i]];
  				max3_ite=list2[i];
  			}
  		}


  		for (auto it = list2.begin(); it != list2.end();) {
  		  		    // 条件一致した要素を削除する
  		  		    if (*it == max3_ite) {
  		  		      // 削除された要素の次を指すイテレータが返される。
  		  		      list2.erase(it);
  		  		      break;
  		  		    }
  		  		    // 要素削除をしない場合に、イテレータを進める
  		  		    else {
  		  		      ++it;
  		  		    }
  		  		  }

  		size=list2.size();
  		max4 = resource_table[list2[0]];
  		max4_ite = list2[0];
  		for(int i=0;i<size;i++){
  			if(max4<resource_table[list2[i]]){
  				max4=resource_table[list2[i]];
  				max4_ite=list2[i];
  			}
  		}




  node_number[0]=ite;
  node_number[1]=max_ite;
  node_number[2]=max2_ite;
  node_number[3]=max3_ite;
  node_number[4]=max4_ite;




	  if(resource_table[node_number[0]]<25 || resource_table[node_number[1]]<25){
	  if(resource_table[node_number[0]]<17 || resource_table[node_number[1]]<17 || resource_table[node_number[2]]<17){
	  if(resource_table[node_number[0]]<13 || resource_table[node_number[1]]<13 || resource_table[node_number[2]]<13 || resource_table[node_number[3]]<13){
	  if(resource_table[node_number[0]]<10 || resource_table[node_number[1]]<10 || resource_table[node_number[2]]<10 || resource_table[node_number[3]]<10 || resource_table[node_number[4]]<10){

		non_core_node.push_back(ite);
		 std::cout<<"over"<<std::endl;

		 over_count=1;




  int max_most,max2_most,max3_most,max4_most,max5_most;
		  int max_most_ite,max2_most_ite,max3_most_ite,max4_most_ite,max5_most_ite;


		  for(uint32_t i=0;i<numNodes;i++){
			  resource2_table[i]=resource_table[i];
		  }


		    		max_most = resource2_table[0];
		    		max_most_ite = 0;
		    		for(uint32_t i=0;i<numNodes;i++){
		    			if(max_most<resource2_table[i]){
		    				max_most=resource2_table[i];
		    				max_most_ite = i;
		    			}
		    		}

		    		resource2_table[max_most_ite]=0;

		    		max2_most = resource2_table[0];
		    		max2_most_ite = 0;
		    		for(uint32_t i=0;i<numNodes;i++){
		    			if(max2_most<resource2_table[i]){
		    				max2_most=resource2_table[i];
		    				max2_most_ite=i;
		    			}
		    		}

		    		resource2_table[max2_most_ite]=0;


		    		max3_most = resource2_table[0];
		      		max3_most_ite = 0;
		      		for(uint32_t i=0;i<numNodes;i++){
		    			if(max3_most<resource2_table[i]){
		  	    			max3_most=resource2_table[i];
			    			max3_most_ite=i;
		 	   			}
		       		}

		    		resource2_table[max3_most_ite]=0;


		    		max4_most = resource2_table[0];
		      		max4_most_ite = 0;
		    		for(uint32_t i=0;i<numNodes;i++){
		    			if(max4_most<resource2_table[i]){
		    				max4_most=resource2_table[i];
		    				max4_most_ite=i;
		   	   			}
		      		}

		    		resource2_table[max4_most_ite]=0;

		    		max5_most = resource2_table[0];
		    		max5_most_ite = 0;
		    		for(uint32_t i=0;i<numNodes;i++){
		     			if(max5_most<resource2_table[i]){
		     				max5_most=resource2_table[i];
		     				max5_most_ite=i;
		   	   			}
		      		}


		    		node_number[0]=max_most_ite;
		    		node_number[1]=max2_most_ite;
		    		node_number[2]=max3_most_ite;
		    		node_number[3]=max4_most_ite;
		    		node_number[4]=max5_most_ite;

	  }
	  }
	  }
	  }

	  //flow=0;
	  origin=node_number[0];
	  destination=node_number[1];

	  //flow=1;
  Ptr<Socket> recvSink2 = Socket::CreateSocket (c.Get (destination), tid);
  //InetSocketAddress local2 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  //recvSink2->Bind (local2);
  recvSink2->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source2 = Socket::CreateSocket (c.Get (origin), tid);
  InetSocketAddress remote2 = InetSocketAddress (i.GetAddress (destination, 0), 80);
  source2->Connect (remote2);

  ApplicationContainer serverApps2 = echoServer.Install (c.Get (origin));
  serverApps2.Start (Seconds (z*250+30.0+k*4*5));
  serverApps2.Stop (Seconds (z*250+35.0+k*4*5));

  UdpEchoClientHelper echoClient2 (i.GetAddress (origin), 80);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));


  ApplicationContainer clientApps2 = echoClient2.Install (c.Get (destination));
  clientApps2.Start (Seconds (z*250+32.0+k*4*5));
  clientApps2.Stop (Seconds (z*250+35.0+k*4*5));


  if(resource_table[node_number[0]]<25 || resource_table[node_number[1]]<25){
	  destination=node_number[2];

	  //flow=2;

	  UdpEchoServerHelper echoServer7 (85);

	      Ptr<Socket> recvSink7 = Socket::CreateSocket (c.Get (destination), tid);
	      	  //InetSocketAddress local7 = InetSocketAddress (Ipv4Address::GetAny (), 85);
	      //recvSink7->Bind (local7);
	      recvSink7->SetRecvCallback (MakeCallback (&ReceivePacket));

	      Ptr<Socket> source7 = Socket::CreateSocket (c.Get (origin), tid);
	      InetSocketAddress remote7 = InetSocketAddress (i.GetAddress (destination, 0), 85);
	      source7->Connect (remote7);

	      ApplicationContainer serverApps7 = echoServer7.Install (c.Get (origin));
	      serverApps7.Start (Seconds (z*250+35.0+k*4*5));
	      serverApps7.Stop (Seconds (z*250+40.0+k*4*5));

	      UdpEchoClientHelper echoClient7 (i.GetAddress (origin), 85);
	      echoClient7.SetAttribute ("MaxPackets", UintegerValue (1));
	      echoClient7.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	      echoClient7.SetAttribute ("PacketSize", UintegerValue (1024));


	      ApplicationContainer clientApps7 = echoClient7.Install (c.Get (destination));
	      clientApps7.Start (Seconds (z*250+37.0+k*4*5));
	      clientApps7.Stop (Seconds (z*250+40.0+k*4*5));



	      if(resource_table[node_number[0]]<17 || resource_table[node_number[1]]<17 || resource_table[node_number[2]]<17){

	      	  destination=node_number[3];

	      	 // flow=3;

	      	  UdpEchoServerHelper echoServer8 (86);

	      	      Ptr<Socket> recvSink8 = Socket::CreateSocket (c.Get (destination), tid);
	      	      //InetSocketAddress local8 = InetSocketAddress (Ipv4Address::GetAny (), 86);
	      	      //recvSink8->Bind (local8);
	      	      recvSink8->SetRecvCallback (MakeCallback (&ReceivePacket));

	      	      Ptr<Socket> source8 = Socket::CreateSocket (c.Get (origin), tid);
	      	      InetSocketAddress remote8 = InetSocketAddress (i.GetAddress (destination, 0), 86);
	      	      source8->Connect (remote8);

	      	      ApplicationContainer serverApps8 = echoServer8.Install (c.Get (origin));
	      	      serverApps8.Start (Seconds (z*250+40.0+k*4*5));
	      	      serverApps8.Stop (Seconds (z*250+45.0+k*4*5));

	      	      UdpEchoClientHelper echoClient8 (i.GetAddress (origin), 86);
	      	      echoClient8.SetAttribute ("MaxPackets", UintegerValue (1));
	      	      echoClient8.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	      	      echoClient8.SetAttribute ("PacketSize", UintegerValue (1024));


	      	      ApplicationContainer clientApps8 = echoClient8.Install (c.Get (destination));
	      	      clientApps8.Start (Seconds (z*250+42.0+k*4*5));
	      	      clientApps8.Stop (Seconds (z*250+45.0+k*4*5));



	      	    if(resource_table[node_number[0]]<13 || resource_table[node_number[1]]<13 || resource_table[node_number[2]]<13 || resource_table[node_number[3]]<13){
	      	    	destination=node_number[4];

	      	    //	flow=4;

	      	       UdpEchoServerHelper echoServer9 (87);

	      	       Ptr<Socket> recvSink9 = Socket::CreateSocket (c.Get (destination), tid);
	      	    	//InetSocketAddress local9 = InetSocketAddress (Ipv4Address::GetAny (), 87);
	      	       //recvSink9->Bind (local9);
	      	       recvSink9->SetRecvCallback (MakeCallback (&ReceivePacket));

	      	       Ptr<Socket> source9 = Socket::CreateSocket (c.Get (origin), tid);
	      	      InetSocketAddress remote9 = InetSocketAddress (i.GetAddress (destination, 0), 87);
	      	      source9->Connect (remote9);

	      	       ApplicationContainer serverApps9 = echoServer9.Install (c.Get (origin));
	      	       serverApps9.Start (Seconds (z*250+45.0+k*4*5));
	      	       serverApps9.Stop (Seconds (z*250+50.0+k*4*5));

	      	       UdpEchoClientHelper echoClient9 (i.GetAddress (origin), 87);
	      	       echoClient9.SetAttribute ("MaxPackets", UintegerValue (1));
	      	       echoClient9.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	      	       echoClient9.SetAttribute ("PacketSize", UintegerValue (1024));


	      	       ApplicationContainer clientApps9 = echoClient9.Install (c.Get (destination));
	      	       clientApps9.Start (Seconds (z*250+47.0+k*4*5));
	      	       clientApps9.Stop (Seconds (z*250+50.0+k*4*5));


	      	    }
	      	  }


  }

int calcu=0;
  if(resource_table[node_number[0]]>=25 && resource_table[node_number[1]]>=25){
  	  resource_table[node_number[0]]=resource_table[node_number[0]]-25;
  	  resource_table[node_number[1]]=resource_table[node_number[1]]-25;
  	  calcu=1;
    }
  if(resource_table[node_number[0]]>=17 && resource_table[node_number[1]]>=17 &&resource_table[node_number[2]]>=17 && calcu==0){
  	      	  resource_table[node_number[0]]=resource_table[node_number[0]]-17;
  	      	  resource_table[node_number[1]]=resource_table[node_number[1]]-17;
  	      	  resource_table[node_number[2]]=resource_table[node_number[2]]-17;
  	      	  calcu=1;
  	        }
  if(resource_table[node_number[0]]>=13 && resource_table[node_number[1]]>=13 && resource_table[node_number[2]]>=13 && resource_table[node_number[3]]>=13 && calcu==0){
  	      	    	      	  resource_table[node_number[0]]=resource_table[node_number[0]]-13;
  	      	    	      	  resource_table[node_number[1]]=resource_table[node_number[1]]-13;
  	      	    	      	  resource_table[node_number[2]]=resource_table[node_number[2]]-13;
  	      	    	         resource_table[node_number[3]]=resource_table[node_number[3]]-13;
  	      	    	         calcu=1;
  	      	    	        }
  else if(resource_table[node_number[0]]>=10 && resource_table[node_number[1]]>=10 && resource_table[node_number[2]]>=10 && resource_table[node_number[3]]>=10 && resource_table[node_number[4]]>=6 && calcu==0){
    	      	    	      	  resource_table[node_number[0]]=resource_table[node_number[0]]-10;
    	      	    	      	  resource_table[node_number[1]]=resource_table[node_number[1]]-10;
    	      	    	      	  resource_table[node_number[2]]=resource_table[node_number[2]]-10;
    	      	    	         resource_table[node_number[3]]=resource_table[node_number[3]]-10;
    	      	    	         resource_table[node_number[4]]=resource_table[node_number[4]]-10;
    	      	    	         calcu=1;
    	      	    	        }



/*
 int z_int2=z;


  if(k==0){
  if(flow==1){
	  flow1_number[z_int2]=2;
  }
  if(flow==2){
  	  flow1_number[z_int2]=4;
    }
  if(flow==3){
  	  flow1_number[z_int2]=6;
    }
  if(flow==4){
  	  flow1_number[z_int2]=8;
    }
  }
  if(k==1){
    if(flow==1){
  	  flow2_number[z_int2]=2;
    }
    if(flow==2){
    	  flow2_number[z_int2]=4;
      }
    if(flow==3){
    	  flow2_number[z_int2]=6;
      }
    if(flow==4){
    	  flow2_number[z_int2]=8;
      }
    }
  if(k==2){
    if(flow==1){
  	  flow3_number[z_int2]=2;
    }
    if(flow==2){
    	  flow3_number[z_int2]=4;
      }
    if(flow==3){
    	  flow3_number[z_int2]=6;
      }
    if(flow==4){
    	  flow3_number[z_int2]=8;
      }
    }
  if(k==3){
    if(flow==1){
  	  flow4_number[z_int2]=2;
    }
    if(flow==2){
    	  flow4_number[z_int2]=4;
      }
    if(flow==3){
    	  flow4_number[z_int2]=6;
      }
    if(flow==4){
    	  flow4_number[z_int2]=8;
      }
    }
  if(k==4){
    if(flow==1){
  	  flow5_number[z_int2]=2;
    }
    if(flow==2){
    	  flow5_number[z_int2]=4;
      }
    if(flow==3){
    	  flow5_number[z_int2]=6;
      }
    if(flow==4){
    	  flow5_number[z_int2]=8;
      }
    }
*/

  }





  //
  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
      wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
      olsr.PrintRoutingTableAllEvery (Seconds (35.0), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
      olsr.PrintNeighborCacheAllEvery (Seconds (35.0), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }

  /*// Give OLSR time to converge-- 30 seconds perhaps
  Simulator::Schedule (Seconds (35.0), &GenerateTraffic,
                       source, packetSize, numPackets, interPacketInterval);
*/
  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);



//周辺ノードのresourceを出力
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid2.resource", std::ios::out);
  olsr.PrintResourceAll (Seconds (35.0), routingStream);

  //std::cout<<olsr.ResourceAll()<< std::endl;




  Simulator::Stop (Seconds (250.0));
  Simulator::Run ();

  /*
  int z_int4=z;
if(z_int4%2==0){
  flow2_sum=flow1_sum+flow1_number[z_int4];
  flow3_sum=flow2_sum+flow2_number[z_int4];
  flow4_sum=flow3_sum+flow3_number[z_int4];
  flow5_sum=flow4_sum+flow4_number[z_int4];
  flow6_sum=flow5_sum+flow5_number[z_int4];

*/

  //自分で付け加えた(植田)
  Time sum=Seconds(0.0);
  monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
      {
      /*Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

        std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  end-to-end  " << i->second.delaySum << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
*/
       sum=i->second.delaySum;
    	//int i_int=i->first;

/*
    	if(i_int>flow1_sum){
    		if(i_int<=flow2_sum){
    			flow1_num_sum+=i->second.delaySum;
    			if(z_int4<20){
    				minus_sum1+=i->second.delaySum;
    			}
    		}
    	}
    	if(i_int>flow2_sum){
    	    		if(i_int<=flow3_sum){
    	    			flow2_num_sum+=i->second.delaySum;
    	    			if(z_int4<20){
    	    			    	minus_sum2+=i->second.delaySum;
    	    			 }
    	    		}
    	    	}
    	if(i_int>flow3_sum){
    	    		if(i_int<=flow4_sum){
    	    			flow3_num_sum+=i->second.delaySum;
    	    			if(z_int4<20){
    	    				minus_sum3+=i->second.delaySum;
    	    			}
    	    		}
    	    	}
    	if(i_int>flow4_sum){
    	    		if(i_int<=flow5_sum){
    	    			flow4_num_sum+=i->second.delaySum;
    	    			if(z_int4<20){
    	    				minus_sum4+=i->second.delaySum;
    	    			}
    	    		}
    	    	}
    	if(i_int>flow5_sum){
    	    		if(i_int<=flow6_sum){
    	    			flow5_num_sum+=i->second.delaySum;
    	    			if(z_int4<20){
    	    				minus_sum5+=i->second.delaySum;
    	    			}
    	    		}
    	    	}

    	//std::cout<<flow6_sum<<std::endl;

*/
  }
  /*
    flow1_sum=flow6_sum;

}
*/



    int z_int_100=z;
    average_list[z_int_100]=sum;

   if(over_count==1){
       average_list_over.push_back(z_int_100);
    }

   over_count=0;


    if(z==0){
    	average_list[0]=sum;
    }
    else{
    	int z_int=z;
    	if(z_int%2==0){
    		n=z_int/2;
    		if(n==1){
    			sum=sum-average_list[0];
    		}
    		for(int s=0;s<=n-1;s++){
    		    sum=sum-average_list[z_int-2*s];
    		}
    		average_list[z_int]=sum;
    	}
    }




}

/*
flow1_num_sum=flow1_num_sum - minus_sum1;
flow2_num_sum=flow2_num_sum - minus_sum2;
flow3_num_sum=flow3_num_sum - minus_sum3;
flow4_num_sum=flow4_num_sum - minus_sum4;
flow5_num_sum=flow5_num_sum - minus_sum5;


std::cout<<flow1_num_sum/90<<std::endl;
std::cout<<flow2_num_sum/90<<std::endl;
std::cout<<flow3_num_sum/90<<std::endl;
std::cout<<flow4_num_sum/90<<std::endl;
std::cout<<flow5_num_sum/90<<std::endl;
*/
int isover=0;
int over_size=average_list_over.size();
for(int i=0;i<1040;i++){
	if(i>=40){
		if(i%2==1){
			for(int j=0;j<over_size;j++){
				if(i==average_list_over[j]){
					average_list_over_time.push_back(average_list[i]);
					isover=1;
				}
			}
			if(isover==0){
				average_list_proposal.push_back(average_list[i]);
			}
			isover=0;
		}
	}
}

int proposal_size=average_list_proposal.size();
int proposal_over_size=average_list_over_time.size();
std::cout<<"proposal_size:"<<proposal_size<<std::endl;
std::cout<<"proposal_over_size:"<<proposal_over_size<<std::endl;
std::cout<<std::endl;
for(int i=0;i<proposal_size;i++){
	std::cout<<average_list_proposal[i]<<std::endl;
}
std::cout<<std::endl;
std::cout<<std::endl;
for(int i=0;i<proposal_over_size;i++){
	std::cout<<average_list_over_time[i]<<std::endl;
}

//for(int i=0;i<400;i++){
//	if(i%2==1){
//		std::cout<<average_list[i]<<std::endl;
//	}
//}

    //average_list[l]=sum/5;

    //std::cout<<sum/5<<std::endl;







//




  Simulator::Destroy ();

  return 0;
}
