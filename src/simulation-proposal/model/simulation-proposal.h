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

#ifndef SIMULATION_PROPOSAL_H
#define SIMULATION_PROPOSAL_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/seq-ts-size-header2.h"
#include "ns3/inet-socket-address.h"
#include <unordered_map>

#include "ns3/olsr-routing-protocol.h"

//class RoutingProtocol;

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

/**
 * \ingroup applications
 * \defgroup onoff OnOffApplication
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 */
/**
* \ingroup onoff
*
* \brief Generate traffic to a single destination according to an
*        OnOff pattern.
*
* This traffic generator follows an On/Off pattern: after
* Application::StartApplication
* is called, "On" and "Off" states alternate. The duration of each of
* these states is determined with the onTime and the offTime random
* variables. During the "Off" state, no traffic is generated.
* During the "On" state, cbr traffic is generated. This cbr traffic is
* characterized by the specified "data rate" and "packet size".
*
* Note:  When an application is started, the first packet transmission
* occurs _after_ a delay equal to (packet size/bit rate).  Note also,
* when an application transitions into an off state in between packet
* transmissions, the remaining time until when the next transmission
* would have occurred is cached and is used when the application starts
* up again.  Example:  packet size = 1000 bits, bit rate = 500 bits/sec.
* If the application is started at time 3 seconds, the first packet
* transmission will be scheduled for time 5 seconds (3 + 1000/500)
* and subsequent transmissions at 2 second intervals.  If the above
* application were instead stopped at time 4 seconds, and restarted at
* time 5.5 seconds, then the first packet would be sent at time 6.5 seconds,
* because when it was stopped at 4 seconds, there was only 1 second remaining
* until the originally scheduled transmission, and this time remaining
* information is cached and used to schedule the next transmission
* upon restarting.
*
* If the underlying socket type supports broadcast, this application
* will automatically enable the SetAllowBroadcast(true) socket option.
*
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will
 * use some bytes of the payload to store an header with a sequence number,
 * a timestamp, and the size of the packet sent. Support for extracting
 * statistics from this header have been added to \c ns3::PacketSink
 * (enable its "EnableSeqTsSizeHeader" attribute), or users may extract
 * the header via trace sources.  Note that the continuity of the sequence
 * number may be disrupted across On/Off cycles.
*/



class SimulationProposal : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */

//  friend class ::RoutingProtocol;


  static TypeId GetTypeId (void);

  SimulationProposal ();

  virtual ~SimulationProposal ();


  //パケット送信に関するもの(ここから)
  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

 /**
  * \brief Assign a fixed random variable stream number to the random variables
  * used by this model.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);
  //パケット送信に関するもの(ここまで)

  //パケット受信に関するもの(ここから)
  /**
   * \return the total bytes received in this sink app
   */
  uint64_t GetTotalRx () const;

  /**
   * \return pointer to listening socket
   */
  Ptr<Socket> GetListeningSocket (void) const;

  /**
   * \return list of pointers to accepted sockets
   */
  std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

  /**
   * TracedCallback signature for a reception with addresses and SeqTsSizeHeader
   *
   * \param p The packet received (without the SeqTsSize header)
   * \param from From address
   * \param to Local address
   * \param header The SeqTsSize header
   */
  typedef void (* SeqTsSizeCallback)(Ptr<const Packet> p, const Address &from, const Address & to,
                                   const SeqTsSizeHeader &header);
  //パケット受信に関するもの(ここまで)


  int send_count = 0;


protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop


  //パケット送信に関するもの(ここから)
  //helpers
  /**
   * \brief Cancel all pending events.
   */
  void CancelEvents ();

  // Event handlers
  /**
   * \brief Start an On period
   */
  void StartSending ();
  /**
   * \brief Start an Off period
   */
  void StopSending ();
  /**
   * \brief Send a packet
   */
  void SendPacket ();

  Ptr<Socket>     m_socket;       //!< Associated socket
  Ptr<Socket>     m_socket_local;       //!< Associated socket(local)
  Address         m_peer;         //!< Peer address
  Address         m_local;        //!< Local address to bind to
  bool            m_connected;    //!< True if connected
  Ptr<RandomVariableStream>  m_onTime;       //!< rng for On Time
  Ptr<RandomVariableStream>  m_offTime;      //!< rng for Off Time
  DataRate        m_cbrRate;      //!< Rate that data is generated
  DataRate        m_cbrRateFailSafe;      //!< Rate that data is generated (check copy)
  uint32_t        m_pktSize;      //!< Size of packets
  uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
  Time            m_lastStartTime; //!< Time last packet sent
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  EventId         m_startStopEvent;     //!< Event id for next start or stop event
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
  TypeId          m_tid;          //!< Type of the socket used
  uint32_t        m_seq {0};      //!< Sequence
  Ptr<Packet>     m_unsentPacket; //!< Unsent packet cached for future attempt
  bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the use of SeqTsSizeHeader
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
  uint64_t        m_totalRx;      //!< Total bytes received


  //追加
  //Time task_interval;   //タスクの要求の発生インターバル
  std::map<uint32_t, std::vector <uint32_t>> sink_grasping;

  //coreノード候補における周辺空き計算機資源量の総和
  uint32_t m_resource_sum;

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Callbacks for tracing the packet Tx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;

  /// Callback for tracing the packet Tx events, includes source, destination, the packet sent, and header
  TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader &> m_txTraceWithSeqTsSize;

  TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader2 &> m_txTraceWithSeqTsSize2;

private:
  /**
   * \brief Schedule the next packet transmission
   */
  void ScheduleNextTx ();
  /**
   * \brief Schedule the next On period start
   */
  void ScheduleStartEvent ();
  /**
   * \brief Schedule the next Off period start
   */
  void ScheduleStopEvent ();
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
  //パケット送信に関するもの(ここまで)

  //パケット受信に関するもの(ここから)
      void HandleRead (Ptr<Socket> socket);
        /**
         * \brief Handle an incoming connection
         * \param socket the incoming connection socket
         * \param from the address the connection is from
         */
        void HandleAccept (Ptr<Socket> socket, const Address& from);
        /**
         * \brief Handle an connection close
         * \param socket the connected socket
         */
        void HandlePeerClose (Ptr<Socket> socket);
        /**
         * \brief Handle an connection error
         * \param socket the connected socket
         */
        void HandlePeerError (Ptr<Socket> socket);

        /**
         * \brief Packet received: assemble byte stream to extract SeqTsSizeHeader
         * \param p received packet
         * \param from from address
         * \param localAddress local address
         *
         * The method assembles a received byte stream and extracts SeqTsSizeHeader
         * instances from the stream to export in a trace source.
         */
        void PacketReceived (const Ptr<Packet> &p, const Address &from, const Address &localAddress);

        /**
         * \brief Hashing for the Address class
         */
        struct AddressHash
        {
          /**
           * \brief operator ()
           * \param x the address of which calculate the hash
           * \return the hash of x
           *
           * Should this method go in address.h?
           *
           * It calculates the hash taking the uint32_t hash value of the ipv4 address.
           * It works only for InetSocketAddresses (Ipv4 version)
           */
          size_t operator() (const Address &x) const
          {
            NS_ABORT_IF (!InetSocketAddress::IsMatchingType (x));
            InetSocketAddress a = InetSocketAddress::ConvertFrom (x);
            return std::hash<uint32_t>()(a.GetIpv4 ().Get ());
          }
        };

        std::unordered_map<Address, Ptr<Packet>, AddressHash> m_buffer; //!< Buffer for received packets

    //    // In the case of TCP, each socket accept returns a new socket, so the
    //    // listening socket is stored separately from the accepted sockets
    //    std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
    //    uint64_t        m_totalRx;      //!< Total bytes received
    //  TypeId          m_tid;          //!< Protocol TypeId


        /// Traced Callback: received packets, source address.
        TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
        /// Callback for tracing the packet Rx events, includes source and destination addresses
        TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
        /// Callbacks for tracing the packet Rx events, includes source, destination addresses, and headers
        TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader&> m_rxTraceWithSeqTsSize;
        TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader2&> m_rxTraceWithSeqTsSize2;
        //パケット受信に関するもの(ここまで)


};

} // namespace ns3

#endif /* SIMULATION_PROPOSAL_H */
