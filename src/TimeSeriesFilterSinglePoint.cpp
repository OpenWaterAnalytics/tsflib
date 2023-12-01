//
//  TimeSeriesFilterSinglePoint.cpp
//  tsflib
//
//  Created by Sam Hatchett on 1/16/15.
//
//

#include "TimeSeriesFilterSinglePoint.h"
#include <boost/foreach.hpp>

using namespace TSF;
using namespace std;



PointCollection TimeSeriesFilterSinglePoint::filterPointsInRange(TimeRange range) {
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->timeBefore(range.start + 1);
    qRange.end = this->source()->timeAfter(range.end - 1);
  }
  
  qRange.correctWithRange(range);
//  PointCollection sourceData = this->source()->pointCollection(qRange);
  
  PointCollection data;
  if (this->source()) {
    data = source()->pointCollection(qRange);
  }
  
  vector<Point> outPoints;
  bool didDropPoints = false;
  data.apply([&](Point& p){
    Point converted = this->filteredWithSourcePoint(p);
    if (converted.isValid) {
      outPoints.push_back(converted);
    }
    else {
      didDropPoints = true;
    }
  });
  
  
  PointCollection outData(outPoints, this->units());
  if (this->willResample() || (didDropPoints && this->clock())) {
    set<time_t> timeValues = this->timeValuesInRange(range); // if infinite recursion occurs here, check canDropPoints
    outData.resample(timeValues);
  }
  
  return outData;
}

