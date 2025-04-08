// Includes core functionalities like simulation time, event scheduling, logging, and command-line argument parsing
#include "ns3/core-module.h"
// Includes basic network components like Node, NetDevice, Packet, and helpers to create and manage nodes/devices
#include "ns3/network-module.h"
// Includes the Internet protocol stack (TCP/IP), IP address assignment, and routing functionalities
#include "ns3/internet-module.h"
// Includes the PointToPointHelper class to create point-to-point (wired) links between two nodes with defined bandwidth and delay
#include "ns3/point-to-point-module.h"
// Includes standard application-level traffic generators like UDP Echo Server/Client, OnOffApplication, PacketSink, etc.
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleNetwork");

int main(int argc, char *argv[]) {
    LogComponentEnable("SimpleNetwork", LOG_LEVEL_INFO);

    NS_LOG_INFO("Hello NS-3 Simulation");

  // Create 3 nodes: Host A, Router, Host B
  NodeContainer nodesAtoR;
  nodesAtoR.Create(2); // Host A (0), Router (1)

  NodeContainer nodesRtoB;
  nodesRtoB.Add(nodesAtoR.Get(1)); // Router
  nodesRtoB.Create(1); // Host B (2)

  // Create point-to-point links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  // Install network devices on the links
  NetDeviceContainer devAtoR = p2p.Install(nodesAtoR);
  NetDeviceContainer devRtoB = p2p.Install(nodesRtoB);

  // Install the Internet stack on all nodes
  InternetStackHelper stack;
  stack.Install(nodesAtoR);
  stack.Install(nodesRtoB.Get(1)); // Only install on Host B (Router already has it)

  // Assign IP addresses to the interfaces
  Ipv4AddressHelper address;

  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaceAtoR = address.Assign(devAtoR);

  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaceRtoB = address.Assign(devRtoB);

  // Set up routing tables automatically
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Install an Echo Server on Host B
  uint16_t port = 9;
  UdpEchoServerHelper echoServer(port);
  ApplicationContainer serverApp = echoServer.Install(nodesRtoB.Get(1));
  serverApp.Start(Seconds(1.0));
  serverApp.Stop(Seconds(10.0));

  // Install an Echo Client on Host A
  UdpEchoClientHelper echoClient(ifaceRtoB.GetAddress(1), port);
  echoClient.SetAttribute("MaxPackets", UintegerValue(1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApp = echoClient.Install(nodesAtoR.Get(0));
  clientApp.Start(Seconds(2.0));
  clientApp.Stop(Seconds(10.0));
    NS_LOG_INFO("Host A IP Address: " << ifaceAtoR.GetAddress(0));
NS_LOG_INFO("Router IP Address (A side): " << ifaceAtoR.GetAddress(1));
NS_LOG_INFO("Router IP Address (B side): " << ifaceRtoB.GetAddress(0));
NS_LOG_INFO("Host B IP Address: " << ifaceRtoB.GetAddress(1));

  // Run the simulation
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
