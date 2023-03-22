/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
 * Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
 *                    (added timestamp and size fields)
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

#include "seq-ts-size-header2.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeqTsSizeHeader2");

NS_OBJECT_ENSURE_REGISTERED (SeqTsSizeHeader2);

SeqTsSizeHeader2::SeqTsSizeHeader2 ()
  : SeqTsHeader ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SeqTsSizeHeader2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqTsSizeHeader2")
    .SetParent<SeqTsHeader> ()
    .SetGroupName ("simulation-proposal")
    .AddConstructor<SeqTsSizeHeader2> ()
  ;
  return tid;
}

TypeId
SeqTsSizeHeader2::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
SeqTsSizeHeader2::SetSize (uint64_t size)
{
  m_size = size;
}

void
SeqTsSizeHeader2::SetResource (uint32_t resource_sum)
{
  m_resource = resource_sum;
}

uint64_t
SeqTsSizeHeader2::GetSize (void) const
{
  return m_size;
}

uint32_t
SeqTsSizeHeader2::GetResource (void) const
{
  return m_resource;
}

void
SeqTsSizeHeader2::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(size=" << m_size << ") AND ";
  SeqTsHeader::Print (os);
}

uint32_t
SeqTsSizeHeader2::GetSerializedSize (void) const
{
  return SeqTsHeader::GetSerializedSize () + 8;
}

void
SeqTsSizeHeader2::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU64 (m_size);
  i.WriteHtonU32(m_resource);
  SeqTsHeader::Serialize (i);
}

uint32_t
SeqTsSizeHeader2::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_size = i.ReadNtohU64 ();
  m_resource = i.ReadNtohU32();
  SeqTsHeader::Deserialize (i);
  return GetSerializedSize ();
}

} // namespace ns3
