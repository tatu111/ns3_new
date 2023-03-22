

#include "simulation-proposal.h"
#include "ns3/olsr-routing-protocol.h"
#include <random>

#include "ns3/rand-simulation.h"

//#include "../scratch/simulation-proposal-scenario.cc"


#define RANDOM_MAX 500000000
#define RANDOM_MIN 5000000


namespace ns3 {

//NS_LOG_COMPONENT_DEFINE ("RandSimulation");
//
//NS_OBJECT_ENSURE_REGISTERED (RandSimulation);

//TypeId
//RandSimulation::GetTypeId (void){
//	static TypeId tid = TypeId ("ns3::RandSimulation")
//		.AddConstructor<RandSimulation> ();
//
//	return tid;
//}


void
RandSimulation::rand_node (){

	uint32_t node_number = 36;//ノード数に応じて変更が必要


	for(uint32_t i=0;i<node_number;i++){

		std::mt19937 engine(i*2);//乱数の種の変更が必要

		std::uniform_real_distribution<double> rnd(RANDOM_MIN, RANDOM_MAX);//メルセンヌ・ツイスタを利用

		m_resource_num = rnd(engine);
		m_resource = m_resource_num/25000000;

	 	randnode.insert(std::make_pair(i,m_resource_num));
	}

}





}
