// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

#define NUM_CONSUMERS 2
#define NUM_ROUTERS 2
#define NUM_PRODUCER 1

#define DELAY_OUTPUT_FILE_NAME "simple-delay"
#define RATE_OUTPUT_FILE_NAME "simple-rate"
#define SIMULATION_DURATION 10 // real-time?

#include "../apps/accounting-consumer.hpp"
#include "../apps/ndn-consumer-cbr.hpp"

void
ReceivedMeaningfulContent(ns3::Ptr<ns3::ndn::AccountingConsumer> consumer)
{
    std::cout << "CALLBACK" << std::endl;
    // for(std::vector<ns3::ndn::NameTime*>::iterator it = consumer->rtts.begin(); it != consumer->rtts.end(); ++it) {
    //     ns3::ndn::NameTime *nt = *it;
    //     std::cout << "\t" << nt->name << ", RTT: " << nt->rtt << std::endl;
    // }
}

namespace ns3 {
  ofstream delayFile;

  void
  ForwardingDelay(ns3::Time eventTime, float delay)
  {
    delayFile << eventTime.GetNanoSeconds() << "\t" << delay * 1000000000 << "\n";
  }

  // void
  //   ReceivedMeaningfulContent (std::string context, Ptr<ns3::ndn::ContentObject const> content, ns3::Time stoppingTime)
  //   {
  //     stoppingMicroSeconds[stoppedConsumerCount] += stoppingTime.GetMicroSeconds();
  //     stoppedConsumerCount++;
  //   }

  int
  run(int argc, char* argv[])
  {
    delayFile.open(DELAY_OUTPUT_FILE_NAME);

    // setting default parameters for PointToPoint links and channels
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1000Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("4294967295"));

    // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Creating nodes
    NodeContainer nodes;
    nodes.Create(NUM_CONSUMERS + NUM_ROUTERS + NUM_PRODUCER);

    // Connecting nodes using two links
    PointToPointHelper p2p;
    for (int i = 0; i < NUM_CONSUMERS; i++) {
      p2p.Install(nodes.Get (i), nodes.Get (NUM_CONSUMERS + 0)); // Cr <-> R
    }
    for (int i = 0; i < NUM_ROUTERS - 1; i++) {
      p2p.Install(nodes.Get (NUM_CONSUMERS + i), nodes.Get (NUM_CONSUMERS + i + 1)); // Ri <-> R(i+1)
    }
    p2p.Install(nodes.Get (NUM_CONSUMERS + NUM_ROUTERS - 1), nodes.Get (NUM_CONSUMERS + NUM_ROUTERS + 0)); // last router <-> P

    // Install NDN stack without cache on consumers
    ndn::StackHelper ndnHelperNoCache;
    ndnHelperNoCache.SetDefaultRoutes(true);
    ndnHelperNoCache.SetOldContentStore("ns3::ndn::cs::Nocache"); // no cache
    for (int i = 0; i < NUM_CONSUMERS; i++) {
      ndnHelperNoCache.Install(nodes.Get(i));
    }
    ndnHelperNoCache.Install(nodes.Get(NUM_CONSUMERS + NUM_ROUTERS + 0));

    // Install NDN stack with cache
    ndn::StackHelper ndnHelperWithCache;
    ndnHelperWithCache.SetDefaultRoutes(true);
    ndnHelperWithCache.SetOldContentStore("ns3::ndn::cs::Freshness::Lru", "MaxSize", "10000"); // no max size
    for (int i = 0; i < NUM_ROUTERS; i++) {
      ndnHelperWithCache.InstallWithCallback(nodes.Get(NUM_CONSUMERS + i), (size_t)&ForwardingDelay, i, true);
    }
    //ndnHelperWithCache.InstallWithCallback(nodes.Get(NUM_CONSUMERS + 1), (size_t)&ForwardingDelay, 1, true);
    //ndnHelperWithCache.InstallWithCallback(nodes.Get(NUM_CONSUMERS + 2), (size_t)&ForwardingDelay, 2, true);

    ndn::AppHelper consumerHelperHonest("ns3::ndn::AccountingConsumer");
    consumerHelperHonest.SetAttribute("Frequency", StringValue("1")); // 10 interests a second
    consumerHelperHonest.SetAttribute("Randomize", StringValue("uniform"));
    consumerHelperHonest.SetAttribute("StartSeq", IntegerValue(0));
    consumerHelperHonest.SetPrefix("/prefix/A/"); // + std::to_string(i));
    for (int i = 0; i < NUM_CONSUMERS; i++) {
      consumerHelperHonest.SetAttribute("ConsumerID", IntegerValue(i));
      ApplicationContainer consumer = consumerHelperHonest.Install(nodes.Get(i));

      std::ostringstream node_id;
      node_id << i;
      Config::ConnectWithoutContext("/NodeList/" + node_id.str() + "/ApplicationList/0/ReceivedMeaningfulContent", MakeCallback(ReceivedMeaningfulContent));
    //   ConnectWithoutContext
    }

    // Producer
    // Producer will reply to all requests starting with /prefix/A
    ndn::AppHelper producerHelper("ns3::ndn::AccountingProducer");
    producerHelper.SetPrefix("/prefix/A/");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.SetAttribute("Freshness", TimeValue(Seconds(SIMULATION_DURATION)));
    producerHelper.Install(nodes.Get(NUM_CONSUMERS + NUM_ROUTERS + 0));

    // Traces
    ndn::L3RateTracer::InstallAll(RATE_OUTPUT_FILE_NAME, Seconds(1.0));

    Simulator::Stop(Seconds(SIMULATION_DURATION));

    Simulator::Run();
    Simulator::Destroy();

    delayFile.close();
    return 0;
  }

} // namespace ns3

int
main(int argc, char* argv[])
{
  //LogComponentEnable ("nfd.Forwarder", ns3::LOG_LEVEL_DEBUG);
  return ns3::run(argc, argv);
}
