#ifndef RAND_SIMULATION_H
#define RAND_SIMULATION_H

//#include "ns3/log.h"
//#include "ns3/address.h"
//#include "ns3/application.h"
//#include "ns3/event-id.h"
//#include "ns3/ptr.h"
//#include "ns3/data-rate.h"
//#include "ns3/traced-callback.h"
//#include "ns3/seq-ts-size-header.h"
//#include "ns3/inet-socket-address.h"
//#include <unordered_map>

#include "simulation-proposal.h"
#include "ns3/olsr-routing-protocol.h"
#include <random>
#include <vector>
//#include "../scratch/simulation-proposal-scenario.cc"


namespace ns3 {

class RandSimulation
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */


//  static TypeId GetTypeId (void);

  RandSimulation ();

  virtual ~RandSimulation ();

  void rand_node ();

  uint8_t m_resource; //計算機資源量を表す
  uint32_t m_resource_num; //計算機資源量の具体的な数値を表す

  std::map<int, std::vector<uint32_t>> randnode;
  std::vector<uint32_t> node_ID;

};

}

#endif /*RandSimulation*/
