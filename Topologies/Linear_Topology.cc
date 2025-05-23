#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace ns3;
using namespace std;

// Global tracing variables
static ofstream logFile;
static uint32_t lastCwnd = 0;
static double prevThroughput = 0.0;
static Time prevTime;
static ApplicationContainer sinkApp;

FlowMonitorHelper flowmonhelper;
Ptr<FlowMonitor> monitor = flowmonhelper.InstallAll();

void CwndTracer(uint32_t oldCwnd, uint32_t newCwnd)
{
    lastCwnd = newCwnd;
}
void ThroughputTracer(Ptr<PacketSink> sink)
{
    double curTotalRx = sink->GetTotalRx() * 8.0 / 1e6; // Mbit
    Time curTime = Simulator::Now();
    double interval = (curTime - prevTime).GetSeconds();
    double throughput = 0.0;
    if (interval > 0.0)
    {
        throughput = (curTotalRx - prevThroughput) / interval;
    }

    logFile << fixed << setprecision(3)
            << "Time: " << curTime.GetSeconds() << " s, "
            << "Throughput: " << throughput << " Mbps, "
            << "cwnd: " << lastCwnd << " bytes\n";

    prevThroughput = curTotalRx;
    prevTime = curTime;

    Simulator::Schedule(MilliSeconds(100), &ThroughputTracer, sink);
}

int main(int argc, char *argv[])
{
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpLinuxReno::GetTypeId()));

    logFile.open("/Users/debar/Desktop/CN_LAB_PROJECT/log_stats.txt", ios::out);
    cout << fixed << setprecision(3);
    NodeContainer nodes;
    nodes.Create(10);
    InternetStackHelper stack;
    stack.Install(nodes);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("5p")));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    vector<Ipv4InterfaceContainer> interfaces;
    for (uint32_t i = 0; i < nodes.GetN() - 1; ++i)
    {
        NodeContainer pair(nodes.Get(i), nodes.Get(i + 1));
        auto ndc = p2p.Install(pair);
        interfaces.push_back(address.Assign(ndc));
        address.NewNetwork();
    }
    uint16_t port = 5000;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), port));
    sinkApp = sinkHelper.Install(NodeContainer(nodes.GetN()-1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(61.0));

    BulkSendHelper bulk("ns3::TcpSocketFactory",InetSocketAddress(interfaces.back().GetAddress(1), port));
    bulk.SetAttribute("MaxBytes", UintegerValue(0)); 
    auto senderApp = bulk.Install(nodes.Get(0));
    senderApp.Start(Seconds(1.0));
    senderApp.Stop(Seconds(61.0));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    prevTime = Seconds(1.1);
    prevThroughput = 0.0;

    Simulator::Schedule(Seconds(1.0001), []() {
        Config::ConnectWithoutContext(
            "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
            MakeCallback(&CwndTracer));
    });
    Simulator::Schedule(Seconds(1.1), &ThroughputTracer,DynamicCast<PacketSink>(sinkApp.Get(0)));
    Simulator::Stop(Seconds(61.0));
    Simulator::Run();
    
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonhelper.GetClassifier ());
    map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (auto &flow : stats)
    {
        FlowId flowId = flow.first;
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (flowId);
        FlowMonitor::FlowStats st = flow.second;
        cout << "Flow " << flowId << " (" << t.sourceAddress << " → " << t.destinationAddress << ")\n";
        cout << "  Tx Packets:   " << st.txPackets << "\n";
        cout << "  Rx Packets:   " << st.rxPackets << "\n";
        cout << "  Throughput:   "<< (st.rxBytes * 8.0 / (st.timeLastRxPacket.GetSeconds()  - st.timeFirstTxPacket.GetSeconds()) / 1e6)<< " Mbps\n";
        cout << "  Delay Sum:    " << st.delaySum.GetSeconds() << " s\n";
        cout << "  Jitter Sum:   " << st.jitterSum.GetSeconds() << " s\n\n";
}
    Simulator::Destroy();

    logFile.close();
    return 0;
}