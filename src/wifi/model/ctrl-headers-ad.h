/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 * ns-3 upstreaming process: Natale Patriciello <natale.patriciello@gmail.com>
 */
#pragma once

#include "ns3/header.h"
#include "fields-headers.h"

namespace ns3 {

/*************************
 *  Poll Frame (8.3.1.11)
 *************************/

/**
 * \ingroup wifi
 * \brief Header for Poll Frame.
 */
class CtrlDmgPoll : public Header
{
public:
  CtrlDmgPoll ();
  ~CtrlDmgPoll ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the offset in units of 1 microseconds.
   *
   * \param value The offset in units of 1 microseconds.
   */
   void SetResponseOffset (uint16_t value);

  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  uint16_t GetResponseOffset (void) const;

private:
  uint16_t m_responseOffset;

};

/***********************************************
 * Service Period Request (SPR) Frame (8.3.1.12)
 ***********************************************/

/**
 * \ingroup wifi
 * \brief Header for Service Period Request (SPR) Frame.
 */
class CtrlDMG_SPR : public Header
{
public:
  CtrlDMG_SPR ();
  ~CtrlDMG_SPR ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the Dynamic Allocation Information Field.
   *
   * \param value The Dynamic Allocation Information Field.
   */
  void SetDynamicAllocationInfo (DynamicAllocationInfoField field);
  /**
   * Set the offset in units of 1 microseconds.
   *
   * \param value The offset in units of 1 microseconds.
   */
  void SetBFControl (BF_Control_Field value);

  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  DynamicAllocationInfoField GetDynamicAllocationInfo (void) const;
  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  BF_Control_Field GetBFControl (void) const;

private:
  DynamicAllocationInfoField m_dynamic;
  BF_Control_Field m_bfControl;

};

/*************************
 * Grant Frame (8.3.1.13)
 *************************/

/**
 * \ingroup wifi
 * \brief Header for Grant Frame.
 */
class CtrlDMG_Grant : public CtrlDMG_SPR
{
public:
  CtrlDMG_Grant ();
  ~CtrlDMG_Grant ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

};

/********************************************
 * DMG Denial to Send (DTS) Frame (8.3.1.15)
 ********************************************/

/**
 * \ingroup wifi
 * \brief Header for Denial to Send (DTS) Frame.
 */
class CtrlDMG_DTS: public Header
{
public:
  CtrlDMG_DTS ();
  ~CtrlDMG_DTS ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * Set the Dynamic Allocation Information Field.
   *
   * \param value The Dynamic Allocation Information Field.
   */
  void SetNAV_SA (Mac48Address value);
  /**
   * Set the offset in units of 1 microseconds.
   *
   * \param value The offset in units of 1 microseconds.
   */
  void SetNAV_DA (Mac48Address value);

  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  Mac48Address GetNAV_SA (void) const;
  /**
   * Return the offset in units of 1 microseconds.
   *
   * \return the offset in units of 1 microseconds.
   */
  Mac48Address GetNAV_DA (void) const;

private:
    Mac48Address m_navSA;
    Mac48Address m_navDA;

};

/****************************************
 *  Sector Sweep (SSW) Frame (8.3.1.16)
 ****************************************/

/**
 * \ingroup wifi
 * \brief Header for Sector Sweep (SSW) Frame.
 */
class CtrlDMG_SSW : public Header
{
public:
  CtrlDMG_SSW ();
  ~CtrlDMG_SSW ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetSswField (DMG_SSW_Field &field);
  void SetSswFeedbackField (DMG_SSW_FBCK_Field &field);
  DMG_SSW_Field GetSswField (void) const;
  DMG_SSW_FBCK_Field GetSswFeedbackField (void) const;

private:
  DMG_SSW_Field m_ssw;
  DMG_SSW_FBCK_Field m_sswFeedback;

};

/*********************************************************
 *  Sector Sweep Feedback (SSW-Feedback) Frame (8.3.1.17)
 *********************************************************/

/**
 * \ingroup wifi
 * \brief Header for Sector Sweep Feedback (SSW-Feedback) Frame.
 */
class CtrlDMG_SSW_FBCK : public Header
{
public:
  CtrlDMG_SSW_FBCK ();
  virtual ~CtrlDMG_SSW_FBCK ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetSswFeedbackField (DMG_SSW_FBCK_Field &field);
  void SetBrpRequestField (BRP_Request_Field &field);
  void SetBfLinkMaintenanceField (BF_Link_Maintenance_Field &field);

  DMG_SSW_FBCK_Field GetSswFeedbackField (void) const;
  BRP_Request_Field GetBrpRequestField (void) const;
  BF_Link_Maintenance_Field GetBfLinkMaintenanceField (void) const;

private:
  DMG_SSW_FBCK_Field m_sswFeedback;
  BRP_Request_Field m_brpRequest;
  BF_Link_Maintenance_Field m_linkMaintenance;

};

/**********************************************
 * Sector Sweep ACK (SSW-ACK) Frame (8.3.1.18)
 **********************************************/

/**
 * \ingroup wifi
 * \brief Header for Sector Sweep ACK (SSW-ACK) Frame.
 */
class CtrlDMG_SSW_ACK : public CtrlDMG_SSW_FBCK
{
public:
  CtrlDMG_SSW_ACK (void);
  virtual ~CtrlDMG_SSW_ACK (void);
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

};

/******************************
 *  Grant ACK Frame (8.3.1.19)
 ******************************/

/**
 * \ingroup wifi
 * \brief Header for Grant ACK Frame.
 * The Grant ACK frame is sent only in CBAPs as a response to the reception of a Grant frame
 * that has the Beamforming Training field equal to 1.
 */
class CtrlGrantAck : public Header
{
public:
  CtrlGrantAck ();
  ~CtrlGrantAck ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint8_t m_reserved[5];
  BF_Control_Field m_bfControl;

};

} // namespace ns3
