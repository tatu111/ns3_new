/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"

//追加
#include "simulation-proposal.h"
#include "ns3/olsr-routing-protocol.h"
#include <random>
#include "ns3/ipv4-list-routing.h"
//

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimulationProposal");

NS_OBJECT_ENSURE_REGISTERED (SimulationProposal);

TypeId
SimulationProposal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationProposal")
    .SetParent<Application> ()
	.SetParent<olsr::RoutingProtocol> ()
    .SetGroupName("Applications")
    .AddConstructor<SimulationProposal> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   DataRateValue (DataRate ("500kb/s")),
                   MakeDataRateAccessor (&SimulationProposal::m_cbrRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&SimulationProposal::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&SimulationProposal::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Local",
                   "The Address on which to bind the socket. If not set, it is generated automatically.",
                   AddressValue (),
                   MakeAddressAccessor (&SimulationProposal::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&SimulationProposal::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&SimulationProposal::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (1000000),
                   MakeUintegerAccessor (&SimulationProposal::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&SimulationProposal::m_tid),
                   // This should check for SocketFactory as a parent
                   MakeTypeIdChecker ())
    .AddAttribute ("EnableSeqTsSizeHeader",
                   "Enable use of SeqTsSizeHeader for sequence number and timestamp",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationProposal::m_enableSeqTsSizeHeader),
                   MakeBooleanChecker ())
//    .AddTraceSource ("Tx", "A new packet is created and is sent",
//                     MakeTraceSourceAccessor (&SimulationProposal::m_txTrace),
//                     "ns3::Packet::TracedCallback")
//    .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
//                     MakeTraceSourceAccessor (&SimulationProposal::m_txTraceWithAddresses),
//                     "ns3::Packet::TwoAddressTracedCallback")
//    .AddTraceSource ("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
//                     MakeTraceSourceAccessor (&SimulationProposal::m_txTraceWithSeqTsSize),
//                     "ns3::PacketSink::SeqTsSizeCallback")
//	.AddTraceSource ("Rx",
//	                "A packet has been received",
//					  MakeTraceSourceAccessor (&SimulationProposal::m_rxTrace),
//					  "ns3::Packet::AddressTracedCallback")
//	.AddTraceSource ("RxWithAddresses", "A packet has been received",
//			         MakeTraceSourceAccessor (&SimulationProposal::m_rxTraceWithAddresses),
//					 "ns3::Packet::TwoAddressTracedCallback")
//    .AddTraceSource ("RxWithSeqTsSize",
//    		         "A packet with SeqTsSize header has been received",
//					  MakeTraceSourceAccessor (&SimulationProposal::m_rxTraceWithSeqTsSize),
//					  "ns3::PacketSink::SeqTsSizeCallback")
  ;
  return tid;
}


SimulationProposal::SimulationProposal ()
  : m_socket_local (0),
    m_connected (false),
    m_residualBits (0),
    m_lastStartTime (Seconds (0)),
    m_totBytes (0),
    m_unsentPacket (0)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

SimulationProposal::~SimulationProposal()
{
  NS_LOG_FUNCTION (this);
}

void
SimulationProposal::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
SimulationProposal::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket_local;
}

int64_t
SimulationProposal::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
  return 2;
}

uint64_t
SimulationProposal::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}


Ptr<Socket>
SimulationProposal::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
SimulationProposal::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}


void
SimulationProposal::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  m_socket = 0;
  m_socket_local = 0;
  m_unsentPacket = 0;
  m_socketList.clear ();
  // chain up
  Application::DoDispose ();
}

// Application Methods
void SimulationProposal::StartApplication () // Called at time specified by Start
{


  Ptr<Node> node = GetNode();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  Ptr<Ipv4RoutingProtocol> ipv4_proto = ipv4->GetRoutingProtocol ();
  Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (ipv4_proto);
  if (list)
  {
	  int16_t priority;
	  Ptr<Ipv4RoutingProtocol> listProto;
	  Ptr<olsr::RoutingProtocol> protocol;
	  for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
	  {
		  listProto = list->GetRoutingProtocol (i, priority);
         protocol = DynamicCast<olsr::RoutingProtocol> (listProto);
         if (protocol){
        	 if (protocol->GetThresholdFlag() == 1)
        	 {
        		 std::cout<<"ok"<<std::endl;
        		 if(protocol->GetResourceThreshold().size()==0)
        		 {
        			 std::cout<<"NULL"<<std::endl;
        		 }

//        	 int port = (rnd()%(RANDOM_MAX - RANDOM_MIN + 1)) + RANDOM_MIN;
//
//
//        	 m_peer = InetSocketAddress ("10.1.1.1", port);


				 //listen
				 // Create the socket if not already
				 if(node->GetId()==0){
					 if (!m_socket)
					 {
						 m_socket = Socket::CreateSocket (node, m_tid);
	                if (m_socket->Bind (m_peer) == -1)
	                   {
	        			 	NS_FATAL_ERROR ("Failed to bind socket");
	                   }
					   m_socket->Listen ();

					   if (addressUtils::IsMulticast (m_peer))
						  {
						   Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
						   if (udpSocket)
								{
							   // equivalent to setsockopt (MCAST_JOIN_GROUP)
						   udpSocket->MulticastJoinGroup (0, m_peer);
								}
						   else
								{
							   NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
								}
						  }
					 }



					 m_socket->SetAllowBroadcast (true);
					 m_socket->SetRecvCallback (MakeCallback (&SimulationProposal::HandleRead, this));
					 m_socket->SetAcceptCallback (
					   MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
					   MakeCallback (&SimulationProposal::HandleAccept, this));
					 m_socket->SetCloseCallbacks (
					   MakeCallback (&SimulationProposal::HandlePeerClose, this),
					   MakeCallback (&SimulationProposal::HandlePeerError, this));

					 protocol->threshold_flag = 0;




					 }
					 else
					 {
						 std::map<Ptr<Node>, Address> threshold = protocol->GetResourceThreshold();
						 std::map<Ptr<Node>, Address>::iterator it;
						 it = threshold.find(node);
						 m_resource_sum = protocol->m_resource_sum[node->GetId()];
						 if(it != threshold.end()){
							 {
								 std::cout<<node->GetId()<<std::endl;
							 m_local = threshold[node];
							  //send
							 // Create the socket if not already
							 if (!m_socket_local)
								 {
								 m_socket_local = Socket::CreateSocket (node, m_tid);
								 int ret = -1;

								 if (! m_local.IsInvalid())
									 {
									 NS_ABORT_MSG_IF ((Inet6SocketAddress::IsMatchingType (m_peer) && InetSocketAddress::IsMatchingType (m_local)) ||
														   (InetSocketAddress::IsMatchingType (m_peer) && Inet6SocketAddress::IsMatchingType (m_local)),
														   "Incompatible peer and local address IP version");
									 ret = m_socket_local->Bind (m_local);
									 }
								 else
									 {
									 if (Inet6SocketAddress::IsMatchingType (m_peer))
											{
										 ret = m_socket_local->Bind6 ();
											}
									 else if (InetSocketAddress::IsMatchingType (m_peer) ||
												   PacketSocketAddress::IsMatchingType (m_peer))
											{
										 ret = m_socket_local->Bind ();
											}
									 }

									  if (ret == -1)
											{
										  NS_FATAL_ERROR ("Failed to bind socket");
											}

									  m_socket_local->Connect (m_peer);
									  m_socket_local->SetAllowBroadcast (true);
//									  m_socket_local->ShutdownRecv ();

									  m_socket_local->SetConnectCallback (
										MakeCallback (&SimulationProposal::ConnectionSucceeded, this),
										MakeCallback (&SimulationProposal::ConnectionFailed, this));
								 }
							 m_cbrRateFailSafe = m_cbrRate;

							 // Insure no pending event
//							 CancelEvents ();
							 // If we are not yet connected, there is nothing to do here
							 // The ConnectionComplete upcall will start timers at that time
							 //if (!m_connected) return;
							 ScheduleStartEvent ();



							 }
						 }
					 }
        	 }
           }
	  }
  }



  //タスクの要求の発生
  std::random_device seed_gen;
  std::default_random_engine engine(seed_gen());

  std::exponential_distribution<> dist(1.0);

  for (int n = 0; n < 1000; n++) {
      // 指数分布で乱数を生成する
     Time task_interval = Seconds(dist(engine));
   }


//  if (list)
//    {
//  	  int16_t priority2;
//  	  Ptr<Ipv4RoutingProtocol> listProto2;
//  	  Ptr<olsr::RoutingProtocol> protocol2;
//  	  for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
//  	  {
//  		  listProto2 = list->GetRoutingProtocol (i, priority2);
//         protocol2 = DynamicCast<olsr::RoutingProtocol> (listProto2);
//           if (protocol2){
//        	   sink_grasping.insert(std::make_pair(node->GetId(), protocol2->m_resource_sum[node->GetId()]));
//           }
//  	  }
//    }




}

void SimulationProposal::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();

  //listen
  while(!m_socketList.empty ()) //these are accepted sockets, close them
      {
        Ptr<Socket> acceptedSocket = m_socketList.front ();
        m_socketList.pop_front ();
        acceptedSocket->Close ();
      }
    if (m_socket)
      {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      }

  //send
  if(m_socket_local != 0)
    {
      m_socket_local->Close ();
    }
  else
    {
      NS_LOG_WARN ("simulation_proposal found null socket to close in StopApplication");
    }
}

void SimulationProposal::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
  // Canceling events may cause discontinuity in sequence number if the
  // SeqTsSizeHeader is header, and m_unsentPacket is true
  if (m_unsentPacket)
    {
      NS_LOG_DEBUG ("Discarding cached packet upon CancelEvents ()");
    }
  m_unsentPacket = 0;
}

// Event handlers
void SimulationProposal::StartSending ()
{
  NS_LOG_FUNCTION (this);
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();  // Schedule the send packet event
  ScheduleStopEvent ();
}

void SimulationProposal::StopSending ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();

  ScheduleStartEvent ();
}

// Private helpers
void SimulationProposal::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);


  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
	  if(send_count<1){
		  m_residualBits = 0;
		  NS_ABORT_MSG_IF (m_residualBits > m_pktSize * 8, "Calculation to compute next send time will overflow");
		  uint32_t bits = m_pktSize * 8 - m_residualBits;
		  NS_LOG_LOGIC ("bits = " << bits);
		  Time nextTime (Seconds (bits /
								  static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
		  NS_LOG_LOGIC ("nextTime = " << nextTime.As (Time::S));
		  m_sendEvent = Simulator::Schedule (nextTime,
											 &SimulationProposal::SendPacket, this);
		  send_count += 1;
	  }

    }
  else
    { // All done, cancel any pending events

      StopApplication ();
    }
}

void SimulationProposal::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  send_count = 0;

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval.As (Time::S));
  m_startStopEvent = Simulator::Schedule (offInterval, &SimulationProposal::StartSending, this);
}

void SimulationProposal::ScheduleStopEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval.As (Time::S));
  m_startStopEvent = Simulator::Schedule (onInterval, &SimulationProposal::StopSending, this);
}


void SimulationProposal::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());




  Ptr<Node> node = GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

    Ptr<Ipv4RoutingProtocol> ipv4_proto = ipv4->GetRoutingProtocol ();
    Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (ipv4_proto);
    if (list)
    {
  	  int16_t priority;
  	  Ptr<Ipv4RoutingProtocol> listProto;
  	  Ptr<olsr::RoutingProtocol> protocol;
  	  for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
  	  {
  		  listProto = list->GetRoutingProtocol (i, priority);
           protocol = DynamicCast<olsr::RoutingProtocol> (listProto);
           //NS_ASSERT (listOlsr);
           if (protocol){
          	 if (protocol->GetThresholdFlag() == 1)
          	 {

  				 if(protocol->GetResourceThreshold().size()!=0){

  						 std::map<Ptr<Node>, Address> threshold = protocol->GetResourceThreshold();
  						 m_local = threshold[node];
  						 //send
  						 // Create the socket if not already
  						 if (!m_socket_local)
  						 {
  							 m_socket_local = Socket::CreateSocket (node, m_tid);
  							 int ret = -1;
  							 if (! m_local.IsInvalid())
  							 {
  								 NS_ABORT_MSG_IF ((Inet6SocketAddress::IsMatchingType (m_peer) && InetSocketAddress::IsMatchingType (m_local)) ||
  												   (InetSocketAddress::IsMatchingType (m_peer) && Inet6SocketAddress::IsMatchingType (m_local)),
  													"Incompatible peer and local address IP version");
  								 ret = m_socket_local->Bind (m_local);
  							 }
  							 else
  							 {
  								 if (Inet6SocketAddress::IsMatchingType (m_peer))
  								 {
  									 ret = m_socket_local->Bind6 ();
  								 }
  								 else if (InetSocketAddress::IsMatchingType (m_peer) ||
  										    PacketSocketAddress::IsMatchingType (m_peer))
  								 {
  									 ret = m_socket_local->Bind ();
  								 }
  							 }

  							 if (ret == -1)
  							 {
  								 NS_FATAL_ERROR ("Failed to bind socket");
  							 }

  							 m_socket_local->Connect (m_peer);



  							 m_socket_local->SetConnectCallback (
  										MakeCallback (&SimulationProposal::ConnectionSucceeded, this),
  										MakeCallback (&SimulationProposal::ConnectionFailed, this));
  							 }
  							 m_cbrRateFailSafe = m_cbrRate;


  							 protocol->threshold_flag = 0;

  							 }
  						 }
             }
  	  }
    }


  Ptr<Packet> packet;
  if (m_unsentPacket)
    {
      packet = m_unsentPacket;
    }
  else if (m_enableSeqTsSizeHeader)
    {
      Address from, to;
      m_socket_local->GetSockName (from);
      m_socket_local->GetPeerName (to);
      SeqTsSizeHeader2 header;
      header.SetSeq (m_seq++);
      header.SetSize (m_pktSize);
      header.SetResource(m_resource_sum);
      NS_ABORT_IF (m_pktSize < header.GetSerializedSize ());
      packet = Create<Packet> (m_pktSize - header.GetSerializedSize ());
      // Trace before adding header, for consistency with PacketSink
      m_txTraceWithSeqTsSize (packet, from, to, header);
      packet->AddHeader (header);
    }
  else
    {
      packet = Create<Packet> (m_pktSize);
    }

  int actual = m_socket_local->Send (packet);
  if ((unsigned) actual == m_pktSize)
    {
      m_txTrace (packet);
      m_totBytes += m_pktSize;
      m_unsentPacket = 0;
      Address localAddress;
      m_socket_local->GetSockName (localAddress);
      if (InetSocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " on-off application sent "
                       <<  packet->GetSize () << " bytes to "
                       << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (packet, localAddress, InetSocketAddress::ConvertFrom (m_peer));
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " on-off application sent "
                       <<  packet->GetSize () << " bytes to "
                       << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
        }
    }
  else
    {
      NS_LOG_DEBUG ("Unable to send packet; actual " << actual << " size " << m_pktSize << "; caching for later attempt");
      m_unsentPacket = packet;
    }



  m_residualBits = 0;
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();
}


void SimulationProposal::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void SimulationProposal::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Can't connect");
}

void SimulationProposal::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      socket->GetSockName (localAddress);
      m_rxTrace (packet, from);
      m_rxTraceWithAddresses (packet, from, localAddress);

      if (m_enableSeqTsSizeHeader)
        {
          PacketReceived (packet, from, localAddress);
        }
    }
}

void
SimulationProposal::PacketReceived (const Ptr<Packet> &p, const Address &from,
                            const Address &localAddress)
{
  SeqTsSizeHeader2 header;
  Ptr<Packet> buffer;

  auto itBuffer = m_buffer.find (from);
  if (itBuffer == m_buffer.end ())
    {
      itBuffer = m_buffer.insert (std::make_pair (from, Create<Packet> (0))).first;
    }

  buffer = itBuffer->second;
  buffer->AddAtEnd (p);
  buffer->PeekHeader (header);

  NS_ABORT_IF (header.GetSize () == 0);

  while (buffer->GetSize () >= header.GetSize ())
    {
      NS_LOG_DEBUG ("Removing packet of size " << header.GetSize () << " from buffer of size " << buffer->GetSize ());
      Ptr<Packet> complete = buffer->CreateFragment (0, static_cast<uint32_t> (header.GetSize ()));
      buffer->RemoveAtStart (static_cast<uint32_t> (header.GetSize ()));

      complete->RemoveHeader (header);

      m_rxTraceWithSeqTsSize (complete, from, localAddress, header);

      if (buffer->GetSize () > header.GetSerializedSize ())
        {
          buffer->PeekHeader (header);
        }
      else
        {
          break;
        }
    }
}

void SimulationProposal::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void SimulationProposal::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void SimulationProposal::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&SimulationProposal::HandleRead, this));
  m_socketList.push_back (s);
}





} // Namespace ns3
