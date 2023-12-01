#include "ValidRangeTimeSeries.h"
#include "WhereClause.h"

using namespace std;
using namespace TSF;

ValidRangeTimeSeries::ValidRangeTimeSeries() {
  _mode = drop;
  _range = make_pair(0, 1);
}

pair<double,double> ValidRangeTimeSeries::range() {
  return _range;
}
void ValidRangeTimeSeries::setRange(double min, double max) {
  _range = make_pair(min, max);
  this->invalidate();
}

ValidRangeTimeSeries::filterMode_t ValidRangeTimeSeries::mode() {
  return _mode;
}
void ValidRangeTimeSeries::setMode(filterMode_t mode) {
  _mode = mode;
  this->invalidate();
}

bool ValidRangeTimeSeries::willResample() {
  return TimeSeriesFilter::willResample() || (this->clock() && _mode == filterMode_t::drop);
}

bool ValidRangeTimeSeries::canDropPoints() {
  return this->mode() == ValidRangeTimeSeries::drop;
}


Point ValidRangeTimeSeries::pointBefore(time_t time) {
  if (!source() || !source()->supportsQualifiedQuery() || !this->canDropPoints()) {
    // default to the simple iterative search provided by the base class
    return TimeSeriesFilter::pointBefore(time);
  }
  else {
    // ok, so we might drop points, and we know that our source can support a qualified lookup.
    // this means that we can delegate the valid-ranging on to the source database.
    WhereClause q;
    q.clauses[WhereClause::gte] = Units::convertValue(_range.first, this->units(), this->source()->units());
    q.clauses[WhereClause::lte] = Units::convertValue(_range.second, this->units(), this->source()->units());
    
    Point p = source()->pointBefore(time, q);
    return p.converted(source()->units(), this->units());
  }
}

Point ValidRangeTimeSeries::pointAfter(time_t time) {
  if (!source() || !source()->supportsQualifiedQuery() || !this->canDropPoints()) {
    // default to the simple iterative search provided by the base class
    return TimeSeriesFilter::pointAfter(time);
  }
  else {
    // ok, so we might drop points, and we know that our source can support a qualified lookup.
    // this means that we can delegate the valid-ranging on to the source database.
    
    WhereClause q;
    q.clauses[WhereClause::gte] = Units::convertValue(_range.first, this->units(), this->source()->units());
    q.clauses[WhereClause::lte] = Units::convertValue(_range.second, this->units(), this->source()->units());
    
    Point p = source()->pointAfter(time, q);
    return p.converted(source()->units(), this->units());
  }
}


PointCollection ValidRangeTimeSeries::filterPointsInRange(TimeRange range) {
  
  // if we are to resample, then there's a possibility that we need to expand the range
  // used to query the source ts. but we have to limit the search to something reasonable, in case
  // too many points are excluded. yikes!
  TimeRange sourceQuery = range;
  if (this->willResample()) {
    sourceQuery = this->expandedRange(range);
  }
  
  // get raw values, exclude outliers, then resample if needed.
  PointCollection raw = this->source()->pointCollection(sourceQuery);
  
  raw.convertToUnits(this->units());
  
  vector<Point> outP;
  
  raw.apply([&](Point& p){
    Point newP;
    double pointValue = p.value;
    
    if (pointValue < _range.first || _range.second < pointValue) {
      // out of range.
      if (_mode == saturate) {
        // bring pointValue to the nearest max/min value
        pointValue = TSF_MAX(pointValue, _range.first);
        pointValue = TSF_MIN(pointValue, _range.second);
        newP = Point(p.time, pointValue, p.quality, p.confidence);
      }
      else {
        // drop point
        newP.time = p.time;
      }
    }
    else {
      // just pass it through
      newP = p;
    }
    if (newP.isValid) {
      outP.push_back(newP);
    }
  });
  
  PointCollection outData(outP,this->units());
  
  if (this->willResample()) {
    set<time_t> resTimes = this->timeValuesInRange(range);
    outData.resample(resTimes);
  }
  
  return outData;
}


