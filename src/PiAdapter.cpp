#include "PiAdapter.h"

#include <iostream>
#include <regex>
#include <boost/lexical_cast.hpp>

#include "PointRecordTime.h"

using namespace std;
using namespace TSF;

using namespace web::http::client;
using web::http::method;
using web::http::methods;
using web::http::http_request;
using web::http::status_codes;
using web::http::http_response;
using web::http::uri;

using jsv = web::json::value;


#ifndef PI_SSL_VALIDATE
  #define PI_SSL_VALIDATE true
#endif

#ifndef PI_MAX_POINT_COUNT
  // max points should accomodate data at 1-s resolution for 24-h
  #define PI_MAX_POINT_COUNT 90000
#endif

#define PI_TIMEOUT 3

const string kOSI_REST("OSIsoft.REST");
const string kFULL_VERSION("FullVersion");
const string kWebId("WebId");
const string kItems("Items");
const string kName("Name");
const string kDescriptor("Descriptor");
const string kEngUnits("EngineeringUnits");
const string kTimeStamp("Timestamp");
const string kValue("Value");
const string kGood("Good");
const string kQuest("Questionable");
const string kSubst("Substituted");


const char* t_fmt = "%Y-%m-%dT%H:%M:%SZ";


Point PiAdapter::_pointFromJson(const jsv& j) {
  for (const string& key : {kTimeStamp,kValue,kGood,kQuest,kSubst}) {
    if (!j.has_field(key)) {
      cerr << "ERR: JSON object does not contain required field " << key << endl;
      return Point();
    }
  }
  
  double v;
  
  // check value type?
  auto ov = j.at(kValue);
  auto ovType = ov.type();
  
  if (ovType == jsv::value_type::Number) {
    v = ov.as_double();
  }
  else if (ovType == jsv::value_type::String) {
    string key = ov.as_string();
    if (_conversions.count(key) > 0) {
      v = _conversions.at(key);
    }
    else {
      cerr << "No valid conversion for: " << ov.serialize() << endl;
      return Point();
    }
  }
  else if (ovType == jsv::Object) {
    // try getting Value.Value
    if (ov.has_field(kValue) && ov.at(kValue).is_number()) {
      // nested value, as in an object
      auto nested = ov.at(kValue);
      v = nested.as_double();
    }
    else {
      cerr << "Nested value could not be retrieved: " << ov.serialize() << endl;
      return Point();
    }
  }
  else {
    cerr << "Value field is not double, or no conversion exists" << endl;
    return Point();
  }
  
  const string tstamp(j.at(kTimeStamp).as_string());
  const bool good(j.at(kGood).as_bool());
  Point::PointQuality q = (good ? Point::PointQuality::opc_good : Point::PointQuality::opc_bad);
  time_t t = PointRecordTime::timeFromIso8601(tstamp);
  
  return Point(t,v,q);
}





PiAdapter::PiAdapter(errCallback_t cb) : DbAdapter(cb) { 
  _conn.proto = "https";
  _conn.host = "devdata.osisoft.com";
  _conn.port = 443;
  _conn.apiPath = "piwebapi";
  _conn.dataServer = "PISRV1";
  _conn.user = "user";
  _conn.pass = "pass";
  
  tagSearchPath = "";
  valueConversions = "Active=1&Inactive=0&On=1&Off=0";
}

PiAdapter::~PiAdapter() { 
  
}

const DbAdapter::adapterOptions PiAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = false;
  o.canAssignUnits = false;
  o.searchIteratively = true;
  o.supportsSinglyBoundQuery = false;
  o.implementationReadonly = true;
  o.canDoWideQuery = false;
  
  return o;
}

std::string PiAdapter::connectionString() {
  stringstream ss;
  ss << "proto=" << this->_conn.proto << "&host=" << this->_conn.host << "&port=" << this->_conn.port << "&api=" << this->_conn.apiPath << "&dataserver=" << this->_conn.dataServer << "&u=" << this->_conn.user << "&p=" << this->_conn.pass;
  return ss.str();
}

void PiAdapter::setConnectionString(const std::string& str) {
  _TSF_DB_SCOPED_LOCK;
  
  // split the tokenized string. we're expecting something like "host=127.0.0.1&port=4242"
  
  
  const map<string, function<void(string)> > 
  kvSetters({
    {"proto", [&](string v){this->_conn.proto = v;}},
    {"host", [&](string v){this->_conn.host = v;}},
    {"port", [&](string v){this->_conn.port = boost::lexical_cast<int>(v);}},
    {"api", [&](string v){this->_conn.apiPath = v;}},
    {"dataserver", [&](string v){this->_conn.dataServer = v;}},
    {"u", [&](string v){this->_conn.user = v;}},
    {"p", [&](string v){this->_conn.pass = v;}}
  }); 
  
  auto kvPairs = _kvFromDelimited(str);
  
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

void PiAdapter::doConnect() {
  _TSF_DB_SCOPED_LOCK;
  _connected = false;
  _conn.dsWebId = "";
  
  _conversions.clear();
  try {
    auto kvPairs = _kvFromDelimited(valueConversions);
    for (auto kv : kvPairs) {
      _conversions[kv.first] = boost::lexical_cast<double>(kv.second);
    }
  } catch (const exception& e) {
    //...
  }
  
  // try the endpoint
  auto uriQ = uriBase()
  .append_path("system")
  .append_path("versions")
  .to_uri();
  
  jsv j = jsonFromRequest(uriQ, methods::GET);
  if (j.has_field(kOSI_REST)) {
    auto v = j[kOSI_REST];
    if (v.has_field(kFULL_VERSION)) {
      string vers = v[kFULL_VERSION].as_string();
      cout << "PI REST VERSION: " << vers << endl;
    }
  }
  else {
    cerr << "could not retrieve version info from PI endpoint" << endl;
    _connected = false;
    _errCallback("Malformed Response");
    return;
  }
  
  
  // ok, now try to get the webId for the data server
  auto uriDS = uriBase()
  .append_path("dataservers")
  .append_query("name", _conn.dataServer)
  .append_query("selectedFields", "WebId;Name;IsConnected;ServerVersion")
  .to_uri();
  
  jsv dj = jsonFromRequest(uriDS, methods::GET);
  if (dj.has_field(kWebId)) {
    string webId = dj[kWebId].as_string();
    cout << "PI DATA SERVER WEB ID: " << webId << endl;
    _conn.dsWebId = webId;
    _connected = true;
    _errCallback("Connected");
  }
  else {
    cerr << "could not retrieve dataserver WebId from PI endpoint" << endl;
    _connected = false;
    _errCallback("No Data Server");
    return;
  }
  
}

IdentifierUnitsList PiAdapter::idUnitsList() {
  _TSF_DB_SCOPED_LOCK;
  
  IdentifierUnitsList ids;
  _webIdLookup.clear();
  
  auto uriPointsB = uriBase()
  .append_path("dataservers")
  .append_path(_conn.dsWebId)
  .append_path("points")
  .append_query("maxCount",10000)
  .append_query("selectedFields=Items.WebId;Items.Name;Items.Descriptor;Items.EngineeringUnits");
  
  if (this->tagSearchPath != "") {
    uriPointsB.append_query("nameFilter",this->tagSearchPath);
  }
  
  jsv j = jsonFromRequest(uriPointsB.to_uri(), methods::GET);
  if (j.has_field(kItems)) {
    for(auto i : j[kItems].as_array()) {
      if (i.has_field(kName) && i.has_field(kWebId)) {
        string name = i[kName].as_string();
        string webId = i[kWebId].as_string();
        _webIdLookup[name] = webId;
        ids.set(name, TSF_DIMENSIONLESS);
      }
    }
  }
  
  return ids;
}

void PiAdapter::beginTransaction() {
  // nope
}
void PiAdapter::endTransaction() {
  // nope
}

// READ
std::vector<Point> PiAdapter::selectRange(const std::string& id, TimeRange range) {
  _TSF_DB_SCOPED_LOCK;
  vector<Point> points;
  
  if (_webIdLookup.count(id) == 0) {
    cerr << "PI RECORD ERROR: id " << id << " not in cache" << endl;
    return points;
  }
  
  auto startStr = PointRecordTime::utcDateStringFromUnix(range.start,t_fmt);
  auto endStr = PointRecordTime::utcDateStringFromUnix(range.end,t_fmt);
  string webId = _webIdLookup[id];
  
  auto uriRange = uriBase()
  .append_path("streams")
  .append_path(webId)
  .append_path("recorded")
  .append_query("startTime",startStr)
  .append_query("endTime",endStr)
  .append_query("maxCount",PI_MAX_POINT_COUNT)
  .to_uri();
  
  jsv j = jsonFromRequest(uriRange, methods::GET);
  
  if (!j.has_field(kItems)) {
    cerr << "PI RECORD COULD NOT FIND ITEMS in response: " << j.serialize() << endl;
    return points;
  }
  
  for (auto pjs : j[kItems].as_array()) {
    Point p = _pointFromJson(pjs);
    if (p.isValid) {
      points.push_back(p);
    }
  }
  
  return points;
}
Point PiAdapter::selectNext(const std::string& id, time_t time, WhereClause q) {
  return Point();
}

Point PiAdapter::selectPrevious(const std::string& id, time_t time, WhereClause q) {
  return Point();
}

// CREATE
bool PiAdapter::insertIdentifierAndUnits(const std::string& id, Units units) {
  return false;
}

void PiAdapter::insertSingle(const std::string& id, Point point) {
  // nope.
}

void PiAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  // nope.
}

// UPDATE
bool PiAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  return false;
}

// DELETE
void PiAdapter::removeRecord(const std::string& id) {
  // nope.
}

void PiAdapter::removeAllRecords() {
  // nope.
}


web::json::value PiAdapter::jsonFromRequest(web::http::uri uri, web::http::method withMethod) {
  jsv js = jsv::object();
  try {
    string auth = _conn.user + ":" + _conn.pass;
    std::vector<unsigned char> bytes(auth.begin(), auth.end());
    string userpass = utility::conversions::to_base64(bytes);
    
    http_client_config config;
    config.set_validate_certificates(PI_SSL_VALIDATE);
    config.set_timeout(std::chrono::seconds(PI_TIMEOUT));
    //    credentials cred(_conn.user,_conn.pass);
    //    config.set_credentials(cred);
    http_client client(uri, config);
    http_request req(methods::GET);
    req.headers().add("Authorization", "Basic " + userpass);
    http_response r = client.request(req).get(); // waits for response
    if (r.status_code() == status_codes::OK) {
      js = r.extract_json().get();
    }
    else {
      cerr << "CONNECTION ERROR: " << r.reason_phrase() << EOL;
    }
  }
  catch (std::exception &e) {
    cerr << "exception in GET: " << e.what() << endl;
  }
  return js;
}

web::http::uri_builder PiAdapter::uriBase() {
  web::http::uri_builder b;
  b.set_scheme(_conn.proto).set_host(_conn.host).set_port(_conn.port)
  .append_path(_conn.apiPath);
  return b;
}


map<string,string> PiAdapter::_kvFromDelimited(const string& str) {
  std::map<std::string, std::string> kvPairs;
  regex kvReg("([^=]+)=([^&]+)&?"); // key - value pair
  auto kv_begin = sregex_iterator(str.begin(), str.end(), kvReg);
  auto kv_end = sregex_iterator();
  for (auto it = kv_begin ; it != kv_end; ++it) {
    kvPairs[(*it)[1]] = (*it)[2];
  }
  
  return kvPairs;
}





