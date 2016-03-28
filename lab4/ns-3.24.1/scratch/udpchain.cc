/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UDPChain");

int
main (int argc, char *argv[])
{
	bool verbose = true;
	CommandLine cmd;
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
	cmd.Parse (argc,argv);
	if (verbose)
	{
		LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
		LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
	}

	NodeContainer nodes;
	nodes.Create (4);

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("1Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));

	NetDeviceContainer devices;
	devices = csma.Install (nodes);

	InternetStackHelper stack;
	stack.Install (nodes);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");

	Ipv4InterfaceContainer interfaces = address.Assign (devices);

	UdpEchoServerHelper echoServer (9);

	ApplicationContainer serverApps = echoServer.Install (nodes.Get (3));
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (10.0));

	UdpEchoClientHelper echoClient (interfaces.GetAddress (3), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  	ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  	clientApps.Start (Seconds (2.0));
  	clientApps.Stop (Seconds (10.0));

  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	csma.EnablePcap ("second", devices.Get (0), true);
	csma.EnablePcap ("second", devices.Get (1), true);
	csma.EnablePcap ("second", devices.Get (2), true);
	csma.EnablePcap ("second", devices.Get (3), true);

  	Simulator::Run ();
  	Simulator::Destroy ();
  return 0;
}