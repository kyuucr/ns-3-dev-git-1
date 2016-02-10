#ifndef ETX_METRIC_H
#define ETX_METRIC_H

#include "ns3/object.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/timer.h"
#include "ns3/log.h"
#include "ns3/routing-metric.h"

namespace ns3 {

struct EtxNeighborTuple
{
  Ipv4Address addr;
  uint32_t lastRcvSeqNum; // final seq number of the window interval
  uint32_t firstRcvSeqNum;// inital seq number of the window interval
  uint16_t rcvHellos;     // number of hellos received from a Neighbor in the last window interval
  uint16_t sntHellos;     // number of hellos sent by a Neighbor during the last window interval
  double   pdr;	          // packet delivery ratio from current node to a Neighbor
  double   fdr;	          // packet forwarding ratio from current node to a Neighbor
  double   etx;           // the etx metric
};

typedef std::vector<EtxNeighborTuple>		EtxSet;		/// Etx Set type

class EtxMetric : public RoutingMetric
{
public:
  static TypeId GetTypeId (void);
  EtxMetric ();
  virtual ~EtxMetric();
  uint16_t SendPacketProbing();
  void BeginCountingReceivedPkts();
  uint32_t GetPktsReceived();
  uint16_t m_rcvHellos;
  uint16_t m_sntHellos;
  double m_pdr;
  double m_fdr;
  void RcvPacketProbing (const Ipv4Address &senderIface, uint32_t recvSequenceNumber, uint16_t metric);
  uint16_t GetMetric4Hello(Ipv4Address &addr);
private:
  Time m_windowInterval;
  //Time m_period;
  double m_alpha;
  uint16_t m_pdr2Send;
  
  //Timer Handler
  Timer    m_probingTimer;
  Timer    m_windowTimer;
  void ProbingTimerExpire ();
  void WindowTimerExpire ();

  //Management of Neighbor Tuples
  void AddEtxNeighborTuple (const EtxNeighborTuple &tuple);
  EtxNeighborTuple* FindNeighborEtxTuple(Ipv4Address const &addr);


protected:
  virtual void DoStart ();
  virtual void DoDispose();
  EtxSet m_etxSet;
};

} // namespace ns3

#endif /*ETX_METRIC_H*/
