/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_ACCOUNTING_RANDOM_CONSUMER_H
#define NDN_ACCOUNTING_RANDOM_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-consumer-cbr.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class AccountingRandomConsumer : public ConsumerCbr {
public:
  static TypeId
  GetTypeId();

  // constructor
  AccountingRandomConsumer();
  ~AccountingRandomConsumer();

  std::vector<NameTime*> startTimes;

protected:

  virtual void
  SendPacket();

  virtual void
  OnData(shared_ptr<const Data> data);

  uint32_t
  GetNextSeq();

  virtual void
  ScheduleNextPacket();

private:
  void
  SetNumberOfContents(uint32_t numOfContents);

  uint32_t
  GetNumberOfContents() const;

  void
  SetQ(double q);

  double
  GetQ() const;

  void
  SetS(double s);

  double
  GetS() const;

private:
  uint32_t m_id;
  uint32_t m_N;               // number of the contents
  double m_q;                 // q in (k+q)^s
  double m_s;                 // s in (k+q)^s
  std::vector<double> m_Pcum; // cumulative probability
  uint64_t sentCount;
  uint64_t receiveCount;

  UniformVariable m_SeqRng; // RNG

  // Meaningful content retrieval trace callback
  TracedCallback<Ptr<AccountingRandomConsumer>> m_receivedMeaningfulContent;

};

} // namespace ndn
} // namespace ns3

#endif // NDN_ACCOUNTING_RANDOM_CONSUMER_H
