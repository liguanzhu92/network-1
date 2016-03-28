
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //LogComponentEnableAll(LOG_LEVEL_INFO);
  
  //p2p nodes A---B
  NodeContainer nodesAB;
  nodesAB.Create (2);

  //p2p nodes B--C
  NodeContainer nodesBC;
  nodesBC.Add (nodesAB.Get (1)); 
  nodesBC.Create (1);

  //p2p nodes C--D
  NodeContainer nodesCD;
  nodesCD.Add (nodesBC.Get (1));
  nodesCD.Create(1);


  //create channel
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  
  NetDeviceContainer devicesAB;
  devicesAB = pointToPoint.Install (nodesAB);
  NetDeviceContainer devicesBC;
  devicesBC = pointToPoint.Install (nodesBC);
  NetDeviceContainer devicesCD;
  devicesCD = pointToPoint.Install (nodesCD);

  pointToPoint.EnablePcapAll ("udp");

  //install stack and assign IP
  InternetStackHelper stack;
  stack.Install (nodesAB);
  stack.Install (nodesBC.Get (1));
  stack.Install (nodesCD.Get (1));

  Ipv4AddressHelper addressAB;
  addressAB.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper addressBC;
  addressBC.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4AddressHelper addressCD;
  addressCD.SetBase ("10.1.3.0", "255.255.255.0");

  Ipv4InterfaceContainer interfacesAB = addressAB.Assign (devicesAB);
  Ipv4InterfaceContainer interfacesBC = addressBC.Assign (devicesBC);
  Ipv4InterfaceContainer interfacesCD = addressCD.Assign (devicesCD);

  //echo server
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodesCD.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //echo client
  UdpEchoClientHelper echoClient (interfacesCD.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (nodesAB.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}