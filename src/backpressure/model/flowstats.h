#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"

using namespace ns3;
using namespace std;


class FlowStats {
  public:
    FlowStats ();
    ~FlowStats ();
    void SetTsSource(Time time);
    void SetTsDest(Time time);
    void SetTxTime (float);
    void SetDelay(Time time);
    void SetSeqNumber(uint32_t);
    void SetDestinationIp(const Ipv4Address addr);
    void SetSourceIp(const Ipv4Address addr);
    void SetHops(uint32_t hops);
    void SetSize(uint32_t size);
  
    
    Time GetTsSource (void) const;
    Time GetTsDest   (void) const;
    Time GetTxTime   (void) const;
    Time GetDelay   (void) const;
    float GetTxTimeF (void) const;
    uint32_t GetSeqNumber(void) const;
    const Ipv4Address GetDestinationIp();
    const Ipv4Address GetSourceIp();
    uint32_t GetHops (void) const;
    uint32_t GetSize() const;

  private:
    uint32_t m_seq;
    uint32_t m_hops;
    uint32_t m_size;
    uint64_t m_tsSource;
    uint64_t m_tsDest;
    uint64_t m_delay;
    float m_txTime;
    Ipv4Address m_srcIp;
    Ipv4Address m_dstIp;
};
