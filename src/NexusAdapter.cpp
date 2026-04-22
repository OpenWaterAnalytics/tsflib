//
//  NexusAdapter.cpp
//  tsflib
//
//  DbAdapter implementation for the Nexus Integra REST API.
//

#include <regex>
#include <sstream>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "NexusAdapter.h"

using namespace std;
using namespace TSF;
using namespace oatpp;
using namespace oatpp::web;
using namespace oatpp::network;
using namespace nlohmann;


#include "oatpp/web/client/HttpRequestExecutor.hpp"





/****************************************************************************************************/
// connectionInfo defaults
/****************************************************************************************************/

NexusAdapter::connectionInfo::connectionInfo() {
  proto = "https";
  host = "";
  port = 443;
  token = "";
  basePath = "";
  resolution = "RES_1_MIN";
  dataSource = "RAW";
  graviteeApiKey = "";
}


/****************************************************************************************************/
// ctor / dtor
/****************************************************************************************************/

NexusAdapter::NexusAdapter(errCallback_t cb) : DbAdapter(cb) {
  _inTransaction = false;
  _connected = false;
  _transactionBuffer = json::array();
}

NexusAdapter::~NexusAdapter() {
}


/****************************************************************************************************/
// options
/****************************************************************************************************/

const DbAdapter::adapterOptions NexusAdapter::options() const {
  DbAdapter::adapterOptions o;
  o.supportsUnitsColumn    = false;
  o.canAssignUnits         = false;
  o.searchIteratively      = true;
  o.supportsSinglyBoundQuery = false;
  o.implementationReadonly = false;
  o.canDoWideQuery         = false;
  return o;
}


/****************************************************************************************************/
// connection string
/****************************************************************************************************/

std::string NexusAdapter::connectionString() {
  stringstream ss;
  ss << "proto="      << conn.proto
     << "&host="      << conn.host
     << "&port="      << conn.port
     << "&token="     << conn.token
     << "&resolution=" << conn.resolution
     << "&datasource=" << conn.dataSource;
  if (!conn.graviteeApiKey.empty()) {
    ss << "&gravitee=" << conn.graviteeApiKey;
  }
  if (!conn.basePath.empty()) {
    ss << "&basepath=" << conn.basePath;
  }
  return ss.str();
}

void NexusAdapter::setConnectionString(const std::string& str) {
  _TSF_DB_SCOPED_LOCK;

  regex kvReg("([^=]+)=([^&\\s]+)&?");
  map<string, string> kvPairs;
  {
    auto kv_begin = sregex_iterator(str.begin(), str.end(), kvReg);
    auto kv_end   = sregex_iterator();
    for (auto it = kv_begin; it != kv_end; ++it) {
      kvPairs[(*it)[1]] = (*it)[2];
    }
  }

  const map<string, function<void(string)>>
  kvSetters({
    {"proto",      [&](string v){ conn.proto = v; }},
    {"host",       [&](string v){ conn.host = v; }},
    {"port",       [&](string v){ conn.port = boost::lexical_cast<int>(v); }},
    {"token",      [&](string v){ conn.token = v; }},
    {"basepath",   [&](string v){ conn.basePath = v; }},
    {"resolution", [&](string v){ conn.resolution = v; }},
    {"datasource", [&](string v){ conn.dataSource = v; }},
    {"gravitee",   [&](string v){ conn.graviteeApiKey = v; }}
  });

  for (auto& kv : kvPairs) {
    if (kvSetters.count(kv.first) > 0) {
      kvSetters.at(kv.first)(kv.second);
    }
    else {
      cerr << "NexusAdapter: key not recognized: " << kv.first << " - skipping." << '\n' << flush;
    }
  }
}


/****************************************************************************************************/
// apiPath — build the oatpp PATH prefix: "api" or "basePath/api"
/****************************************************************************************************/

std::string NexusAdapter::apiPath() {
  if (conn.basePath.empty()) {
    return "api";
  }
  return conn.basePath + "/api";
}


/****************************************************************************************************/
// connect
/****************************************************************************************************/

shared_ptr<oatpp::web::client::RequestExecutor> NexusAdapter::createExecutor() {
  // virtual-host path for unit testing
  if (TSF_STRINGS_ARE_EQUAL(conn.host, "localhost") && conn.port == 0) {
    auto interface = oatpp::network::virtual_::Interface::obtainShared("virtualhost");
    auto clientConnectionProvider = oatpp::network::virtual_::client::ConnectionProvider::createShared(interface);
    return client::HttpRequestExecutor::createShared(clientConnectionProvider);
  }

  shared_ptr<ClientConnectionProvider> connectionProvider;
  if (TSF_STRINGS_ARE_EQUAL(conn.proto, "http")) {
    connectionProvider = oatpp::network::tcp::client::ConnectionProvider::createShared(
      {conn.host, (v_uint16)conn.port});
  }
  else {
    // default to https / TLS
    auto config = oatpp::openssl::Config::createShared();
    connectionProvider = oatpp::openssl::client::ConnectionProvider::createShared(
      config, {conn.host, (v_uint16)conn.port});
  }

  auto monitor = std::make_shared<oatpp::network::monitor::ConnectionMonitor>(connectionProvider);
  monitor->addMetricsChecker(
    std::make_shared<oatpp::network::monitor::ConnectionMaxAgeChecker>(
      std::chrono::seconds(20)));

  auto retryPolicy = std::make_shared<client::SimpleRetryPolicy>(
    5, std::chrono::seconds(5));

  auto connectionPool = oatpp::network::ClientConnectionPool::createShared(
    connectionProvider, 10, std::chrono::seconds(20));

  auto baseExecutor = client::HttpRequestExecutor::createShared(connectionPool, retryPolicy);
  return baseExecutor;
}

void NexusAdapter::doConnect() {
  _connected = false;
  _errCallback("Connecting...");
  
  if (this->conn.graviteeApiKey == "" && this->conn.token == "") {
    _errCallback("No tokens");
    return;
  }

  try {
    auto requestExecutor = createExecutor();
    auto objectMapper = oatpp::parser::json::mapping::ObjectMapper::createShared();
    _restClient  = NexusClient::createShared(requestExecutor, objectMapper);
    _objectMapper = objectMapper;
  }
  catch (const std::exception& e) {
    _errCallback(string("Failed to create HTTP client: ") + e.what());
    return;
  }

  // validate the connection by fetching the tag list
  try {
    refreshTagList();
  }
  catch (const std::exception& e) {
    _errCallback(string("Tag list fetch failed: ") + e.what());
    return;
  }

  _connected = true;
  _errCallback("OK");
  cout << "NexusAdapter: connected to " << conn.host << ":" << conn.port << EOL << flush;
}


/****************************************************************************************************/
// JSON response helper
/****************************************************************************************************/

json NexusAdapter::jsonFromResponse(const shared_ptr<Response> response) {
  lastError = "";
  if (response == nullptr) {
    lastError = "null response";
    return json();
  }
  int code = response->getStatusCode();
  string bodyStr = response->readBodyToString().getValue("");

  if (code == 200) {
    if (json::accept(bodyStr)) {
      return json::parse(bodyStr);
    }
    else {
      OATPP_LOGE(TAG, "JSON parse error: %s", bodyStr.c_str());
      lastError = bodyStr;
      return json();
    }
  }
  else {
    string desc(response->getStatusDescription()->c_str());
    cerr << TAG << ": HTTP " << code << " " << desc << " - " << bodyStr << endl;
    lastError = bodyStr;
    return json();
  }
}


/****************************************************************************************************/
// tag list refresh
/****************************************************************************************************/

void NexusAdapter::refreshTagList() {
  _TSF_DB_SCOPED_LOCK;

  auto response = _restClient->getTags(apiPath(), conn.token, conn.graviteeApiKey);
  json tagsJson = jsonFromResponse(response);

  if (!tagsJson.is_array()) {
    throw std::runtime_error("Unexpected response from GET /api/Tags");
  }

  _tagUidMap.clear();
  _uidTagMap.clear();
  IdentifierUnitsList ids;

  for (auto& tag : tagsJson) {
    if (!tag.contains("name") || !tag.contains("uid")) continue;
    string name = tag["name"].get<string>();
    string uid  = tag["uid"].get<string>();
    auto units = TSF_NO_UNITS;
    if (tag.contains("attributes")) {
      auto attr = tag["attributes"];
      if (attr.is_array()) {
        for (auto attr_dict : attr) {
          if (attr_dict.contains("attributeName")
              && attr_dict["attributeName"].is_string()
              && attr_dict.contains("value")
              && attr_dict["value"].is_string() ) {
            
            // two options: alias and units
            if (attr_dict["attributeName"].get<string>() == "units") {
              auto units_str = attr_dict["value"].get<string>();
              units = Units::unitOfType(units_str);
            }
            else if (attr_dict["attributeName"].get<string>() == "alias") {
              name = attr_dict["value"].get<string>();
            }
            
          }
        }
      }
    }
    
    _tagUidMap[name] = uid;
    _uidTagMap[uid]  = name;
    ids.set(name, units);
  }
  _idCache = ids;
}

string NexusAdapter::uidForName(const string& name) {
  if (_tagUidMap.count(name) > 0) {
    return _tagUidMap.at(name);
  }
  cerr << TAG << ": no Nexus UID for tag name: " << name << endl;
  return "";
}


/****************************************************************************************************/
// idUnitsList
/****************************************************************************************************/

IdentifierUnitsList NexusAdapter::idUnitsList() {
  if (_inTransaction) {
    return _idCache;
  }
  try {
    refreshTagList();
  }
  catch (const std::exception& e) {
    cerr << TAG << ": error refreshing tag list: " << e.what() << endl;
  }
  return _idCache;
}


/****************************************************************************************************/
// pointsFromNexusValues — parse [{uid, value, timeStamp}, …] into vector<Point>
/****************************************************************************************************/

vector<Point> NexusAdapter::pointsFromNexusValues(const json& arr) {
  vector<Point> points;
  if (!arr.is_array()) return points;
  points.reserve(arr.size());

  for (auto& nv : arr) {
    if (nv.is_null()) continue;
    if (!nv.contains("timeStamp") || !nv.contains("value")) continue;
    if (nv["value"].is_null()) continue;

    time_t t = static_cast<time_t>(nv["timeStamp"].get<double>());
    double v = nv["value"].get<double>();
    points.emplace_back(t, v, Point::opc_tsf_override, 0);
  }

  // ensure chronological order
  std::sort(points.begin(), points.end(), Point::comparePointTime);
  return points;
}


/****************************************************************************************************/
// READ
/****************************************************************************************************/

vector<Point> NexusAdapter::selectRange(const string& id, TimeRange range) {
  string uid = uidForName(id);
  if (uid.empty()) return {};

  json body;
  body["uids"]       = json::array({uid});
  body["startTs"]    = static_cast<double>(range.start);
  body["endTs"]      = static_cast<double>(range.end);
  body["dataSource"] = conn.dataSource;
  body["resolution"] = conn.resolution;

  json result;
  try {
    auto response = _restClient->getHistoric(apiPath(), conn.token, conn.graviteeApiKey, body.dump());
    result = jsonFromResponse(response);
  }
  catch (const std::exception& e) {
    cerr << TAG << ": selectRange error: " << e.what() << endl;
    return {};
  }

  return pointsFromNexusValues(result);
}

Point NexusAdapter::selectNext(const string& id, time_t time, WhereClause q) {
  // not used — searchIteratively=true, supportsSinglyBoundQuery=false
  return Point();
}

Point NexusAdapter::selectPrevious(const string& id, time_t time, WhereClause q) {
  // not used — searchIteratively=true, supportsSinglyBoundQuery=false
  return Point();
}


/****************************************************************************************************/
// TRANSACTIONS
/****************************************************************************************************/

void NexusAdapter::beginTransaction() {
  if (_inTransaction) return;
  _inTransaction = true;
  {
    _TSF_DB_SCOPED_LOCK;
    _transactionBuffer = json::array();
  }
}

void NexusAdapter::endTransaction() {
  if (!_inTransaction) return;
  flushTransactionBuffer();
  _inTransaction = false;
}

void NexusAdapter::flushTransactionBuffer() {
  _TSF_DB_SCOPED_LOCK;
  if (_transactionBuffer.empty()) return;

  try {
    auto response = _restClient->insertHistoric(apiPath(), conn.token, conn.graviteeApiKey, _transactionBuffer.dump());
    auto result = jsonFromResponse(response);
    // response is simply `true` on success — no further validation
  }
  catch (const std::exception& e) {
    cerr << TAG << ": flushTransactionBuffer error: " << e.what() << endl;
  }
  _transactionBuffer = json::array();
}


/****************************************************************************************************/
// CREATE
/****************************************************************************************************/

bool NexusAdapter::insertIdentifierAndUnits(const string& id, Units units) {
  // POST /api/Tags/insert with body ["tagname"]
  json body = json::array({id});

  try {
    auto response = _restClient->insertTags(apiPath(), conn.token, conn.graviteeApiKey, body.dump());
    json result = jsonFromResponse(response);

    if (result.is_array()) {
      for (auto& tag : result) {
        if (!tag.contains("name") || !tag.contains("uid")) continue;
        string name = tag["name"].get<string>();
        string uid  = tag["uid"].get<string>();
        _tagUidMap[name] = uid;
        _uidTagMap[uid]  = name;
      }
    }
  }
  catch (const std::exception& e) {
    cerr << TAG << ": insertIdentifierAndUnits error: " << e.what() << endl;
    return false;
  }

  _idCache.set(id, units);
  return true;
}

void NexusAdapter::insertSingle(const string& id, Point point) {
  insertRange(id, {point});
}

void NexusAdapter::insertRange(const string& id, vector<Point> points) {
  if (points.empty()) return;

  string uid = uidForName(id);
  if (uid.empty()) return;

  json values = json::array();
  for (auto& p : points) {
    json nv;
    nv["Uid"]       = uid;
    nv["Value"]     = p.value;
    nv["TimeStamp"] = static_cast<double>(p.time);
    values.push_back(nv);
  }

  if (_inTransaction) {
    _TSF_DB_SCOPED_LOCK;
    for (auto& v : values) {
      _transactionBuffer.push_back(v);
    }
    return;
  }

  // immediate send
  try {
    auto response = _restClient->insertHistoric(apiPath(), conn.token, conn.graviteeApiKey, values.dump());
    jsonFromResponse(response);
  }
  catch (const std::exception& e) {
    cerr << TAG << ": insertRange error: " << e.what() << endl;
  }
}


/****************************************************************************************************/
// UPDATE — not supported
/****************************************************************************************************/

bool NexusAdapter::assignUnitsToRecord(const string& name, const Units& units) {
  return false;
}


/****************************************************************************************************/
// DELETE — not supported by Nexus API
/****************************************************************************************************/

void NexusAdapter::removeRecord(const string& id) {
  cerr << TAG << ": removeRecord not supported by Nexus API" << endl;
}

void NexusAdapter::removeAllRecords() {
  cerr << TAG << ": removeAllRecords not supported by Nexus API" << endl;
}

