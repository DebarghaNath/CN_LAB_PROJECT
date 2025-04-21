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
const double eps_a = 1, eps_b = 10, eps_c = 1, eps_d = 10; 

// For gradient descent
const double learning_rate = 3;

// number of iterations
const int iterations = 100;

// Loss function / Congestion metric
const double alpha = 0, Beta = 1, Gamma = 0; // Coefficients
const double scale_1 = 1, scale_2 = 1, scale_3 = 1; // Normalization

// During simulation
const int runtime = 20;       // Seconds for which the simulation runs
ApplicationContainer sinkApp;
bool slow_start;              // Checks if it is still in slow start phase
double Throughput;            // Throughput after slow start

double getThroughput(){
    return DynamicCast<PacketSink>(sinkApp.Get(0))->GetTotalRx() / 1e6;
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
                if(slow_start){
                    slow_start = false;
                    Throughput = -getThroughput(); // Remove the slow start portion
                }
                // Additive increase
                double cwndInSegments = tcb->m_cWnd.Get() / tcb->m_segmentSize;
                double delta = max(1.0,a/(b+cwndInSegments) * tcb->m_segmentSize);
                tcb->m_cWnd += static_cast<uint32_t>(delta);
            }
            // lastCwnd = static_cast<uint32_t>(tcb->m_cWnd.Get() / tcb->m_segmentSize);
        }
        
        // On loss
        void CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState) override {
            TcpNewReno::CongestionStateSet(tcb, newState);
            if (newState == TcpSocketState::CA_LOSS) {
                tcb->m_cWnd = static_cast<uint32_t>(max(1.0,tcb->m_cWnd.Get() * c + d * tcb->m_segmentSize));
            }
            // lastCwnd = static_cast<uint32_t>(tcb->m_cWnd.Get() / tcb->m_segmentSize);
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

double run(){
    // Set TCP variant
    CongestionProtocol("Custom");

    slow_start = true;

    NodeContainer nodes;
    nodes.Create(10);
    InternetStackHelper stack;
    stack.Install(nodes);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("10p")));

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

    int port = 5000;

    // Install sink on receiver
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), port));
    sinkApp = sinkHelper.Install(nodes.Get(nodes.GetN()-1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(runtime));
    
    // Delay BulkSender installation and start
    Simulator::Schedule(Seconds(0.000001), [&]() mutable {
        BulkSendHelper bulk("ns3::TcpSocketFactory",InetSocketAddress(interfaces.back().GetAddress(1), port));
        bulk.SetAttribute("MaxBytes", UintegerValue(0)); 
        auto senderApp = bulk.Install(nodes.Get(0));
        senderApp.Start(Seconds(1.0));
        senderApp.Stop(Seconds(runtime));
    });
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(runtime));
    Simulator::Run();

    // Calcluate and return Congestion Metric
    Throughput += getThroughput();
    double CM = alpha*0/scale_1 - Beta*Throughput/scale_2 + Gamma*0/scale_3;

    Simulator::Destroy();

    return CM;
}

int main(int argc, char *argv[]) {

    cout<<"\n\n ---- START ---- \n"<<endl;

    cout << fixed << setprecision(6);

    cout << 0 << " -> " << " a: " << a << ", b: " << b << ", c: " << c << ", d: " << d
         << ", CM: " << -run() << endl;

    for(int i=1;i<=iterations;i++){
        double CM = run();

        a += eps_a;
        double CM_a = run();
        double dcm_da = (CM_a - CM)/eps_a;
        a -= eps_a;
        
        b += eps_b;
        double CM_b = run();
        double dcm_db = (CM_b - CM)/eps_b;
        b -= eps_b;
        
        c += eps_c;
        double CM_c = run();
        double dcm_dc = (CM_c - CM)/eps_c;
        c -= eps_c;
        
        d += eps_d;
        double CM_d = run();
        double dcm_dd = (CM_d - CM)/eps_d;
        d -= eps_d;

        // Update Parameters
        a -= learning_rate * dcm_da;
        a = max(0.1,a);

        b -= learning_rate * dcm_db;

        c -= learning_rate * dcm_dc;
        c = max(0.001,c);
        c = min(1.0,c);

        d -= learning_rate * dcm_dd;

        cout << i << " -> " << " a: " << a << ", b: " << b << ", c: " << c << ", d: " << d
             << ", CM: " << -CM << endl;
    }

    return 0;
}
