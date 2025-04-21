#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/tcp-cubic.h"
#include "ns3/tcp-vegas.h"

#include <iomanip>
#include <fstream>

using namespace ns3;
using namespace std;

// Optimization parameters
double a = 1, b = 0, c = 0.5 , d = 0;
// For gradient calculation
double dcm_da,dcm_db,dcm_dc,dcm_dd;
double learning_rate = 0.1;
double lastTotalTP = 0;
// Loss function / Congestion metric
double alpha = 0, beta = 1, Gamma = 0; // Coefficients
double scale_1, scale_2, scale_3; // Normalization
double CM;

ofstream logFile;
uint32_t lastCwnd = 0;
double prevThroughput = 0;
Time prevTime = Simulator::Now();

ApplicationContainer sinkApp;

void ThroughputTracer(Ptr<Application> app) {
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(app);
    double curThroughput = (sink->GetTotalRx() * 8.0) / 1e6; // Mb
    Time curTime = Simulator::Now();
    double throughput = (curThroughput - prevThroughput)/(curTime.GetSeconds()-prevTime.GetSeconds()); // Mbps

    prevThroughput = curThroughput;
    prevTime = curTime;

    logFile << "Time: " << curTime.GetSeconds() << " s, Throughput: "
            << throughput << " Mbps, cwnd: "
            << lastCwnd << " segments " << endl;

    Simulator::Schedule(MilliSeconds(10), &ThroughputTracer, app); // Every 10ms
}

void gradient(){
    double curTotalTP = prevThroughput;
    double TP1 = curTotalTP - lastTotalTP;

}

void UpdateParameters(){
    // gradient();

    // lastTotalTP = prevThroughput;

    // a -= learning_rate * dcm_da;
    // a = max(0.1,a);
    // b -= learning_rate * dcm_db;
    // c -= learning_rate * dcm_dc;
    // c = max(0.001,c);
    // c = min(1.0,c);
    // d -= learning_rate * dcm_dd;
    
    // logFile << "Time: " << curTime.GetMinutes() << " min, a: "<< a << ", b: " << b
    //         << ", c: " <<c << ", d: " << d << endl;

    // Simulator::Schedule(Minutes(0.5), &UpdateParameters); // Every 30sec
}

class CustomTcp : public TcpNewReno {
    public:
        static TypeId GetTypeId(void) {
            static TypeId tid = TypeId("CustomTcp")
            .SetParent<TcpNewReno>()
            .SetGroupName("Internet")
            .AddConstructor<CustomTcp>();
            return tid;
        }
        
        CustomTcp() {}
        CustomTcp(const CustomTcp& sock) : TcpNewReno(sock) {}
    
        Ptr<TcpCongestionOps> Fork() override {
            return CopyObject<CustomTcp>(this);
        }
        
        void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override {
            if (tcb->m_cWnd < tcb->m_ssThresh) {
                // Slow start
                TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
            } 
            else{
                // Additive increase
                double cwndInSegments = tcb->m_cWnd.Get() / tcb->m_segmentSize;
                double delta = max(1.0,a/(b+cwndInSegments) * tcb->m_segmentSize);
                tcb->m_cWnd += static_cast<uint32_t>(delta);
            }
            lastCwnd = static_cast<uint32_t>(tcb->m_cWnd.Get() / tcb->m_segmentSize);
        }
        
        // On loss
        void CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState) override {
            TcpNewReno::CongestionStateSet(tcb, newState);
            if (newState == TcpSocketState::CA_RECOVERY) {
                tcb->m_cWnd = static_cast<uint32_t>(max(1.0,tcb->m_cWnd.Get() * c + d * tcb->segmentSize));
            }
            lastCwnd = static_cast<uint32_t>(tcb->m_cWnd.Get() / tcb->m_segmentSize);
        }
};
    
void CongestionProtocol(string s = ""){
    if(s=="Cubic")
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpCubic::GetTypeId()));
    else if(s=="Vegas")
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
    else if(s=="Custom")
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(CustomTcp::GetTypeId()));
}

int main(int argc, char *argv[]) {

    cout<<"\n\n ---- START ---- \n"<<endl;

    logFile.open("Project/log_stats.txt", ios::out);

    cout << fixed << setprecision(3);
    
    // Set TCP variant
    CongestionProtocol("Custom");

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps")); // Large enough for no bottleneck
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    NetDeviceContainer devices = p2p.Install(nodes);

    // Setting the max queue size (100 packets in this case)
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("100p"))); 
    

    // Increase buffer size
    // Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20)); // 1 MB
    // Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20)); // 1 MB
    
    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    int port = 5000;

    // Install sink on receiver
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(61.0));
    
    // Delay BulkSender installation and start
    Simulator::Schedule(Seconds(0.000001), [&]() mutable {
        BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
        bulk.SetAttribute("MaxBytes", UintegerValue(0)); // Unlimited
        ApplicationContainer senderApp = bulk.Install(nodes.Get(0));
        senderApp.Start(Seconds(1.0));  // Safe to start now
        senderApp.Stop(Seconds(61.0));
    });

    // Start throughput measurement
    Simulator::Schedule(Seconds(1.0), &ThroughputTracer, sinkApp.Get(0));
    
    // Update parameters every 30 sec
    Simulator::Schedule(Minutes(0.5), &UpdateParameters);

    Simulator::Stop(Minutes(1));
    Simulator::Run();
    Simulator::Destroy();

    logFile.close();
    return 0;
}
