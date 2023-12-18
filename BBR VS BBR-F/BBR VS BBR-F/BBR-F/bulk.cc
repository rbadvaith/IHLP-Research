#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

#define ENABLE_PCAP      true     
#define ENABLE_TRACE     true     
#define BIG_QUEUE        2000      
#define QUEUE_SIZE       100       
#define START_TIME       0.0       
#define STOP_TIME        5.0       
#define S_TO_R_BW        "1000Mbps" 
#define S_TO_R_DELAY     "5ms"
#define R_TO_C_BW        "15Mbps"  
#define R_TO_C_DELAY     "10ms"
#define PACKET_SIZE      500      


#define TCP_PROTOCOL     "ns3::TcpBbr"

NS_LOG_COMPONENT_DEFINE ("main");

int main (int argc, char *argv[]) {
  
  LogComponentEnable("main", LOG_LEVEL_INFO);
  LogComponentEnable("TcpBbr", LOG_LEVEL_INFO);

  Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                     StringValue(TCP_PROTOCOL));

  NS_LOG_INFO("TCP protocol: " << TCP_PROTOCOL);
  NS_LOG_INFO("Server to Router Bwdth: " << S_TO_R_BW);
  NS_LOG_INFO("Server to Router Delay: " << S_TO_R_DELAY);
  NS_LOG_INFO("Router to Client Bwdth: " << R_TO_C_BW);
  NS_LOG_INFO("Router to Client Delay: " << R_TO_C_DELAY);
  NS_LOG_INFO("Packet size (bytes): " << PACKET_SIZE);
  
  Config::SetDefault("ns3::TcpSocket::SegmentSize",
                     UintegerValue(PACKET_SIZE)); 

  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
   
  NS_LOG_INFO("Creating nodes.");
  NodeContainer nodes;  
  nodes.Create(3);

  NS_LOG_INFO("Creating channels.");
  NodeContainer n0_to_r = NodeContainer(nodes.Get(0), nodes.Get(1));
  NodeContainer r_to_n1 = NodeContainer(nodes.Get(1), nodes.Get(2));

  NS_LOG_INFO("Creating links.");

  int mtu = 1500;
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue (S_TO_R_BW));
  p2p.SetChannelAttribute("Delay", StringValue (S_TO_R_DELAY));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  NetDeviceContainer devices1 = p2p.Install(n0_to_r);

  p2p.SetDeviceAttribute("DataRate", StringValue (R_TO_C_BW));
  p2p.SetChannelAttribute("Delay", StringValue (R_TO_C_DELAY));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  NS_LOG_INFO("Router queue size: "<< QUEUE_SIZE);
  p2p.SetQueue("ns3::DropTailQueue",
               "Mode", StringValue ("QUEUE_MODE_PACKETS"),
               "MaxPackets", UintegerValue(QUEUE_SIZE));
  NetDeviceContainer devices2 = p2p.Install(r_to_n1);

  NS_LOG_INFO("Installing Internet stack.");
  InternetStackHelper internet;
  internet.Install(nodes);
  
  NS_LOG_INFO("Assigning IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign(devices1);

  ipv4.SetBase("191.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign(devices2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  NS_LOG_INFO("Creating applications.");
  NS_LOG_INFO("  Bulk send.");

  uint16_t port = 911;  

  BulkSendHelper source("ns3::TcpSocketFactory",
                        InetSocketAddress(i1i2.GetAddress(1), port));
  
  source.SetAttribute("MaxBytes", UintegerValue(0));
  source.SetAttribute("SendSize", UintegerValue(PACKET_SIZE));
  ApplicationContainer apps = source.Install(nodes.Get(0));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));

  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), port));
  apps = sink.Install(nodes.Get(2));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));
  Ptr<PacketSink> p_sink = DynamicCast<PacketSink> (apps.Get(0)); 

  if (ENABLE_TRACE) {
    NS_LOG_INFO("Enabling trace files.");
    AsciiTraceHelper ath;
    p2p.EnableAsciiAll(ath.CreateFileStream("trace.tr"));
  }  
  if (ENABLE_PCAP) {
    NS_LOG_INFO("Enabling pcap files.");
    p2p.EnablePcapAll("shark", true);
  }

  NS_LOG_INFO("Running simulation.");
  Simulator::Stop(Seconds(STOP_TIME));
  NS_LOG_INFO("Simulation time: [" << 
              START_TIME << "," <<
              STOP_TIME << "]");
  NS_LOG_INFO("---------------- Start -----------------------");
  Simulator::Run();
  NS_LOG_INFO("---------------- Stop ------------------------");
  

  NS_LOG_INFO("Total bytes received: " << p_sink->GetTotalRx());
  double tput = p_sink->GetTotalRx()*1.1 / (STOP_TIME - START_TIME);
  tput *= 8;          
  tput /= 1000000.0;  
  NS_LOG_INFO("Throughput: " << tput << " Mb/s");
  NS_LOG_INFO("Done.");

  Simulator::Destroy();
  return 0;
}
