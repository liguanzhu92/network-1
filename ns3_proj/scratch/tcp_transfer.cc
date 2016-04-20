//
// Network topology
//
//           1Mb/s, 10ms       1Mb/s, 10ms       1Mb/s, 10ms
//       A-----------------B-----------------C-----------------D
//
//
// - Tracing of queues and packet receptions to file 
//   "tcp-large-transfer.tr"
// - pcap traces also generated in the following files
//   "tcp-large-transfer-$n-$i.pcap" where n and i represent node and interface
// numbers respectively
//  Usage (e.g.): ./waf --run tcp-large-transfer


#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCP_Transfer");

// The number of bytes to send in this simulation.
static const uint32_t totalTxBytes = 2000000;
static uint32_t currentTxBytes = 0;
// Perform series of 1040 byte writes (this is a multiple of 26 since
// we want to detect data splicing in the output stream)
static const uint32_t writeSize = 1040;
uint8_t data[writeSize];

// These are for starting the writing process, and handling the sending 
// socket's notification upcalls (events).  These two together more or less
// implement a sending "Application", although not a proper ns3::Application
// subclass.

void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);
static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd);

static void 
CwndTracer (uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO ("Moving cwnd from " << oldval << " to " << newval);
}

int
main (int argc, char *argv[])
{
  bool enableVerbose = false;
  if(argc == 2) {
    if(strcmp(argv[1], "-v") == 0) {
      enableVerbose = true;
    }
  }
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  if(enableVerbose) {
    LogComponentEnableAll(LOG_LEVEL_INFO);
  }

  // initialize the tx buffer.
  for(uint32_t i = 0; i < writeSize; ++i)
    {
      char m = toascii (97 + i % 26);
      data[i] = m;
    }

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpFixed::GetTypeId()));
  /* p2p nodes A--B */
  NodeContainer nodesAB;
  nodesAB.Create (2);

  /* p2p nodes B--C */
  NodeContainer nodesBC;
  nodesBC.Add (nodesAB.Get (1)); 
  nodesBC.Create (1);

  /* p2p nodes C--D */
  NodeContainer nodesCD;
  nodesCD.Add (nodesBC.Get (1));
  nodesCD.Create(1);

  /* create channel */
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  
  NetDeviceContainer devicesAB;
  devicesAB = pointToPoint.Install (nodesAB);
  NetDeviceContainer devicesBC;
  devicesBC = pointToPoint.Install (nodesBC);
  NetDeviceContainer devicesCD;
  devicesCD = pointToPoint.Install (nodesCD);

  pointToPoint.EnablePcapAll ("tcp");

  /* assign error rate to each devices */
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (1e-6));
  devicesAB.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  devicesCD.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  Ptr<RateErrorModel> em1 = CreateObject<RateErrorModel> ();
  em1->SetAttribute ("ErrorRate", DoubleValue (1e-5));
  devicesBC.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

  /* install stack and assign IP */
  InternetStackHelper stack;
  stack.Install (nodesAB);
  stack.Install (nodesBC.Get (1));
  stack.Install (nodesCD.Get (1));

  /* assign ip addresses */
  Ipv4AddressHelper addressAB;
  addressAB.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper addressBC;
  addressBC.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4AddressHelper addressCD;
  addressCD.SetBase ("10.1.3.0", "255.255.255.0");

  Ipv4InterfaceContainer interfacesAB = addressAB.Assign (devicesAB);
  Ipv4InterfaceContainer interfacesBC = addressBC.Assign (devicesBC);
  Ipv4InterfaceContainer interfacesCD = addressCD.Assign (devicesCD);

/*------Simulation---------------------------*/


  // Create a source to send packets from n0.  Instead of a full Application
  // and the helper APIs you might see in other example files, this example
  // will use sockets directly and register some socket callbacks as a sending
  // "Application".



  /* set up a server */
  uint16_t sinkPort = 8080;
  //Address sinkAddress (InetSocketAddress (interfacesCD.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodesCD.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (2000.));

  /* set up a client */
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodesAB.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket->Bind();
  // Trace changes to the congestion window
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));

  // ...and schedule the sending "Application"; This is similar to what an 
  // ns3::Application subclass would do internally.
  Simulator::ScheduleNow (&StartFlow, ns3TcpSocket,
                          interfacesCD.GetAddress (1), sinkPort);
  //sinkApps->SetStartTime (Seconds (1.));
  //sinkApps->SetStopTime (Seconds (200.));

  // One can toggle the comment for the following line on or off to see the
  // effects of finite send buffer modelling.  One can also change the size of
  // said buffer.

  //localSocket->SetAttribute("SndBufSize", UintegerValue(4096));

 //  tracing
  //Ask for ASCII and pcap traces of network traffic
  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-large-transfer.tr"));
  pointToPoint.EnablePcapAll ("tcp-large-transfer");

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("CwndChange.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  //PcapHelper pcapHelper;
  //Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", std::ios::out, PcapHelper::DLT_PPP);
  //devicesCD.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (2000));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


//begin implementation of sending "Application"
void StartFlow (Ptr<Socket> localSocket,
                Ipv4Address servAddress,
                uint16_t servPort)
{
  NS_LOG_LOGIC ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
  localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
  localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
  WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
  while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0) 
    {
      uint32_t left = totalTxBytes - currentTxBytes;
      uint32_t dataOffset = currentTxBytes % writeSize;
      uint32_t toWrite = writeSize - dataOffset;
      toWrite = std::min (toWrite, left);
      toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
      if(amountSent < 0)
        {
          // we will be called again when new tx space becomes available.
          return;
        }
      currentTxBytes += amountSent;
    }
  localSocket->Close ();
}

static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  // TODO let's see if we can add retransmitted package to another stream, basically use something like hashSet
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

