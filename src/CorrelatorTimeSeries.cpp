//
//  CorrelatorTimeSeries.cpp
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//


#include "CorrelatorTimeSeries.h"
#include "AggregatorTimeSeries.h"

#include <boost/foreach.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/covariance.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

using namespace boost::accumulators;
using namespace RTX;
using namespace std;


CorrelatorTimeSeries::CorrelatorTimeSeries() {
  Clock::_sp c( new Clock(3600) );
  _corWindow = c;
}



TimeSeries::_sp CorrelatorTimeSeries::correlatorTimeSeries() {
  return _secondary;
}

void CorrelatorTimeSeries::setCorrelatorTimeSeries(TimeSeries::_sp ts) {
  
  if (this->source() && ts && !this->source()->units().isSameDimensionAs(ts->units())) {
    return; // can't do it.
  }
  
  _secondary = ts;
  this->invalidate();
}

Clock::_sp CorrelatorTimeSeries::correlationWindow() {
  return _corWindow;
}

void CorrelatorTimeSeries::setCorrelationWindow(Clock::_sp window) {
  _corWindow = window;
  this->invalidate();
}


#pragma mark - superclass overrides

TimeSeries::PointCollection CorrelatorTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection data(vector<Point>(), this->units());
  if (!this->correlatorTimeSeries() || !this->source()) {
    return data;
  }
  
  // force pre-cache
  TimeRange preFetchRange(range.start - this->correlationWindow()->period(), range.end);
  this->source()->points(preFetchRange);
  this->correlatorTimeSeries()->points(preFetchRange);
  
  TimeSeries::_sp sourceTs = this->source();
  time_t windowWidth = this->correlationWindow()->period();
  
  set<time_t> times = this->timeValuesInRange(range);
  vector<Point> thePoints;
  thePoints.reserve(times.size());
  
  BOOST_FOREACH(time_t t, times) {
    double corrcoef = 0;
    TimeRange q(t-windowWidth, t);
    PointCollection sourceCollection = sourceTs->pointCollection(q);
    
    set<time_t> sourceTimeValues;
    BOOST_FOREACH(const Point& p, sourceCollection.points) {
      sourceTimeValues.insert(p.time);
    }
    
    // expand the query range for the secondary collection
    TimeRange primaryRange(*(sourceTimeValues.begin()), *(sourceTimeValues.rbegin()));
    TimeRange secondaryRange;
    secondaryRange.start = this->correlatorTimeSeries()->timeBefore(primaryRange.start + 1);
    secondaryRange.end = this->correlatorTimeSeries()->timeAfter(primaryRange.end - 1);
    
    PointCollection secondaryCollection = this->correlatorTimeSeries()->pointCollection(secondaryRange);
    secondaryCollection.resample(sourceTimeValues);
    
    if (sourceCollection.count() == 0 || secondaryCollection.count() == 0) {
      continue; // no points to correlate
    }
    
    if (sourceCollection.count() != secondaryCollection.count()) {
      cout << "Unequal number of points" << endl;
      return PointCollection(vector<Point>(), this->units());
    }
    
    // get consistent units
    secondaryCollection.convertToUnits(sourceCollection.units);
    
    // correlation coefficient
    accumulator_set<double, stats<tag::mean, tag::variance> > acc1;
    accumulator_set<double, stats<tag::mean, tag::variance> > acc2;
    accumulator_set<double, stats<tag::covariance<double, tag::covariate1> > > acc3;
    for (int i = 0; i < sourceCollection.count(); i++) {
      Point p1 = sourceCollection.points.at(i);
      Point p2 = secondaryCollection.points.at(i);
      acc1(p1.value);
      acc2(p2.value);
      acc3(p1.value, covariate1 = p2.value);
    }
    corrcoef = covariance(acc3)/sqrt(variance(acc1))/sqrt(variance(acc2));
    
    thePoints.push_back(Point(t,corrcoef));
    
  }
  
  return PointCollection(thePoints, this->units());
}

bool CorrelatorTimeSeries::canSetSource(TimeSeries::_sp ts) {
  if (this->correlatorTimeSeries() && !ts->units().isSameDimensionAs(this->correlatorTimeSeries()->units())) {
    return false;
  }
  return true;
}

void CorrelatorTimeSeries::didSetSource(TimeSeries::_sp ts) {
  this->invalidate();
}

bool CorrelatorTimeSeries::canChangeToUnits(Units units) {
  if (units.isDimensionless()) {
    return true;
  }
  return false;
}


// static methods
TimeSeries::_sp correlationArray(TimeSeries::_sp primary, TimeSeries::_sp secondary, Clock::_sp window, int nLags) {
  
  if (!primary || !primary->clock() || !secondary) {
    TimeSeries::_sp blank;
    return blank;
  }
  
  AggregatorTimeSeries::_sp maxCor(new AggregatorTimeSeries);
  
  maxCor->setName(primary->name() + "." + secondary->name() + ".maxCorrelation" );
  
  int period = primary->clock()->period();
  
  
  
  
  
  
  
  
}

