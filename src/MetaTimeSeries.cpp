#include "MetaTimeSeries.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace TSF;

MetaTimeSeries::MetaTimeSeries() {
  this->setUnits(TSF_SECOND); // Default time unit
  _metaMode = MetaModeGap;
}


PointCollection MetaTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection gaps(vector<Point>(), TSF_DIMENSIONLESS);
  
  if (_metaMode == MetaModeGap) {
    gaps.units = TSF_SECOND;
  }
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->timeBefore(range.start + 1);
    qRange.end = this->source()->timeAfter(range.end - 1);
  }
  
  // one prior
  qRange.start = this->source()->timeBefore(qRange.start);
  
  qRange.correctWithRange(range);
  PointCollection sourceData = this->source()->pointCollection(qRange);
  
  if (sourceData.count() < 2) {
    return PointCollection(vector<Point>(), this->units());
  }
  
  auto raw = sourceData.raw();
  vector<Point>::const_iterator it = raw.first;
  vector<Point>::const_iterator prev = it;
  if (_metaMode == MetaModeGap) {
    ++it;
  }
  vector<Point> theGaps;
  while (it != raw.second) {
    time_t t1,t2;
    t1 = prev->time;
    t2 = it->time;
    Point metaPoint(t2);
    
    switch (_metaMode) {
      case MetaModeGap:
        metaPoint.value = (double)(t2-t1);
        break;
      case MetaModeConfidence:
        metaPoint.value = it->confidence;
        break;
      case MetaModeQuality:
        metaPoint.value = (double)(it->quality);
        break;
      default:
        break;
    }
    
    metaPoint.addQualFlag(Point::tsf_integrated);
    theGaps.push_back(metaPoint);
    
    ++prev;
    ++it;
  }
  gaps.setPoints(theGaps);
  gaps.convertToUnits(this->units());
  
  if (this->willResample()) {
    set<time_t> times = this->timeValuesInRange(range);
    gaps.resample(times);
  }
  
  return gaps;
}

bool MetaTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return true;
}

void MetaTimeSeries::didSetSource(TimeSeries::_sp ts) {
  this->invalidate();
}

bool MetaTimeSeries::canChangeToUnits(Units units) {
  if (_metaMode == MetaModeGap) {
    return (units.isSameDimensionAs(TSF_SECOND));
  }
  else if (_metaMode == MetaModeConfidence) {
    return true;
  }
  else if (units.isDimensionless()) {
    return true;
  }
  return false;
}

void MetaTimeSeries::setMetaMode(TSF::MetaTimeSeries::MetaMode mode) {
  if (mode == _metaMode) {
    return;
  }
  
  _metaMode = mode;
  this->invalidate();
  
  if (_metaMode == MetaModeGap) {
    this->setUnits(TSF_SECOND);
  }
}

MetaTimeSeries::MetaMode MetaTimeSeries::metaMode() {
  return _metaMode;
}
