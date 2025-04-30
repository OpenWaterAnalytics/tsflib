#include "TimeSeriesQuery.h"

#include <regex>
#include <string>
#include <cctype>
#include <cstdint>

using namespace TSF;
using namespace std;

// -----------------------------------------------------------------------------
// Utility: convert a magnitude+unit suffix (s, m, h, d, w) to seconds
// -----------------------------------------------------------------------------
static std::int64_t duration_to_seconds(std::int64_t magnitude, char unit)
{
  switch (std::tolower(unit))
  {
    case 's': return magnitude;                 // seconds
    case 'm': return magnitude * 60;            // minutes
    case 'h': return magnitude * 3600;          // hours
    case 'd': return magnitude * 86400;         // days  (24 h)
    case 'w': return magnitude * 604800;        // weeks (7 d)
    default:  return 0;                         // unknown → no expansion
  }
}

static TimeRange adjust_range(TimeRange range, const string& query) {
  TimeRange newRange = range;
  static const std::regex timeGroupRe(R"(group\s+by\s+time\s*\(\s*(\d+)\s*([smhdw])\s*\))", std::regex_constants::icase);
  
  std::smatch m;
  if (std::regex_search(query, m, timeGroupRe))
  {
    const std::int64_t magnitude = std::stoll(m[1].str()); // e.g. 30
    const char unit              = m[2].str()[0];          // e.g. 'm'
    const std::int64_t interval  = duration_to_seconds(magnitude, unit);
    
    if (interval > 0)
    {
      newRange.start = newRange.start > 0 ? newRange.start - interval : 0;
      newRange.end = newRange.end > 0 ? newRange.end + interval : 0;
    }
  }
  return newRange;
}

TimeSeriesQuery::TimeSeriesQuery() {
  _query = "select mean(value) as value from series_name where \"tag\"='value' and $timeFilter group by time(5m)";
}

string TimeSeriesQuery::query() {
  return _query;
}
void TimeSeriesQuery::setQuery(const std::string &query) {
  _query = query;
}


time_t TimeSeriesQuery::timeBefore(time_t t) {
  return this->pointBefore(t).time;
}

Point TimeSeriesQuery::pointBefore(time_t time) {
  vector<Point> points;
  if (_qRecord) {
    points = _qRecord->pointsWithQuery(this->query(), TimeRange(0,time));
  }
  return points.size() > 0 ? points.front() : Point();
}


time_t TimeSeriesQuery::timeAfter(time_t t) {
  return this->pointAfter(t).time;
}

Point TimeSeriesQuery::pointAfter(time_t time) {
  vector<Point> points;
  if (_qRecord) {
    points = _qRecord->pointsWithQuery(this->query(), TimeRange(time,0));
  }
  return points.size() > 0 ? points.front() : Point();
}

vector< Point > TimeSeriesQuery::points(TimeRange range) {
  vector<Point> points;
  
  std::string q = this->query();
  TimeRange newRange = adjust_range(range, q);
  
  if (_qRecord) {
    points = _qRecord->pointsWithQuery(this->query(), newRange);
    points = PointCollection(points, this->units()).trimmedToRange(range).points();
  }
  return points;
}


PointRecord::_sp TimeSeriesQuery::record() {
  return _qRecord;
}

void TimeSeriesQuery::setRecord(PointRecord::_sp record) {
  auto db = dynamic_pointer_cast<DbPointRecord>(record);
  if (db) {
    _qRecord = db;
  }
}

