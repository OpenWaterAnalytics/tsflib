#ifndef OpcPointRecord_hpp
#define OpcPointRecord_hpp

#include <stdio.h>

#include <ua_client.h>

#include "DbPointRecord.h"

namespace TSF {
  class OpcPointRecord : public DbPointRecord {
  public:
    TSF_BASE_PROPS(OpcPointRecord);
    OpcPointRecord();
    
    bool supportsUnitsColumn() {return false;};
    
    // stubs
    void truncate() {};
    bool supportsSinglyBoundedQueries() {return false;};
    bool shouldSearchIteratively() {return false;};
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time) {};
    Point selectPrevious(const std::string& id, time_t time) {};
    
    // insertions or alterations: may choose to ignore / deny
    void insertSingle(const std::string& id, Point point) {};
    void insertRange(const std::string& id, std::vector<Point> points) {};
    void removeRecord(const std::string& id) {};
    bool insertIdentifierAndUnits(const std::string& id, Units units) {};
    
  protected:
    void doConnect() throw(TsfException);
    
  private:
    void refreshIds();
    void parseConnectionString(const std::string& str);
    std::string serializeConnectionString();
    
    std::string _endpoint;
    UA_Client *_client;
//    std::map<string,OpcUa::Node> _nodes;
  };
}

#endif /* OpcPointRecord_hpp */
