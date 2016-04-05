#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCP_CHAIN");

class MyApp : public Application
{
public:
    MyApp ();
    virtual ~MyApp ();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId (void);
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleTx (void);
    void SendPacket (void);

    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    EventId         m_sendEvent;
    bool            m_running;
    uint32_t        m_packetsSent;
};
//application constructor
MyApp::MyApp ()
        : m_socket (0),
          m_peer (),
          m_packetSize (0),
          m_nPackets (0),
          m_dataRate (0),
          m_sendEvent (),
          m_running (false),
          m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
          .SetParent<Application> ()
          .SetGroupName ("Tutorial")
          .AddConstructor<MyApp> ()
  ;
  return tid;
}
// initialize application variables
void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}
// called to run the application
void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}
// for stop
void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
  {
    Simulator::Cancel (m_sendEvent);
  }

  if (m_socket)
  {
    m_socket->Close ();
  }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
  {
    ScheduleTx ();
  }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
  {
    Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
    m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
  }
}
//callbacks for congestion window updated
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

//show the dropped packages
static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

int
main (int argc, char *argv[])
{
  /*int enableVerbose = 0;
  if(argc == 2) {
    if(strcmp(argv[1], "-v") == 0) {
      enableVerbose = 1;
    }
  }
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  if(enableVerbose) {
    LogComponentEnableAll(LOG_LEVEL_INFO);
  }
   */
  
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
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
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
  em1->SetAttribute ("ErrorRate", DoubleValue (5e-5));
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

  /* set up a server */
  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (interfacesCD.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodesCD.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (200.));

  /* set up a client */
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodesAB.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("0.1Mbps"));
  nodesAB.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (200.));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("sixth2.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("sixth.pcap", std::ios::out, PcapHelper::DLT_PPP);
  devicesCD.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (200));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
