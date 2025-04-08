#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstSim");  

int main(int argc, char *argv[]) {
  LogComponentEnable("FirstSim", LOG_LEVEL_INFO);

  NS_LOG_INFO("Hello NS-3 Simulation");
  return 0;
}
