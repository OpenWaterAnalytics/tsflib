#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/asio.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>

#include "InfluxAdapter.h"

#include "MetricInfo.h"

#define TSF_INFLUX_CLIENT_TIMEOUT 30

using namespace std;
using namespace TSF;
using namespace nlohmann;

// Resolve ambiguity with macOS MacTypes.h Point
using TSF::Point;


/***************************************************************************************/
InfluxAdapter::connectionInfo::connectionInfo() {
  proto = "HTTP";
  host = "localhost";
  user = "USER";
  pass = "PASS";
  db = "DB";
  port = 0;
  validate = true;
  msec_ratelimit = 0;
}
/***************************************************************************************/

InfluxAdapter::InfluxAdapter( errCallback_t cb ) : DbAdapter(cb) {
  _inTransaction = false;
  _connected = false;
}
InfluxAdapter::~InfluxAdapter() {
  
}


void InfluxAdapter::setConnectionString(const std::string& str) {
  _TSF_DB_SCOPED_LOCK;
  
  regex kvReg("([^=]+)=([^&\\s]+)&?"); // key - value pair
  // split the tokenized string. we're expecting something like "host=127.0.0.1&port=4242"
  std::map<std::string, std::string> kvPairs;
  {
    auto kv_begin = sregex_iterator(str.begin(), str.end(), kvReg);
    auto kv_end = sregex_iterator();
    for (auto it = kv_begin ; it != kv_end; ++it) {
      kvPairs[(*it)[1]] = (*it)[2];
    }
  }
  
  const map<string, function<void(string)> > 
  kvSetters({
    {"proto", [&](string v){this->conn.proto = v;}},
    {"host", [&](string v){this->conn.host = v;}},
    {"port", [&](string v){this->conn.port = boost::lexical_cast<int>(v);}},
    {"db", [&](string v){this->conn.db = v;}},
    {"u", [&](string v){this->conn.user = v;}},
    {"p", [&](string v){this->conn.pass = v;}},
    {"validate", [&](string v){this->conn.validate = boost::lexical_cast<bool>(v);}},
    {"ratelimit", [&](string v){this->conn.msec_ratelimit = boost::lexical_cast<int>(v);}}
  });

  for (auto kv : kvPairs) {
    if (kvSetters.count(kv.first) > 0) {
      kvSetters.at(kv.first)(kv.second);
    }
    else {
      cerr << "key not recognized: " << kv.first << " - skipping." << '\n' << flush;
    }
  }
  
  return;
}



void InfluxAdapter::beginTransaction() {
  if (_inTransaction) {
    return;
  }
  _inTransaction = true;
  {
    _TSF_DB_SCOPED_LOCK;
    _transactionLines.clear();
  }
}
void InfluxAdapter::endTransaction() {
  if (!_inTransaction) {
    return;
  }
  this->commitTransactionLines();
  _inTransaction = false;
}

void InfluxAdapter::commitTransactionLines() {
  {
    _TSF_DB_SCOPED_LOCK;
    if (_transactionLines.size() == 0) {
      return;
    }
  }
  {
    string concatLines;
    _TSF_DB_SCOPED_LOCK;
    auto curs = _transactionLines.begin();
    auto end = _transactionLines.end();
    size_t iLine = 0;
    while (curs != end) {
      const string str = *curs;
      ++iLine;
      concatLines.append(str);
      concatLines.append("\n");
      // check for max lines (must respect max lines per send event)
      if (iLine >= this->maxTransactionLines()) {
        // push these lines and clear the queue
        this->sendPointsWithString(concatLines);
        concatLines.clear();
        iLine = 0;
      }
      ++curs;
    }
    if (iLine > 0) {
      // push all/remaining points out
      this->sendPointsWithString(concatLines);
    }
    _transactionLines.clear();
  }
}


bool InfluxAdapter::insertIdentifierAndUnits(const std::string &id, TSF::Units units) {
  
  MetricInfo m(id);
  m.tags.erase("units"); // get rid of units if they are included.
  string properId = m.name();
  
  _idCache.set(properId, units);
  
  // insert a field key/value for something that we won't ever query again.
  // pay attention to bulk operations here, since we may be inserting new ids en-masse
  string tsNameEscaped = influxIdForTsId(id);
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  const string content(tsNameEscaped + " exist=true");
  if (_inTransaction) {
    _TSF_DB_SCOPED_LOCK;
    _transactionLines.push_back(content);
  }
  else {
    this->sendPointsWithString(content);
  }
  // no futher validation.
  return true;
}


void InfluxAdapter::insertSingle(const std::string& id, Point point) {
  this->insertRange(id, {point});
}

void InfluxAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  string dbId = influxIdForTsId(id);
  auto content = this->insertionLinesFromPoints(dbId, points);
  
  if (_inTransaction) {
    size_t nLines = 0;
    { // mutex
      _TSF_DB_SCOPED_LOCK;
      for (auto s : content) {
        _transactionLines.push_back(s);
      }
      nLines = _transactionLines.size();
    } // end mutex
    if (nLines > maxTransactionLines()) {
      this->commitTransactionLines();
    }
  }
  else {
    _transactionLines = content;
    this->commitTransactionLines();
  }
  
}


void InfluxAdapter::sendInfluxString(time_t time, const string& seriesId, const string& values) {
  
  string tsNameEscaped = seriesId;
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  
  stringstream ss;
  string timeStr = this->formatTimestamp(time);
  
  ss << tsNameEscaped << " " << values << " " << timeStr;
  string data = ss.str();
  
  
  if (_inTransaction) {
    size_t nLines = 0;
    {
      _TSF_DB_SCOPED_LOCK;
      _transactionLines.push_back(data);
      nLines = _transactionLines.size();
    }
    if (nLines > maxTransactionLines()) {
      this->commitTransactionLines();
    }
  }
  else {
    this->sendPointsWithString(data);
  }
}

string InfluxAdapter::influxIdForTsId(const string& id) {
  // sort named keys into proper order...
  MetricInfo m(id);
  if (m.tags.count("units")) {
    m.tags.erase("units");
  }
  string tsId = m.name();
  if (_idCache.get()->count(tsId) == 0) {
    cerr << "no registered ts with that id: " << tsId << endl;
    // yet i'm being asked for it??
    return "";
  }
  m.tags["units"] = (*_idCache.get())[tsId].second;
  return m.name();
}


vector<string> InfluxAdapter::insertionLinesFromPoints(const string& tsName, vector<Point> points) {
  /*
   As you can see in the example below, you can post multiple points to multiple series at the same time by separating each point with a new line. Batching points in this manner will result in much higher performance.
   
   curl -i -XPOST 'http://localhost:8086/write?db=mydb' --data-binary '
   cpu_load_short,host=server01,region=us-west value=0.64
   cpu_load_short,host=server02,region=us-west value=0.55 1422568543702900257
   cpu_load_short,direction=in,host=server01,region=us-west value=23422.0 1422568543702900257'
   */
  
  // escape any spaces in the tsName
  string tsNameEscaped = tsName;
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  vector<string> outData;
  
  for(const Point& p: points) {
    stringstream ss;
    string valueStr = to_string(p.value); // influxdb 0.10+ supports integers, but only when followed by trailing "i"
    string timeStr = this->formatTimestamp(p.time);
    ss << tsNameEscaped << " value=" << valueStr << "," << "quality=" << (int)p.quality << "i," << "confidence=" << p.confidence << " " << timeStr;
    outData.push_back(ss.str());
  }
  
  return outData;
}

bool InfluxAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  return false;
}



/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
const char *kSERIES = "series";
const char *kSHOW_SERIES = "show series";
const char *kERROR = "error";
const char *kRESULTS = "results";
// INFLUX TCP


std::string InfluxTcpAdapter::Query::selectStr() {
  stringstream ss;
  ss << "SELECT ";
  if (this->select.size() == 0) {
    ss << "*";
  }
  else {
    ss << boost::algorithm::join(this->select,", ");
  }
  ss << " FROM " << this->nameAndWhereClause();
  if (this->groupBy.length() > 0) {
    ss << " GROUP BY " << this->groupBy;
  }
  if (this->order.length() > 0) {
    ss << " ORDER BY " << this->order;
  }
  return ss.str();
}
std::string InfluxTcpAdapter::Query::nameAndWhereClause() {
  stringstream ss;
  ss << this->from;
  if (this->where.size() > 0) {
    ss << " WHERE " << boost::algorithm::join(this->where," AND ");
  }
  return ss.str();
}




InfluxTcpAdapter::InfluxTcpAdapter( errCallback_t cb) : InfluxAdapter(cb) {
}


InfluxTcpAdapter::~InfluxTcpAdapter() {
  
}

void InfluxTcpAdapter::createHttpClient() {
  string baseUrl;
  if (TSF_STRINGS_ARE_EQUAL(this->conn.proto, "https")) {
    baseUrl = "https://" + this->conn.host + ":" + to_string(this->conn.port);
  }
  else {
    baseUrl = "http://" + this->conn.host + ":" + to_string(this->conn.port);
  }
  _httpClient = std::make_unique<httplib::Client>(baseUrl);
  _httpClient->set_connection_timeout(TSF_INFLUX_CLIENT_TIMEOUT);
  _httpClient->set_read_timeout(TSF_INFLUX_CLIENT_TIMEOUT);
  _httpClient->set_basic_auth(this->conn.user, this->conn.pass);
  _httpClient->set_keep_alive(true);
}

const DbAdapter::adapterOptions InfluxTcpAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = true;
  o.supportsSinglyBoundQuery = true;
  o.searchIteratively = false;
  o.canAssignUnits = false;
  o.implementationReadonly = false;
  o.canDoWideQuery = true;
  
  return o;
}

std::string InfluxTcpAdapter::connectionString() {
  stringstream ss;
  ss << "proto=" << this->conn.proto
      << "&host=" << this->conn.host
      << "&port=" << this->conn.port
      << "&db=" << this->conn.db
      << "&u=" << this->conn.user
      << "&p=" << this->conn.pass
      << "&validate=" << (this->conn.validate ? 1 : 0);
  return ss.str();
}

void InfluxTcpAdapter::doConnect() { 
  _connected = false;
  _errCallback("Connecting...");
  
  createHttpClient();

  // see if the database needs to be created
  bool dbExists = false;
  
  string q("SHOW SERIES LIMIT 1");
  json jsoMeas;
  try {
    jsoMeas = executeQuery(q);
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  if (!jsoMeas.contains(kRESULTS)) {
    if (jsoMeas.contains("error")) {
      _errCallback(jsoMeas["error"]);
      return;
    }
    else {
      _errCallback("Connect failed: No Database?");
      throw(std::invalid_argument("Bad database connection."));
      return;
    }
  }
  
  json resVal = jsoMeas[kRESULTS];
  if (!resVal.is_array() || resVal.size() == 0) {
    _errCallback("JSON Format Not Recognized");
    return;
  }
  
  // for sure it's an array.
  if (resVal.size() > 0 && resVal[0].contains(kERROR)) {
    _errCallback(resVal[0][kERROR]);
  }
  else {
    dbExists = true;
  }
  
  
  if (!dbExists) {
    string createQ("CREATE DATABASE " + this->conn.db);
    // Use POST /query for database creation (no db param needed)
    httplib::Params params = {{"q", createQ}};
    auto res = _httpClient->Post("/query", httplib::Headers{}, params);
    json js = json::object();
    if (res && res->status == 200) {
      if (json::accept(res->body)) {
        js = json::parse(res->body);
      }
    }
    if (js.size() == 0 || !js.contains(kRESULTS) ) {
      _errCallback("Can't create database");
      return;
    }
  }
  
  // made it this far? at least we are connected.
  _connected = true;
  _errCallback("OK");
  
  cout << "influx connector: " << this->conn.db << " connected? " << (_connected ? "yes" : "NO") << EOL << flush;
  
  return;

}


nlohmann::json InfluxTcpAdapter::executeQuery(const std::string& query, const std::string& epoch) {
  httplib::Params params = {{"db", this->conn.db}, {"q", query}};
  if (!epoch.empty()) {
    params.emplace("epoch", epoch);
  }

  auto res = _httpClient->Get("/query", params, httplib::Headers{});
  if (!res) {
    cerr << TAG << ": HTTP request failed (no response)" << endl;
    return json::object();
  }
  return jsonFromResponseBody(res->status, res->body);
}


IdentifierUnitsList InfluxTcpAdapter::idUnitsList() {
  
  IdentifierUnitsList ids;
  
  // if i'm busy, then don't bother. unless this could be the first query.
  if (_inTransaction) {
    return _idCache;
  }
  _TSF_DB_SCOPED_LOCK;
  
  json jsv;
  try {
    jsv = executeQuery(kSHOW_SERIES);
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  
  
  if (jsv.contains(kRESULTS) &&
      jsv[kRESULTS].is_array() &&
      jsv[kRESULTS].size() > 0 &&
      jsv[kRESULTS][0].contains(kSERIES) &&
      jsv[kRESULTS][0][kSERIES].is_array() ) 
  {
    json seriesArray = jsv["results"][0][kSERIES];
    for (auto seriesIt = seriesArray.begin(); seriesIt != seriesArray.end(); ++seriesIt) {
      
      string str = seriesIt->dump();
      json thisSeries = *seriesIt;
      
      json columns = thisSeries["columns"]; // the only column is "key"
      json values = thisSeries["values"]; // array of single-string arrays
      
      for (auto valuesIt = values.begin(); valuesIt != values.end(); ++valuesIt) {
        json singleStrArr = *valuesIt;
        json dbIdStr = singleStrArr.at(0);
        string dbId = dbIdStr.get<string>();
        boost::replace_all(dbId, "\\ ", " ");
        MetricInfo m(dbId);
        // now we have all kv pairs that define a time series.
        // do we have units info? strip it off before showing the user.
        Units units = TSF_NO_UNITS;
        if (m.tags.count("units") > 0) {
          units = Units::unitOfType(m.tags["units"]);
          // remove units from string name.
          m.tags.erase("units");
        }
        // now assemble the complete name and cache it:
        string properId = m.name();
        ids.set(properId,units);
      } // for each values array (ts definition)
    }
  }
  // else nothing
  _idCache = ids;
  return ids;
}


std::map<std::string, std::vector<TSF::Point> > InfluxTcpAdapter::wideQuery(TimeRange range) {

  vector<string> fields({"time", "value", "quality", "confidence"});
  vector<string> where({"time >= " + to_string(range.start) + "s", "time <= " + to_string(range.end) + "s"});
  
  stringstream ss;
  ss << "SELECT ";
  ss << boost::algorithm::join(fields,", ");
  ss << " FROM /.*/";
  ss << " WHERE " << boost::algorithm::join(where," AND ");
  ss << " GROUP BY * ORDER BY ASC";
  
  string prevQuery = "SELECT time, value, quality, confidence FROM /.*/ WHERE time < " + to_string(range.start) + "s GROUP BY * order by time desc limit 1";
  string nextQuery = "SELECT time, value, quality, confidence FROM /.*/ WHERE time > " + to_string(range.end) + "s GROUP BY * order by time asc limit 1";
  
  auto qstr = prevQuery + ";" + ss.str() + ";" + nextQuery;
  json jsv;
  try {
    jsv = executeQuery(qstr, "s");
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  
  
  map<string, vector<TSF::Point> > fetch = __pointsFromJson(jsv);
  return fetch;
}

// READ
std::vector<TSF::Point> InfluxTcpAdapter::selectRange(const std::string& id, TimeRange range) {

  string dbId = influxIdForTsId(id);
  InfluxTcpAdapter::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time >= " + to_string(range.start) + "s");
  q.where.push_back("time <= " + to_string(range.end) + "s");
  
  json jsv;
  try {
    jsv = executeQuery(q.selectStr(), "s");
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  
  return __pointsSingle(jsv);
}

vector<string> _makeSelectStrs(WhereClause q);
vector<string> _makeSelectStrs(WhereClause q) {
  vector<string> clauses;
  for (auto const& i : q.clauses) {
    switch (i.first) {
      case WhereClause::gte:
        clauses.push_back("value >= " + std::to_string(i.second));
        break;
      case WhereClause::gt:
        clauses.push_back("value > " + std::to_string(i.second));
        break;
      case WhereClause::lte:
        clauses.push_back("value <= " + std::to_string(i.second));
        break;
      case WhereClause::lt:
        clauses.push_back("value < " + std::to_string(i.second));
        break;
      default:
        break;
    }
  }
  return clauses;
}

TSF::Point InfluxTcpAdapter::selectNext(const std::string& id, time_t time, WhereClause whereClause) {

  std::vector<TSF::Point> points;
  string dbId = influxIdForTsId(id);
  Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time > " + to_string(time) + "s");
  q.order = "time asc limit 1";
  
  if (!whereClause.clauses.empty()) {
    auto otherWheres = _makeSelectStrs(whereClause);
    for (auto w : otherWheres) {
      q.where.push_back(w);
    }
  }
  
  json jsv;
  try {
    jsv = executeQuery(q.selectStr(), "s");
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  
  points = __pointsSingle(jsv);
  
  if (points.size() == 0) {
    return TSF::Point();
  }
  
  return points.front();
}

TSF::Point InfluxTcpAdapter::selectPrevious(const std::string& id, time_t time, WhereClause whereClause) {

  std::vector<TSF::Point> points;
  string dbId = influxIdForTsId(id);
  
  Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time < " + to_string(time) + "s");
  q.order = "time desc limit 1";
  
  if (!whereClause.clauses.empty()) {
    auto otherWheres = _makeSelectStrs(whereClause);
    for (auto w : otherWheres) {
      q.where.push_back(w);
    }
  }
  
  json jsv;
  try {
    jsv = executeQuery(q.selectStr(), "s");
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  
  points = __pointsSingle(jsv);
  
  if (points.size() == 0) {
    return TSF::Point();
  }
  
  return points.front();
}



vector<TSF::Point> InfluxTcpAdapter::selectWithQuery(const std::string& query, TimeRange range) {
  // expects a "$timeFilter" placeholder in the string arg,
  // to be replaced with the time range, e.g., "time >= t1 and time <= t2"
  
  //case insensitive find
  if (boost::ifind_first(query, std::string("$timeFilter")).empty()) {
    // add WHERE clause
    return vector<TSF::Point>();
  }
  
  string qStr = query;
  
  stringstream tfss;
  if (range.start > 0) {
    tfss << "time >= " << range.start << "s";
  }
  if (range.start > 0 && range.end > 0) {
    tfss << " and ";
  }
  if (range.end > 0) {
    tfss << "time <= " << range.end << "s";
  }
  string timeFilter = tfss.str();
  
  boost::replace_all(qStr, "$timeFilter", timeFilter);
  
  if (range.start == 0) {
    qStr += " order by desc limit 1";
  }
  else if (range.end == 0) {
    qStr += " order by asc limit 1";
  }
  else {
    qStr += " order by asc";
  }
  json jsv;
  try {
    jsv = executeQuery(qStr, "s");
  } catch (const std::exception &err) {
    cerr << "error executing query: " << err.what() << endl;
  }
  
  auto points = __pointsSingle(jsv);
  return points;
}

// DELETE
void InfluxTcpAdapter::removeRecord(const std::string& id) {
  const string dbId = this->influxIdForTsId(id);
  Query q = this->queryPartsFromMetricId(id);
  
  stringstream sqlss;
  sqlss << "DROP SERIES FROM " << q.nameAndWhereClause();
  string qStr = sqlss.str();
  httplib::Params params = {{"q", qStr}};
  _httpClient->Post("/query", httplib::Headers{}, params);
}

void InfluxTcpAdapter::removeAllRecords() {
  
  _errCallback("Truncating");
  cerr << TAG << ": Truncating" << endl;

  auto ids = this->idUnitsList();
  
  stringstream sqlss;
  sqlss << "DROP DATABASE " << this->conn.db << "; CREATE DATABASE " << this->conn.db;
  string qStr(sqlss.str());
  httplib::Params params = {{"q", qStr}};
  auto res = _httpClient->Post("/query", httplib::Headers{}, params);
  json v = json::object();
  if (res && res->status == 200 && json::accept(res->body)) {
    v = json::parse(res->body);
  }

  this->beginTransaction();
  for (auto ts_units : *ids.get()) {
    this->insertIdentifierAndUnits(ts_units.first, ts_units.second.first);
  }
  this->endTransaction();
  
  _errCallback("OK");
  return;
}

size_t InfluxTcpAdapter::maxTransactionLines() {
  return 5000;
}

void InfluxTcpAdapter::sendPointsWithString(const std::string& content) {

  if(sendPointsFuture.valid()){
    sendPointsFuture.wait();
    sendPointsFuture.get();
  }

  sendPointsFuture = std::async(std::launch::async, [&, content]{
    const string bodyContent(content);

    namespace bio = boost::iostreams;
    std::stringstream compressed;
    std::stringstream origin(bodyContent);
    bio::filtering_streambuf<bio::input> out;
    out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::default_compression)));
    out.push(origin);
    bio::copy(out, compressed);
    const string zippedContent(compressed.str());
    
    try {
      string path = "/write?db=" + this->conn.db + "&precision=s";
      httplib::Headers headers = {
        {"Content-Encoding", "gzip"}
      };
      auto res = _httpClient->Post(path, headers, zippedContent, "application/octet-stream");
      if (res) {
        int code = res->status;
        switch(code) {
          case 204:
          case 200:
            break;
          default:
            cout << "INFLUX TCP ADAPTER: Send points to influx: POST returned " << code << " - " << res->body << EOL << flush;
        }
      } else {
        cerr << TAG << ": HTTP request failed (no response)" << endl;
      }
    } catch (std::exception& e) {
      cerr << TAG << ": sending points: " << e.what() << endl;
    }

  });
  
  if(!_inTransaction){
    sendPointsFuture.wait();
    sendPointsFuture.get();
  }

}


json InfluxTcpAdapter::jsonFromResponseBody(int statusCode, const std::string& body) {
  this->lastError = "";
  json js = json::object();
  
  if (statusCode == 200) {
    if (!json::accept(body)){
      cerr << TAG << ": JSON Parse Error: " << body << endl;
      this->lastError = body;
      return js;
    } else {
      js = json::parse(body);
      return js;
    }
  }
  else {
    cerr << TAG << ": Connection Error: HTTP " << statusCode << " - " << body << endl;
    if (json::accept(body)) {
      auto errjs = json::parse(body);
      if (errjs.contains("error")) {
        this->lastError = errjs["error"].get<string>();
      }
    } else {
      this->lastError = body;
    }
    return js;
  }
}


vector<TSF::Point> InfluxTcpAdapter::__pointsSingle(json& json) {
  auto multi = __pointsFromJson(json);
  if (multi.size() > 0) {
    return multi.begin()->second;
  }
  else {
    return vector<TSF::Point>();
  }
}


map<string, vector<TSF::Point> > InfluxTcpAdapter::__pointsFromJson(json& json) {
  
  map<string, vector<TSF::Point> > out;
  
  // check for correct response format:
  if (!json.is_object() || 
      json.size() == 0 ||
      !json.contains(kRESULTS) ||
      !json[kRESULTS].is_array() ||
      json[kRESULTS].size() == 0)
  {
    return out;
  }
  
  
  for (auto &statement : json[kRESULTS]) {
    
    if ( !statement.is_object() || !statement.contains(kSERIES) ) {
      if (statement.contains("error")) {
        cerr << TAG << ": Error in json decoding: " << statement["error"].dump() << endl;
      }
      continue;
    }
    auto &seriesArray = statement[kSERIES];
    for (const auto &series : seriesArray) {
      // assemble the proper identifier for this series
      MetricInfo metric("");
      if (series.count("name") == 0) {
        cerr << TAG << ": Influx returned malformed response. No \"name\" property in series." << endl;
        cerr << TAG << ": Series format returned: " << series.dump() << endl;
      }
      metric.measurement = series.at("name").get<string>();
      if (series.contains("tags")) {
        auto tagsObj = series.at("tags");
        for (auto it = tagsObj.begin(); it != tagsObj.end(); ++it) {
          auto key = it.key();
          string value = it.value();
          if (value != "") {
            metric.tags[key] = value;
          }
        }
        Units units = TSF_DIMENSIONLESS;
        if (metric.tags.count("units") == 0) {
          cerr << TAG << ": Influx returned malformed response. No \"units\" property in tag list." << endl;
          cerr << TAG << ": Tags object format returned: " << tagsObj.dump() << endl;
        }
        else {
          units = Units::unitOfType(metric.tags.at("units"));
        }
        metric.tags.erase("units"); // get rid of units if they are included.
      }
      string properId = metric.name();
      
      map<string,int> columnMap;
      if (series.count("columns") == 0) {
        cerr << TAG << ": Influx returned malformed response. No \"columns\" property in series." << endl;
        cerr << TAG << ": Series format returned: " << series.dump() << endl;
      }
      auto cols = series.at("columns");
      for (int i = 0; i < cols.size(); ++i) {
        string colName = cols[i];
        columnMap[colName] = (int)i;
      }
      
      // check that required columns are all there
      bool allColumnsPresent = true;
      for (const string &key : {"time","value"}) {
        if (columnMap.count(key) == 0) {
          cerr << "column map does not contain key: " << key << endl;
          allColumnsPresent = false;
        }
      }
      if (!allColumnsPresent) {
        continue; // skip this parsing iteration
      }
      
      const int
      timeIndex = columnMap["time"],
      valueIndex = columnMap["value"],
      qualityIndex = columnMap.count("quality") > 0 ? columnMap["quality"] : -1,
      confidenceIndex = columnMap.count("confidence") > 0 ? columnMap["confidence"] : -1;
      
      if (series.count("values") == 0) {
        cerr << TAG << ": Influx returned malformed response. No \"values\" property in series." << endl;
        cerr << TAG << ": Series format returned: " << series.dump() << endl;
      }
      const auto &values = series.at("values");
      
      auto nValues = values.size();
      if (nValues == 0) {
        continue;
      }
      
      if (out.count(properId) == 0) {
        out[properId] = vector<TSF::Point>();
      }
      
      auto pointVec = &(out.at(properId));
      
      if (nValues > 1) {
        pointVec->reserve(pointVec->size() + nValues + 2);
      }
      
      
      for (const auto &rowV : values) {
        const auto &row = rowV;
        // ensure that these values are non-null and castable
        if (row.at(timeIndex).is_null() || row.at(valueIndex).is_null() || !row.at(timeIndex).is_number() || !row.at(valueIndex).is_number()) {
          cerr << TAG << ": Influx returned malformed row: " << row.dump() << endl;
          continue;
        }
        time_t t = row.at(timeIndex);
        double v = row.at(valueIndex);
        TSF::Point::PointQuality q = TSF::Point::opc_tsf_override;
        if (qualityIndex > 0 && !row.at(qualityIndex).is_null()) {
          q = (TSF::Point::PointQuality)(row.at(qualityIndex));
        }
        double c = 0;
        if (confidenceIndex > 0 && !row.at(confidenceIndex).is_null()) {
          c = row.at(confidenceIndex);
        }
        pointVec->push_back(TSF::Point(t,v,q,c));
      }
    }
  }
  
  // sort these points
  for (auto &ts : out) {
    std::sort(ts.second.begin(), ts.second.end(), TSF::Point::comparePointTime);
  }
  
  
  return out;
}




InfluxTcpAdapter::Query InfluxTcpAdapter::queryPartsFromMetricId(const std::string &name) {
  MetricInfo m(name);
  
  Query q;
  q.select = {"time", "value", "quality", "confidence"};
  q.from = "\"" + m.measurement + "\"";
  
  if (m.tags.size() > 0) {
    for( auto p : m.tags) {
      stringstream ss;
      ss << "\"" << p.first << "\"='" << p.second << "'";
      string s = ss.str();
      q.where.push_back(s);
    }
  }
  
  return q;
}

std::string InfluxTcpAdapter::formatTimestamp(time_t t) {
  return to_string(t);
}


/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/


InfluxUdpAdapter::InfluxUdpAdapter( errCallback_t cb ) : InfluxAdapter(cb) {
  _sendFuture = std::async(launch::async, [&](){return;});
}

InfluxUdpAdapter::~InfluxUdpAdapter() {
  
}

const DbAdapter::adapterOptions InfluxUdpAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = true;
  o.supportsSinglyBoundQuery = true;
  o.searchIteratively = false;
  o.canAssignUnits = false;
  o.implementationReadonly = false;
  o.canDoWideQuery = false;
  
  return o;
}

std::string InfluxUdpAdapter::connectionString() {
  stringstream ss;
  ss << "host=" << this->conn.host << "&port=" << this->conn.port << "&ratelimit=" << this->conn.msec_ratelimit;
  return ss.str();
}

void InfluxUdpAdapter::doConnect() {
  _connected = false;
  boost::asio::io_service io_service;
  try {
    using boost::asio::ip::udp;
    boost::asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), this->conn.host, to_string(this->conn.port));
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket socket(io_service);
    socket.open(udp::v4());
    socket.close();
  } catch (const std::exception &err) {
    DebugLog << "could not connect to UDP endpoint" << EOL << flush;
    _errCallback("Invalid UDP Endpoint");
    return;
  }
  _errCallback("Connected");
  _connected = true;
}

IdentifierUnitsList InfluxUdpAdapter::idUnitsList() {
  return IdentifierUnitsList();
}

// READ
std::vector<TSF::Point> InfluxUdpAdapter::selectRange(const std::string& id, TimeRange range) {
  return {TSF::Point()};
}

TSF::Point InfluxUdpAdapter::selectNext(const std::string& id, time_t time, WhereClause q) {
  return TSF::Point();
}

TSF::Point InfluxUdpAdapter::selectPrevious(const std::string& id, time_t time, WhereClause q) {
  return TSF::Point();
}


// DELETE
void InfluxUdpAdapter::removeRecord(const std::string& id) {
  return;
}

void InfluxUdpAdapter::removeAllRecords() {
  return;
}

size_t InfluxUdpAdapter::maxTransactionLines() {
  return 10;
}

void InfluxUdpAdapter::sendPointsWithString(const std::string& content) {
  if (_sendFuture.valid()) {
    _sendFuture.wait();
  }
  string body(content);
  _sendFuture = std::async(launch::async, [&,body]() {
    using boost::asio::ip::udp;
    boost::asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), this->conn.host, to_string(this->conn.port));
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket socket(io_service);
    socket.open(udp::v4());
    boost::system::error_code err;
    socket.send_to(boost::asio::buffer(body, body.size()), receiver_endpoint, 0, err);
    if (err) {
      DebugLog << "UDP SEND ERROR: " << err.message() << EOL << flush;
    }
    socket.close();
    if (conn.msec_ratelimit > 0) {
      this_thread::sleep_for(chrono::milliseconds(conn.msec_ratelimit));
    }
  });

}

std::string InfluxUdpAdapter::formatTimestamp(time_t t) {
  // Line protocol requires unix-nano unles qualified by HTTP-GET fields,
  // which of course we don't have over UDP
  return to_string(t) + "000000000";
}
