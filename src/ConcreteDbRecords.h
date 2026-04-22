#ifndef __tsflib__ConcreteDbRecords__
#define __tsflib__ConcreteDbRecords__

#include <iostream>
#include <list>

#include "DbPointRecord.h"
#include "PointRecordTime.h"

namespace TSF {
  
  class SqlitePointRecord : public DbPointRecord {
  public:
    TSF_BASE_PROPS(SqlitePointRecord);
    SqlitePointRecord();
    virtual ~SqlitePointRecord();
    
    std::string basePath();
    void setBasePath(const std::string& path);
    
    bool supportsQualifiedQuery() { return true; };
  };
  
  
  
  class InfluxDbPointRecord : public DbPointRecord {
  public:
    TSF_BASE_PROPS(InfluxDbPointRecord);
    InfluxDbPointRecord();
    ~InfluxDbPointRecord();
    bool supportsQualifiedQuery() { return true; };
    void sendInfluxString(time_t time, const string& seriesId, const string& values);
  };
  
  
  class InfluxUdpPointRecord : public DbPointRecord {
  public:
    TSF_BASE_PROPS(InfluxUdpPointRecord);
    InfluxUdpPointRecord();
    ~InfluxUdpPointRecord();
    
    void sendInfluxString(time_t time, const string& seriesId, const string& values);
  };
  

  class NexusPointRecord : public DbPointRecord {
  public:
    TSF_BASE_PROPS(NexusPointRecord);
    NexusPointRecord();
    ~NexusPointRecord();
  };

}

#endif /* defined(__tsflib__ConcreteDbRecords__) */
