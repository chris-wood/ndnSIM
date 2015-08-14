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

#include "accounting-encr-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "model/ndn-app-face.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.AccountingEncrConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(AccountingEncrConsumer);

TypeId
AccountingEncrConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::AccountingEncrConsumer")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<AccountingEncrConsumer>()

      .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                    MakeUintegerAccessor(&AccountingEncrConsumer::SetNumberOfContents,
                                         &AccountingEncrConsumer::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                    MakeDoubleAccessor(&AccountingEncrConsumer::SetQ,
                                       &AccountingEncrConsumer::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power", StringValue("0.7"),
                    MakeDoubleAccessor(&AccountingEncrConsumer::SetS,
                                       &AccountingEncrConsumer::GetS),
                    MakeDoubleChecker<double>());

  return tid;
}

AccountingEncrConsumer::AccountingEncrConsumer()
  : m_N(100) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
  , m_q(0.7)
  , m_s(0.7)
  , m_SeqRng(0.0, 1.0)
{
  // SetNumberOfContents is called by NS-3 object system during the initialization
}

AccountingEncrConsumer::~AccountingEncrConsumer()
{
}

void
AccountingEncrConsumer::SetNumberOfContents(uint32_t numOfContents)
{
  m_N = numOfContents;

  NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);

  m_Pcum = std::vector<double>(m_N + 1);

  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
  }

  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
    NS_LOG_LOGIC("Cumulative probability [" << i << "]=" << m_Pcum[i]);
  }
}

uint32_t
AccountingEncrConsumer::GetNumberOfContents() const
{
  return m_N;
}

void
AccountingEncrConsumer::SetQ(double q)
{
  m_q = q;
  SetNumberOfContents(m_N);
}

double
AccountingEncrConsumer::GetQ() const
{
  return m_q;
}

void
AccountingEncrConsumer::SetS(double s)
{
  m_s = s;
  SetNumberOfContents(m_N);
}

double
AccountingEncrConsumer::GetS() const
{
  return m_s;
}

void
AccountingEncrConsumer::OnData(shared_ptr<const Data> contentObject)
{
  Consumer::OnData(contentObject); // default receive logic
  receiveCount++;
}

void
AccountingEncrConsumer::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s max -> " << m_seqMax << "\n";

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());

    // NS_ASSERT (m_seqLifetimes.find (seq) != m_seqLifetimes.end ());
    // if (m_seqLifetimes.find (seq)->time <= Simulator::Now ())
    //   {

    //     NS_LOG_DEBUG ("Expire " << seq);
    //     m_seqLifetimes.erase (seq); // lifetime expired. Trying to find another unexpired
    //     sequence number
    //     continue;
    //   }
    NS_LOG_DEBUG("=interest seq " << seq << " from m_retxSeqs");
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) // no retransmission
  {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = AccountingEncrConsumer::GetNextSeq();
    m_seq++;
  }

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << seq << "\n";

  shared_ptr<Name> keyName = make_shared<Name>(m_interestName);
  keyName->append("key"); // add the key annotation to this guy

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  m_seq++;
  keyName->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  shared_ptr<Interest> keyInterest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  keyInterest->setNonce(m_rand.GetValue());
  interest->setName(*nameWithSequence);
  keyInterest->setName(*keyName);

  // Note: we're setting this randomly, just to test the encoding/decoding
  std::vector<uint64_t> payload;
  payload.push_back(seq);
  payload.push_back(seq);
  payload.push_back(seq);
  payload.push_back(seq);
  interest->setPayload(payload);
  keyInterest->setPayload(payload); // we can copy this payload, it's not important

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
  NS_LOG_DEBUG("Trying to add " << seq << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqTimeouts.insert(SeqTimeout(seq - 1, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq - 1, Simulator::Now()));

  m_seqLastDelay.erase(seq);
  m_seqLastDelay.erase(seq - 1);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqLastDelay.insert(SeqTimeout(seq - 1, Simulator::Now()));

  m_seqRetxCounts[seq]++;
  m_seqRetxCounts[seq - 1]++;

  m_rtt->SentSeq(SequenceNumber32(seq), 1);
  m_rtt->SentSeq(SequenceNumber32(seq - 1), 1);

  m_transmittedInterests(interest, this, m_face);
  m_transmittedInterests(keyInterest, this, m_face);
  m_face->onReceiveInterest(*interest);
  m_face->onReceiveInterest(*keyInterest);

  AccountingEncrConsumer::ScheduleNextPacket();
  AccountingEncrConsumer::ScheduleNextPacket();
  sentCount += 2; // we issue an interest for the content and key at the same time
}

uint32_t
AccountingEncrConsumer::GetNextSeq()
{
  uint32_t content_index = 1; //[1, m_N]
  double p_sum = 0;

  double p_random = m_SeqRng.GetValue();
  while (p_random == 0) {
    p_random = m_SeqRng.GetValue();
  }
  // if (p_random == 0)
  NS_LOG_LOGIC("p_random=" << p_random);
  for (uint32_t i = 1; i <= m_N; i++) {
    p_sum = m_Pcum[i]; // m_Pcum[i] = m_Pcum[i-1] + p[i], p[0] = 0;   e.g.: p_cum[1] = p[1],
                       // p_cum[2] = p[1] + p[2]
    if (p_random <= p_sum) {
      content_index = i;
      break;
    } // if
  }   // for
  // content_index = 1;
  NS_LOG_DEBUG("RandomNumber=" << content_index);
  return content_index;
}

void
AccountingEncrConsumer::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &AccountingEncrConsumer::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &AccountingEncrConsumer::SendPacket, this);
}


// TypeId
// AccountingEncrConsumer::GetTypeId(void)
// {
//   static TypeId tid =
//     TypeId("ns3::ndn::AccountingEncrConsumer")
//       .SetGroupName("Ndn")
//       .SetParent<ConsumerCbr>()
//       .AddConstructor<AccountingEncrConsumer>()

//     ;

//   return tid;
// }

// AccountingEncrConsumer::AccountingEncrConsumer() : sentCount(0)
// {
//   NS_LOG_FUNCTION_NOARGS();
// }

// void
// AccountingEncrConsumer::SendPacket()
// {
//   if (!m_active)
//     return;

//   NS_LOG_FUNCTION_NOARGS();

//   uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

//   while (m_retxSeqs.size()) {
//     seq = *m_retxSeqs.begin();
//     m_retxSeqs.erase(m_retxSeqs.begin());
//     break;
//   }

//   if (seq == std::numeric_limits<uint32_t>::max()) {
//     if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
//       if (m_seq >= m_seqMax) {
//         return; // we are totally done
//       }
//     }

//     seq = m_seq++;
//   }

//   //
//   shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
//   nameWithSequence->appendSequenceNumber(seq); // all the same
//   //

//   // shared_ptr<Interest> interest = make_shared<Interest> ();
//   shared_ptr<Interest> interest = make_shared<Interest>();
//   interest->setNonce(m_rand.GetValue());
//   interest->setName(*nameWithSequence);
//   time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
//   interest->setInterestLifetime(interestLifeTime);

  // // Note: we're setting this randomly, just to test the encoding/decoding
  // std::vector<uint64_t> payload;
  // payload.push_back(seq);
  // payload.push_back(seq);
  // payload.push_back(seq);
  // payload.push_back(seq);
  // interest->setPayload(payload);

//   // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
//   NS_LOG_INFO("> Interest for " << seq);

//   WillSendOutInterest(seq);

//   m_transmittedInterests(interest, this, m_face);
//   m_face->onReceiveInterest(*interest);

//   ScheduleNextPacket();
//   sentCount++;
// }

} // namespace ndn
} // namespace ns3
