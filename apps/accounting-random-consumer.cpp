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

#include "accounting-random-consumer.hpp"
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

NS_LOG_COMPONENT_DEFINE("ndn.AccountingRandomConsumer");

char
randomChar2()
{
    int stringLength = sizeof(alphanum) - 1;
    return alphanum[rand() % stringLength];
}

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(AccountingRandomConsumer);

TypeId
AccountingRandomConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::AccountingRandomConsumer")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<AccountingRandomConsumer>()
      .AddAttribute("ConsumerID", "Consumer ID",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&AccountingRandomConsumer::m_id), MakeIntegerChecker<uint32_t>())

      .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                    MakeUintegerAccessor(&AccountingRandomConsumer::SetNumberOfContents,
                                         &AccountingRandomConsumer::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                    MakeDoubleAccessor(&AccountingRandomConsumer::SetQ,
                                       &AccountingRandomConsumer::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power", StringValue("0.7"),
                    MakeDoubleAccessor(&AccountingRandomConsumer::SetS,
                                       &AccountingRandomConsumer::GetS),
                    MakeDoubleChecker<double>())

      .AddTraceSource("ReceivedMeaningfulContent", "Trace called every time meaningful content is received",
                   MakeTraceSourceAccessor(&AccountingRandomConsumer::m_receivedMeaningfulContent));

  return tid;
}

AccountingRandomConsumer::AccountingRandomConsumer()
  : m_N(100) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
  , m_q(0.7)
  , m_s(0.7)
  , m_SeqRng(0.0, 1.0)
{
  // SetNumberOfContents is called by NS-3 object system during the initialization
}

AccountingRandomConsumer::~AccountingRandomConsumer()
{
}

void
AccountingRandomConsumer::SetNumberOfContents(uint32_t numOfContents)
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
AccountingRandomConsumer::GetNumberOfContents() const
{
  return m_N;
}

void
AccountingRandomConsumer::SetQ(double q)
{
  m_q = q;
  SetNumberOfContents(m_N);
}

double
AccountingRandomConsumer::GetQ() const
{
  return m_q;
}

void
AccountingRandomConsumer::SetS(double s)
{
  m_s = s;
  SetNumberOfContents(m_N);
}

double
AccountingRandomConsumer::GetS() const
{
  return m_s;
}

void
AccountingRandomConsumer::OnData(shared_ptr<const Data> contentObject)
{
  Consumer::OnData(contentObject); // default receive logic
  receiveCount++;

  for(std::vector<NameTime*>::iterator it = startTimes.begin(); it != startTimes.end(); ++it) {
     NameTime *nt = *it;
     if (nt->name == contentObject->getName()) {
         NameTime *nameRtt = new NameTime(contentObject->getName(), Simulator::Now(), Simulator::Now());
         nameRtt->name = contentObject->getName();
         nameRtt->rtt = (Simulator::Now() - nt->rtt);

         rtts.push_back(nameRtt);
         m_receivedMeaningfulContent(this);

         delete nt;
         startTimes.erase(it); // drop it from the list, and go on with our lives

         break;
     }
  }
}

void
AccountingRandomConsumer::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s max -> " << m_seqMax << "\n";

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());

    NS_LOG_DEBUG("=interest seq " << seq << " from m_retxSeqs");
    break;
  }

  seq = 0;

  // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << seq << "\n";

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);

  // Append the random nonce TO THE NAME (different from the loop nonce)
  // 2 for good measure... heh.
  nameWithSequence->appendNumber(m_rand.GetValue());
  nameWithSequence->appendNumber(m_rand.GetValue());

  char randomString[32] = {0};
  for(unsigned int i = 0; i < 32; ++i)
  {
    randomString[i] = randomChar2();
  }
  nameWithSequence->append(randomString);

  // Now actually create the interest
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*nameWithSequence);

  // Note: we're setting this randomly, just to test the encoding/decoding
  std::vector<uint64_t> payload;
  payload.push_back(seq);
  payload.push_back(seq);
  payload.push_back(seq);
  payload.push_back(seq);
  interest->setPayload(payload);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
  NS_LOG_DEBUG("Trying to add " << seq << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqRetxCounts[seq]++;

  m_rtt->SentSeq(SequenceNumber32(seq), 1);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  NameTime *nt = new NameTime(interest->getName(), Simulator::Now(), Simulator::Now());
  startTimes.push_back(nt);

  AccountingRandomConsumer::ScheduleNextPacket();
  sentCount++;
}

uint32_t
AccountingRandomConsumer::GetNextSeq()
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
AccountingRandomConsumer::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &AccountingRandomConsumer::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &AccountingRandomConsumer::SendPacket, this);
}

} // namespace ndn
} // namespace ns3
