//
//  NexusAdapter.h
//  tsflib
//
//  DbAdapter implementation for the Nexus Integra REST API.
//

#ifndef NexusAdapter_h
#define NexusAdapter_h

#include <stdio.h>
#include <map>
#include <vector>
#include <string>

#include "oatpp/web/client/HttpRequestExecutor.hpp"
#include "oatpp/network/tcp/client/ConnectionProvider.hpp"
#include "oatpp/network/ConnectionPool.hpp"
#include "oatpp-openssl/client/ConnectionProvider.hpp"
#include "oatpp-openssl/Config.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/network/virtual_/client/ConnectionProvider.hpp"
#include "oatpp/network/monitor/ConnectionMonitor.hpp"
#include "oatpp/network/monitor/ConnectionMaxAgeChecker.hpp"

#include "nlohmann/json.hpp"

#include "DbAdapter.h"
#include "NexusClient.hpp"

namespace TSF {

  class NexusAdapter : public DbAdapter {
  public:
    NexusAdapter(errCallback_t cb);
    ~NexusAdapter();

    // ── connection info ─────────────────────────────────────────
    class connectionInfo {
    public:
      connectionInfo();
      std::string proto, host, token;
      int port;
      std::string basePath;     // e.g. "/iotcore" for Gravitee-proxied access
      std::string resolution;   // e.g. "RES_1_MIN", "RES_1_HOUR"
      std::string dataSource;   // e.g. "RAW", "STATS_PER_HOUR"
      std::string graviteeApiKey; // X-Gravitee-Api-Key (optional, for proxied access)
    };
    connectionInfo conn;

    // ── DbAdapter interface ─────────────────────────────────────
    const adapterOptions options() const override;

    std::string connectionString() override;
    void setConnectionString(const std::string& con) override;
    void doConnect() override;

    IdentifierUnitsList idUnitsList() override;

    // TRANSACTIONS
    void beginTransaction() override;
    void endTransaction() override;
    bool inTransaction() override { return _inTransaction; };

    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range) override;
    Point selectNext(const std::string& id, time_t time, WhereClause q = WhereClause()) override;
    Point selectPrevious(const std::string& id, time_t time, WhereClause q = WhereClause()) override;

    // CREATE
    bool insertIdentifierAndUnits(const std::string& id, Units units) override;
    void insertSingle(const std::string& id, Point point) override;
    void insertRange(const std::string& id, std::vector<Point> points) override;

    // UPDATE
    bool assignUnitsToRecord(const std::string& name, const Units& units) override;

    // DELETE
    void removeRecord(const std::string& id) override;
    void removeAllRecords() override;

  private:
    typedef oatpp::web::protocol::http::incoming::Response Response;
    constexpr static const char* TAG = "NexusAdapter";

    std::shared_ptr<NexusClient> _restClient;
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> _objectMapper;

    // tag name <-> Nexus UID mappings
    std::map<std::string, std::string> _tagUidMap;   // name -> uid
    std::map<std::string, std::string> _uidTagMap;   // uid -> name
    IdentifierUnitsList _idCache;

    // transaction buffering
    bool _inTransaction;
    nlohmann::json _transactionBuffer; // array of NexusValue objects

    // helpers
    std::string apiPath();  // returns "api" or "basePath/api"
    std::shared_ptr<oatpp::web::client::RequestExecutor> createExecutor();
    nlohmann::json jsonFromResponse(const std::shared_ptr<Response> response);
    void refreshTagList();
    void flushTransactionBuffer();
    std::string uidForName(const std::string& name);
    std::vector<Point> pointsFromNexusValues(const nlohmann::json& arr);
  };

}

#endif /* NexusAdapter_h */

