#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-flow-classifier.h"
#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpDetailedStats");

uint32_t latestCwnd = 0;

void CwndTracer(uint32_t oldCwnd, uint32_t newCwnd) {
    latestCwnd = newCwnd;
}

void CollectDetailedStats(Ptr<FlowMonitor> monitor, FlowMonitorHelper& flowHelper, Ipv4Address h1Addr, Ipv4Address h2Addr, std::ofstream& statsFile) {
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto stats = monitor->GetFlowStats();

    for (auto &flow : stats) {
        auto t = classifier->FindFlow(flow.first);
        if (t.sourceAddress == h1Addr && t.destinationAddress == h2Addr) {
            double durationSec = flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds();
            double throughputMbps = (flow.second.rxBytes * 8.0) / (durationSec * 1e6);
            double avgDelayMs = (flow.second.rxPackets > 0) ? (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) * 1000.0 : 0.0;
            uint32_t packetLoss = flow.second.txPackets - flow.second.rxPackets;

            std::stringstream ss;
            ss << "Flow " << t.sourceAddress << " → " << t.destinationAddress
               << " | Throughput: " << throughputMbps << " Mbps"
               << " | Avg Delay: " << avgDelayMs << " ms"
               << " | Packet Loss: " << packetLoss
               << " | CWND: " << latestCwnd;

            NS_LOG_INFO(ss.str());
            statsFile << ss.str() << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    LogComponentEnable("TcpDetailedStats", LOG_LEVEL_INFO);
    std::ofstream statsFile("/Users/debar/Desktop/stats.txt", std::ios::out | std::ios::app);

    std::srand(std::time(nullptr));

    for (int i = 0; i < 100; ++i) {
        latestCwnd = 0;
        NodeContainer hosts;
        hosts.Create(2);

        PointToPointHelper p2p;
        std::string rate = std::to_string(8 + std::rand() % 5) + "Mbps"; // 8-12Mbps
        std::string delay = std::to_string(5 + std::rand() % 6) + "ms";   // 5-10ms
        p2p.SetDeviceAttribute("DataRate", StringValue(rate));
        p2p.SetChannelAttribute("Delay", StringValue(delay));
        p2p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("100p"));

        NetDeviceContainer devices = p2p.Install(hosts);

        InternetStackHelper stack;
        stack.Install(hosts);

        Ipv4AddressHelper address;
        address.SetBase("10.0.0.0", "255.255.255.0");
        Ipv4InterfaceContainer interfaces = address.Assign(devices);
        Ipv4Address h1Addr = interfaces.GetAddress(0);
        Ipv4Address h2Addr = interfaces.GetAddress(1);

        uint16_t port = 5001;
        Address sinkAddr(InetSocketAddress(h2Addr, port));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddr);
        auto sinkApp = sinkHelper.Install(hosts.Get(1));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(60.0));

        BulkSendHelper sourceHelper("ns3::TcpSocketFactory", sinkAddr);
        sourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
        sourceHelper.SetAttribute("SendSize", UintegerValue(1024));
        ApplicationContainer clientApp = sourceHelper.Install(hosts.Get(0));
        clientApp.Start(Seconds(1.0));
        clientApp.Stop(Seconds(60.0));

        Simulator::Schedule(Seconds(1.1), [&]() {
            Ptr<BulkSendApplication> appPtr = DynamicCast<BulkSendApplication>(clientApp.Get(0));
            Ptr<Socket> socket = appPtr->GetSocket();
            if (socket) {
                socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndTracer));
            }
        });

        FlowMonitorHelper flowHelper;
        Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

        Simulator::Stop(Seconds(61.0));
        Simulator::Run();

        CollectDetailedStats(monitor, flowHelper, h1Addr, h2Addr, statsFile);
        Simulator::Destroy();
    }

    statsFile.close();
    return 0;
}
