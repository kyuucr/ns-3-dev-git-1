/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef TCPCUBIC_H
#define TCPCUBIC_H

#include "ns3/tcp-socket-base.h"
#include "ns3/timer.h"


namespace ns3 {

/**
 * \brief The Cubic Congestion Control Algorithm
 *
 * CUBIC is an implementation of TCP with an optimized congestion control
 * algorithm for high bandwidth networks with high latency (LFN: long fat
 * networks).
 *
 * It is a less aggressive and more systematic derivative of BIC TCP,
 * in which the window is a cubic function of time since the last
 * congestion event, with the inflection point set to the window prior to
 * the event. Being a cubic function, there are two components to window
 * growth. The first is a concave portion where the window quickly ramps
 * up to the window size before the last congestion event. Next is the convex
 * growth where CUBIC probes for more bandwidth, slowly at first then very
 * rapidly. CUBIC spends a lot of time at a plateau between the concave
 * and convex growth region which allows the network to stabilize before
 * CUBIC begins looking for more bandwidth.
 *
 * Another major difference between CUBIC and standard TCP flavors is
 * that it does not rely on the receipt of ACKs to increase the
 * window size. CUBIC's window size is dependent only on the last
 * congestion event. With standard TCP, flows with very short RTTs
 * will receive ACKs faster and therefore have their congestion windows
 * grow faster than other flows with longer RTTs. CUBIC allows for more
 * fairness between flows since the window growth is independent of RTT.
 *
 * CUBIC TCP is implemented and used by default in Linux kernels 2.6.19
 * and above; this version follows the implementation in Linux 3.14, which
 * slightly differ from the CUBIC paper. It also features HyStart.
 *
 * Home page:
 *      http://netsrv.csc.ncsu.edu/twiki/bin/view/Main/BIC
 * The work starts from the implementation of CUBIC TCP in
 * Sangtae Ha, Injong Rhee and Lisong Xu,
 * "CUBIC: A New TCP-Friendly High-Speed TCP Variant"
 * in ACM SIGOPS Operating System Review, July 2008.
 * Available from:
 *  http://netsrv.csc.ncsu.edu/export/cubic_a_new_tcp_2008.pdf
 *
 * CUBIC integrates a new slow start algorithm, called HyStart.
 * The details of HyStart are presented in
 *  Sangtae Ha and Injong Rhee,
 *  "Taming the Elephants: New TCP Slow Start", NCSU TechReport 2008.
 * Available from:
 *  http://netsrv.csc.ncsu.edu/export/hystart_techreport_2008.pdf
 */
class TcpCubic : public TcpSocketBase
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpCubic ();

  // From TcpSocketBase
  virtual int Connect (const Address &address);
  virtual int Listen (void);

protected:
  // From TcpSocketBase
  virtual uint32_t Window (void);
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual void NewAck (SequenceNumber32 const& seq);
  virtual void DupAck (const TcpHeader& t, uint32_t count);
  virtual void Retransmit (void);

  // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetInitialSSThresh (uint32_t threshold);
  virtual uint32_t GetInitialSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;


protected:
  /**
   * \brief Values to detect the Slow Start mode of HyStart
   */
  enum HybridSSDetectionMode
  {
    PACKET_TRAIN = 0x1, //!< Detection by trains of packet
    DELAY        = 0x2  //!< Detection by delay value
  };

  /**
   * \brief State of the congestion control machine: open or loss
   */
  enum CubicState
  {
    OPEN     = 0x1, //!< Open state
    LOSS     = 0x2, //!< Loss state
  };

  bool     m_fastConvergence;  //!< Enable or disable fast convergence algorithm
  double   m_beta;             //!< Beta for cubic multiplicative increase

  bool     m_hystart;          //!< Enable or disable HyStart algorithm
  int      m_hystartDetect;    //!< Detect way for HyStart algorithm \see HybridSSDetectionMode
  uint32_t m_hystartLowWindow; //!< Lower bound cWnd for hybrid slow start (segments)
  Time     m_hystartAckDelta;  //!< Spacing between ack's indicating train
  Time     m_hystartDelayMin;  //!< Minimum time for hystart algorithm
  Time     m_hystartDelayMax;  //!< Maximum time for hystart algorithm
  uint8_t  m_hystartMinSamples;//!< Number of delay samples for detecting the increase of delay

  uint32_t m_initialCwnd;      //!< Initial cWnd
  uint32_t m_cWndAfterLoss;    //!< cWnd after detecting a loss
  uint8_t  m_cntClamp;         //!< Modulo of the (avoided) float division for cWnd

  double   m_c;                //!< Cubic Scaling factor

  // Cubic parameters
  CubicState   m_cubicState;      //!<  Cubic state \see CubicState
  uint32_t     m_cWndCnt;         //!<  cWnd integer-to-float counter
  uint32_t     m_lastMaxCwnd;     //!<  Last maximum cWnd
  uint32_t     m_lossCwnd;        //!<  Congestion window at last loss
  uint32_t     m_bicOriginPoint;  //!<  Origin point of bic function
  uint32_t     m_bicK;            //!<  Time to origin point from the beginning of the current epoch
  Time         m_delayMin;        //!<  Min delay
  Time         m_epochStart;      //!<  Beginning of an epoch
  uint8_t      m_found;           //!<  The exit point is found?
  Time         m_roundStart;      //!<  Beginning of each round
  SequenceNumber32   m_endSeq;    //!<  End sequence of the round
  Time         m_lastAck;         //!<  Last time when the ACK spacing is close
  Time         m_cubicDelta;      //!<  Time to wait after recovery before update
  Time         m_currRtt;         //!<  Current Rtt
  uint32_t     m_sampleCnt;

  TracedValue<uint32_t>     m_cWnd;     //!< Congestion window
  TracedValue<uint32_t>     m_ssThresh; //!< Slow start threshold

private:
  /**
   * \brief Reset BIC parameters
   */
  void Reset (void);

  /**
   * \brief Reset HyStart parameters
   */
  void HystartReset (void);

  /**
   * \brief Initialize the algorithm
   */
  void Init (void);

  /**
   * \brief Update HyStart parameters
   *
   * \param delay Delay for HyStart algorithm
   */
  void HystartUpdate (const Time& delay);

  /**
   * \brief Clamp time value in a range
   *
   * The returned value is t, clamped in a range specified
   * by attributes (HystartDelayMin < t < HystartDelayMax)
   *
   * \param t Time value to clamp
   * \return t itself if it is in range, otherwise the min or max
   * value
   */
  Time HystartDelayThresh (Time t) const;

  /**
   * \brief Timing calculation about acks
   */
  void PktsAcked (void);

  /**
   * \brief The congestion avoidance phase of Cubic
   *
   * \param seq The sequence number acked
   */
  void CongAvoid (const SequenceNumber32 &seq);

  /**
   * \brief Cubic window update after a new ack received
   */
  uint32_t WindowUpdate(void);

  /**
   * \brief Cubic window update after a loss
   */
  void RecalcSsthresh (void);
};

}

#endif // TCPCUBIC_H
