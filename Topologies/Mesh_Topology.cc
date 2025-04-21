#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-linux-reno.h"
#include <iomanip>
#include <fstream>

using namespace ns3;
using namespace std;

static ofstream logFile;
static uint32_t lastCwnd = 0;
static double prevThroughput = 0.0;
static Time prevTime;
static ApplicationContainer sinkApp;

void CwndTracer(uint32_t oldCwnd, uint32_t newCwnd)
{
    lastCwnd = newCwnd;
}
void ThroughputTracer(Ptr<PacketSink> sink)
{
    cout<<"Throughput"<<endl;
    double curTotalRx = sink->GetTotalRx() * 8.0 / 1e6; 
    Time curTime = Simulator::Now();
    double interval = (curTime - prevTime).GetSeconds();
    double throughput = 0.0;
    if (interval > 0.0)
    {
        throughput = (curTotalRx - prevThroughput) / interval;
    }

    logFile << "Time: " << curTime.GetSeconds() << " s, Throughput: "
    << throughput << " Mbps, cwnd: "
    << lastCwnd << " bytes " << endl;

    prevThroughput = curTotalRx;
    prevTime = curTime;

    Simulator::Schedule(MilliSeconds(100), &ThroughputTracer, sink);
}

int main(int argc, char *argv[])
{

    cout<<"ok"<<endl;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpLinuxReno::GetTypeId()));
    logFile.open("/Users/debar/Desktop/CN_LAB_PROJECT/log_stats.txt", ios::out);cout << fixed << setprecision (3);

    NodeContainer nodes;
    nodes.Create(10);
    InternetStackHelper stack;
    stack.Install(nodes);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("5p")));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    vector<Ipv4InterfaceContainer> interfaces;
    uint32_t n = nodes.GetN();
    for (uint32_t parent = 0; parent < n; ++parent)
    { 
      for (uint32_t child=parent+1;child< n;child++)
      {
       
            NodeContainer pair(nodes.Get(parent), nodes.Get(child));
            auto ndc = p2p.Install(pair);
            interfaces.push_back(address.Assign(ndc));
            address.NewNetwork(); 
      }   
    }

    uint16_t port = 5000;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), port));
    sinkApp = sinkHelper.Install(nodes.Get(5));

    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(61.0));

    BulkSendHelper bulk("ns3::TcpSocketFactory",
    InetSocketAddress(interfaces[4].GetAddress(1), port));
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

    Simulator::Schedule(Seconds(1.1), &ThroughputTracer,
                        DynamicCast<PacketSink>(sinkApp.Get(0)));

    Simulator::Stop(Seconds(61.0));
    Simulator::Run();
    Simulator::Destroy();

    logFile.close();
    return 0;
}
