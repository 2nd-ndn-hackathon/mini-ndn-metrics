/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014, Washington University in St. Louis,
 *
 */

#include <boost/asio.hpp>
#include <ndn-cxx/face.hpp>
#include <fstream>
#include <unordered_map>
#include <list>
#include <chrono>
#include "nfdStatusCollector.hpp"
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/scheduler-scoped-event-id.hpp>
#include <ndn-cxx/util/network-monitor.hpp>
#include <sys/wait.h>
#include <ndn-cxx/util/time.hpp>

#include <fstream>


#define APP_SUFFIX "/ndnmap/stats"

int DEBUG = 0;
namespace ndn {

class Statistics
{

public:
    Statistics()
        : timestamp("0")
        , txBytes(0)
        , rxBytes(0) {}


    void
    setBytes(uint64_t tx, uint64_t rx, std::string timestamp)
    {
        txBytes = tx;
        rxBytes = rx;
        this->timestamp = timestamp;
    }

    std::string
    getTimestamp()
    {
        return timestamp;
    }

    std::string
    toString()
    {
        return "TM:" + timestamp + "-TX:"  + std::to_string(txBytes) + "-RX:" + std::to_string(rxBytes);
    }

private:
    std::string timestamp;

    //
    uint64_t txBytes;
    uint64_t rxBytes;


};
  
class NdnMapServer
{
public:
  NdnMapServer(char* programName)
  : m_programName(programName)
  , m_face(m_io)
  , m_scheduler(m_io)
  , m_terminationSignalSet(m_io)
  , m_statFile("stat")
  {
    m_mapServerAddr = "127.0.0.1";
    m_pollPeriod = 1;
    m_timeoutPeriod = 500;
    
    m_networkMonitor.reset(new util::NetworkMonitor(m_io));
    
  }
  
  void
  usage()
  {
    std::cout << "\n Usage:\n " << m_programName <<
    ""
    "[-h] -f link_file -n number_of_linkids [-s map_addr] [-t poll_period] [-r timeout_period] [-d debug_mode]\n"
    " Poll the status of remote clients and update ndnmap website with the status of the links."
    "\n"
    " The clients to pull from are specified in the input file, as well as their requested links "
    "\n"
    "  -h \t\t\t- print this message and exit"
    "\n"
    "  -f file_name \t\t- link_file is name of the file containing pairs associated with linkid."
    "\n"
    " \t\t\t   valid line format: <linkId> <interestPrefix> <LinkIP>"
    "\n"
    " \t\t\t   example: 1 /ndn/edu/arizona 192.168.1.3"
    "\n"
    "  -n number_of_linkids \t- supplied by the linkfile"
    "\n"
    "  -s map_addr \t\t- addr added to curl command for ndn map"
    "\n"
    "  -t poll_period \t- in seconds, default is 1 second"
    "\n"
    "  -r timeout_period \t- in milliseconds, default is 500 ms"
    "\n"
    "  -d debug mode \t-  1 set debug on, 0 set debug off (default)\n"
    "\n"
    "  -l statistics file \t- statistics file name \n" << std::endl;
    exit(1);
  }

  void
  onData(const ndn::Interest& interest, ndn::Data& data, std::string linkPrefix)
  {
    std::cout << "Data received for: " << interest.getName() << std::endl;
    CollectorData reply;
    
    reply.wireDecode(data.getContent().blockFromValue());
    
    if(reply.m_statusList.empty())
    {
      std::cerr << "received data is empty!!" << std::endl;
    }
    
    // get the list of the remote links requested for this prefix
    std::unordered_map<std::string,std::list<ndn::NdnMapServer::linkPair>>::const_iterator got = m_linksList.find(linkPrefix);
    
    if(got == m_linksList.end())
    {
      std::cerr << "failed to recognize the prefix: " << linkPrefix << std::endl;
    }
    else
    {
      if (DEBUG)
        std::cout << "findind link " << std::endl;
      std::list<ndn::NdnMapServer::linkPair> prefixList = got->second;
      
      for (unsigned i=0; i< reply.m_statusList.size(); i++)
      {
        shared_ptr<Statistics> stat;
        //int LinkId = 0;
        std::cout << "Reply: " << reply.m_statusList.at(i);
        
        // get the link id of the current IP
        for (auto pair = prefixList.cbegin(); pair != prefixList.cend(); ++pair)
        {
          if((*pair).linkIp == reply.m_statusList[i].getLinkIp())
          {
            if (DEBUG)
              std::cout << " Link ID for " << linkPrefix << " and " << reply.m_statusList[i].getLinkIp() << " is " << (*pair).linkId << std::endl;
            
            //LinkId = (*pair).linkId;
            stat = (*pair).stat;
          }
        }


        //ndn::time::system_clock::TimePoint statTime = ndn::time::fromString(stat->getTimestamp(), "%Y-%m-%dT%H:%M:%S%F");
        //ndn::time::system_clock::TimePoint newTime = ndn::time::fromString(reply.m_statusList[i].getTimestamp(), "%Y-%m-%dT%H:%M:%S%F");

        //if (newTime > statTime)
          stat->setBytes(reply.m_statusList[i].getTx() * 8, reply.m_statusList[i].getRx() * 8, reply.m_statusList[i].getTimestamp());

        if (DEBUG)
          std::cout << stat->toString() << std::endl;
        
        
      }
      reply.m_statusList.clear();
      
    }
        writeStatFile();
  }

  void
  writeStatFile() {

    if (DEBUG)
      std::cout << "writing data" << std::endl;
    std::ofstream statFile(m_statFile);
    if (statFile.is_open()) {
      for (std::pair<std::string,std::list<ndn::NdnMapServer::linkPair>> el : m_linksList) {     
        for (ndn::NdnMapServer::linkPair link : el.second) {   
          statFile << "LI:" << link.linkId << "-" << link.stat->toString() << std::endl;
        }
      }
      statFile.close();
    }
        if (DEBUG)
      std::cout << "end write" << std::endl;
  }
  
  void
  onTimeout(const ndn::Interest& interest)
  {
    std::cout << "onTimeout: " << interest.getName() << std::endl;
  }
  
  void
  sendInterests()
  {
    if(DEBUG)
    {
      auto now = std::chrono::system_clock::now();
      auto now_c = std::chrono::system_clock::to_time_t(now);
      std::cout << std::ctime(&now_c) << "about to send interests " <<std::endl;
    }
    for(auto it = m_linksList.begin(); it != m_linksList.end(); ++it)
    {
      std::list<linkPair> linkList = it->second;
      ndn::Name name(it->first+APP_SUFFIX);
      
      std::list<linkPair>::iterator itList;
      for (itList=linkList.begin(); itList!=linkList.end(); ++itList)
      {
        ndn::Name::Component dstIp((*itList).linkIp);
        name.append(dstIp);
      }
      ndn::Interest i(name);
      i.setInterestLifetime(ndn::time::milliseconds(m_timeoutPeriod));
      i.setMustBeFresh(true);
      
      m_face.expressInterest(i,
                             bind(&NdnMapServer::onData, this, _1, _2, it->first),
                             bind(&NdnMapServer::onTimeout, this, _1));
      
      //m_face.processEvents(ndn::time::milliseconds(m_timeoutPeriod));
      if(DEBUG)
        std::cout << "sent: " << name << std::endl;
    }
    // schedule the next fetch
    m_scheduler.scheduleEvent(time::seconds(m_pollPeriod), bind(&NdnMapServer::sendInterests, this));
  }
  void
  terminate(const boost::system::error_code& error, int signalNo)
  {
    if (error)
    {
      std::cout << "error code = " << error << ", signalNo = " << signalNo << std::endl;
      return;
    }
    m_io.stop();
  }
  void startScheduling()
  {
    // schedule the first event soon
    m_scheduler.scheduleEvent(time::milliseconds(100), bind(&NdnMapServer::sendInterests, this));
  }
  void run()
  {
    m_terminationSignalSet.async_wait(bind(&NdnMapServer::terminate, this, _1, _2));
    
    try
    {
      std::cout << m_programName <<  "polling every " << m_pollPeriod << " seconds" << std::endl;
      std::cout << m_programName <<  "timeout set to  " << m_timeoutPeriod << " milliseconds" << std::endl;
      
      m_io.run();
    }
    catch (std::exception& e)
    {
      std::cerr << "ERROR: " << e.what() << "\n" << std::endl;
      exit(1);
    }
    
  }
  void
  setMapServerAddr(std::string & addr)
  {
    m_mapServerAddr = addr;
  }
  void
  setPollPeriod(int period)
  {
    m_pollPeriod = period;
  }
  void
  setTimeoutPeriod(int to)
  {
    m_timeoutPeriod = to;
  }
  void
  setStatFile(std::string & statFile)
  {
    m_statFile = statFile;
  }
  
  class linkPair
  {
  public:
    linkPair() : stat(new Statistics()) {}
  public:
    int linkId;
    std::string linkIp;
    shared_ptr<Statistics> stat;
  };
  
  std::unordered_map<std::string,std::list<linkPair>> m_linksList;
  
private:
  std::string m_programName;
  boost::asio::io_service m_io;
  ndn::Face m_face;
  util::Scheduler m_scheduler;
  unique_ptr<util::NetworkMonitor> m_networkMonitor;
  boost::asio::signal_set m_terminationSignalSet;
  

  int m_pollPeriod;
  int m_timeoutPeriod;
  std::string m_mapServerAddr;

  std::string m_statFile;
  
};
}
int
main(int argc, char* argv[])
{
  ndn::NdnMapServer ndnmapServer(argv[0]);
  int option;
  std::fstream file;
  int num_lines = 0;
  
  // Parse cmd-line arguments
  while ((option = getopt(argc, argv, "hn:f:s:t:r:d:")) != -1)
  {
    switch (option)
    {
      case 'f':
        file.open(optarg, std::fstream::in);
        if (file == NULL)
        {
          std::cout << "cannot open file " << optarg << std::endl;
          ndnmapServer.usage();
        }
        break;
      case 'n':
        num_lines = atoi(optarg);
        break;
      case 's':
        ndnmapServer.setMapServerAddr((std::string&)(optarg));
        break;
      case 't':
        ndnmapServer.setPollPeriod(atoi(optarg));
        break;
      case 'r':
        ndnmapServer.setTimeoutPeriod(atoi(optarg));
        break;
      case 'd':
        DEBUG = atoi(optarg);
        break;
      case 'l':
        ndnmapServer.setStatFile((std::string&)(optarg));
        break;
      default:
      case 'h':
        ndnmapServer.usage();
        break;
    }
  }
  if (num_lines < 1)
  {
    ndnmapServer.usage();
    return 1;
  }
  // read link pairs from input file
  if(DEBUG)
    std::cout << "Rear from input file" << std::endl;

  for(int i = 0; i < num_lines; ++i)
  {
    ndn::NdnMapServer::linkPair linePair;
    std::string linkPrefix;
    file >> linePair.linkId >> linkPrefix >> linePair.linkIp;
    if(file.fail())
    {
      std::cout << "num_lines was incorrect too large" << std::endl;
    }
    else
    {
      if(DEBUG)
        std::cout << linkPrefix << ": " << linePair.linkId << ", " << linePair.linkIp << std::endl;

      std::unordered_map<std::string,std::list<ndn::NdnMapServer::linkPair>>::const_iterator got = ndnmapServer.m_linksList.find(linkPrefix);
      
      if(got == ndnmapServer.m_linksList.end())
      {
        std::list<ndn::NdnMapServer::linkPair> prefixList;
        prefixList.push_back(linePair);
        std::pair<std::string,std::list<ndn::NdnMapServer::linkPair>> pair(linkPrefix,prefixList);
        ndnmapServer.m_linksList.insert(pair);
      }
      else
      {
        std::list<ndn::NdnMapServer::linkPair> prefixList = got->second;
        prefixList.push_back(linePair);
        ndnmapServer.m_linksList.at(linkPrefix) = prefixList;
        
      }
    }
  }
  file.close();
  
  ndnmapServer.startScheduling();
  ndnmapServer.run();
  
  std::cout << "exit server..." << std::endl;
  return 0;
}
