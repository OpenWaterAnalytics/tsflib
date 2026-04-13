#ifndef InfluxAdapter_hpp
#define InfluxAdapter_hpp

#include <stdio.h>
#include <future>
#include <thread>
#include <memory>

#include "httplib.h"
#include "nlohmann/json.hpp"

#include "DbAdapter.h"

namespace TSF {
  class InfluxAdapter : public DbAdapter {
  public:
    InfluxAdapter( errCallback_t cb );
    virtual ~InfluxAdapter();
        
    void setConnectionString(const std::string& con);

    class connectionInfo {
    public:
      connectionInfo();
      std::string proto, host, user, pass, db;
      int port;
      int msec_ratelimit;
      bool validate;
      std::string getAuthString(){ return user + ":" + pass; }
    };
    connectionInfo conn;
    
    // TRANSACTIONS
    void beginTransaction();
    void endTransaction();
    bool inTransaction() {return _inTransaction;};
    
    // CREATE
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    
    // UPDATE
    bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    // utility only for influx
    void sendInfluxString(time_t time, const std::string& seriesId, const std::string& values);
    
  protected:
    // compulsory overrides
    virtual std::string formatTimestamp(time_t t) = 0;
    virtual void sendPointsWithString(const std::string& content) = 0;
    virtual size_t maxTransactionLines() = 0;
    
    // sub types
    std::vector<std::string> insertionLinesFromPoints(const std::string& tsName, std::vector<Point> points);
    std::string influxIdForTsId(const std::string& id);
    
    std::vector<std::string> _transactionLines;
    IdentifierUnitsList _idCache;
    bool _inTransaction;
    
  private:
    void commitTransactionLines();
  };
  
  class ITaskWrapper {
    // empty implementation for async task sub
  };
  
  class InfluxTcpAdapter : public InfluxAdapter {
  public:
    
    InfluxTcpAdapter( errCallback_t cb );
    ~InfluxTcpAdapter();
    
    const adapterOptions options() const;
    std::string connectionString();
    void doConnect();
    IdentifierUnitsList idUnitsList();
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time, WhereClause q = WhereClause());
    Point selectPrevious(const std::string& id, time_t time, WhereClause q = WhereClause());
    std::vector<Point> selectWithQuery(const std::string& query, TimeRange range);

    
    // PREFETCH
    std::map<std::string, std::vector<Point> > wideQuery(TimeRange range);
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
  protected:
    size_t maxTransactionLines();
    void sendPointsWithString(const std::string& content);
    std::string formatTimestamp(time_t t);
    
  private:
    class Query {
    public:
      std::vector<std::string> select,where;
      std::string from,order,groupBy;
      std::string selectStr();
      std::string nameAndWhereClause();
    };
    
    std::shared_ptr<ITaskWrapper> _sendTask;
    constexpr static const char* TAG = "InfluxTCPAdapter";
    std::unique_ptr<httplib::Client> _httpClient;
    void createHttpClient();
    std::future<void> sendPointsFuture;

    Query queryPartsFromMetricId(const std::string& name);
    
    nlohmann::json executeQuery(const std::string& query, const std::string& epoch = "");
    nlohmann::json jsonFromResponseBody(int statusCode, const std::string& body);

    static std::map<std::string, std::vector<Point> > __pointsFromJson(nlohmann::json& json);
    static std::vector<Point> __pointsSingle(nlohmann::json& json);
  };
  
  
  class InfluxUdpAdapter : public InfluxAdapter {
  public:
    InfluxUdpAdapter( errCallback_t cb );
    ~InfluxUdpAdapter();
    
    const adapterOptions options() const;
    std::string connectionString();
    void doConnect();
    IdentifierUnitsList idUnitsList();
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time, WhereClause q = WhereClause());
    Point selectPrevious(const std::string& id, time_t time, WhereClause q = WhereClause());
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
  protected:
    size_t maxTransactionLines();
    void sendPointsWithString(const std::string& content);
    std::string formatTimestamp(time_t t);
    
  private:
    std::future<void> _sendFuture;
  };
  
  
  
}



#endif /* InfluxAdapter_hpp */
