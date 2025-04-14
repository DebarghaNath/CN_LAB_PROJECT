#include <iostream>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;

ofstream logFile;   
ofstream logFile1;  

NS_LOG_COMPONENT_DEFINE("TcpRenoExample");

FlowMonitorHelper g_flowmonHelper;
void CwndTracer(uint32_t oldCwnd, uint32_t newCwnd)
{
    logFile << Simulator::Now().GetSeconds() << "s\tCWnd: "
            << newCwnd << " KB" << std::endl;
}

void SetupCwndTracing(Ptr<Node> node)
{
    logFile.open("/Users/debar/Desktop/CN_LAB_PROJECT/cwnd.txt", ios::out | ios::app);  
    if (!logFile.is_open())
    {
        cerr << "Failed to open cwnd log file!" << std::endl;
        return;
    }

    if (node->GetId() == 0)
    {
        for (uint32_t i = 0; i < node->GetNApplications(); ++i)
        {
            Ptr<Application> app = node->GetApplication(i);
            Ptr<BulkSendApplication> bulkApp = DynamicCast<BulkSendApplication>(app);
            if (bulkApp)
            {
                Ptr<Socket> socket = bulkApp->GetSocket();
                if (socket)
                {
                    socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndTracer));
                }
            }
        }
    }
}


void PrintFlowStats(Ptr<FlowMonitor> monitor)
{
    logFile1.open("/Users/debar/Desktop/CN_LAB_PROJECT/log_stats.txt", ios::out | ios::app);  
    if (!logFile1.is_open())
    {
        cerr << "Failed to open flow stats log file!" << std::endl;
        return;
    }

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(g_flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (const auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        if (t.sourceAddress == "10.1.1.1") 
        {
            logFile1 << "Flow ID: " << flow.first << " (" 
                     << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            logFile1 << "  Tx Packets: " << flow.second.txPackets << "\n";
            logFile1 << "  Rx Packets: " << flow.second.rxPackets << "\n";
            logFile1 << "  Lost Packets: " << (flow.second.txPackets - flow.second.rxPackets) << "\n";
            logFile1 << "  Delay: " << flow.second.delaySum.GetSeconds() << " s\n";

            double jitter = (flow.second.rxPackets > 0) ? 
                            (flow.second.jitterSum.GetSeconds() / flow.second.rxPackets) : 0.0;
            logFile1 << "  Jitter: " << jitter << " s\n";

            double duration = flow.second.timeLastRxPacket.GetSeconds() - 
                              flow.second.timeFirstTxPacket.GetSeconds();
            double throughput = (duration > 0) ? 
                                (flow.second.rxBytes * 8.0 / duration) / 1e6 : 0.0;
            logFile1 << "  Throughput: " << throughput << " Mbps\n\n";
        }
    }

    logFile1.close();  
    Simulator::Schedule(Seconds(1.0), &PrintFlowStats, monitor);  
}

int main(int argc, char *argv[])
{

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
    LogComponentDisableAll(LOG_LEVEL_ALL);
    NodeContainer nodes;
    nodes.Create(3);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    NetDeviceContainer dev1 = p2p.Install(NodeContainer(nodes.Get(0), nodes.Get(1)));
    NetDeviceContainer dev2 = p2p.Install(NodeContainer(nodes.Get(1), nodes.Get(2)));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer intf1 = address.Assign(dev1);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer intf2 = address.Assign(dev2);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 8080;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(2));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(20.0));

    BulkSendHelper bulkSend("ns3::TcpSocketFactory", InetSocketAddress(intf2.GetAddress(1), port));
    bulkSend.SetAttribute("MaxBytes",  UintegerValue(0)); 
    ApplicationContainer clientApps = bulkSend.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(20.0));

    Ptr<FlowMonitor> monitor = g_flowmonHelper.InstallAll();

    Simulator::Schedule(Seconds(2.1), &SetupCwndTracing, nodes.Get(0));

    Simulator::Schedule(Seconds(3.0), &PrintFlowStats, monitor);

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
