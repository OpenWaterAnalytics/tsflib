//
//  BaseStatsTimeSeries.cpp
//  tsflib
//
//  Created by Sam Hatchett on 7/17/14.
//
//

#include "BaseStatsTimeSeries.h"

using namespace TSF;
using namespace std;
using pvIt = PointCollection::pvIt;
using pvRange = PointCollection::pvRange;



BaseStatsTimeSeries::BaseStatsTimeSeries() {
//  Clock::_sp window(new Clock(60));
//  _window = window;
  _summaryOnly = true;
  _samplingMode = StatsSamplingModeLagging;
}


void BaseStatsTimeSeries::setWindow(Clock::_sp window) {
  if (window) {
    _window = window;
  }
  this->invalidate();
}
Clock::_sp BaseStatsTimeSeries::window() {
  return _window;
}


bool BaseStatsTimeSeries::summaryOnly() {
  return _summaryOnly;
}

void BaseStatsTimeSeries::setSummaryOnly(bool summaryOnly) {
  _summaryOnly = summaryOnly;
}


void BaseStatsTimeSeries::setSamplingMode(StatsSamplingMode_t mode) {
  _samplingMode = mode;
  this->invalidate();
}

BaseStatsTimeSeries::StatsSamplingMode_t BaseStatsTimeSeries::samplingMode() {
  return _samplingMode;
}

BaseStatsTimeSeries::rangeGroup BaseStatsTimeSeries::subRanges(set<time_t> times) {
  rangeGroup group;
    
  if (times.size() == 0 || !this->window()) {
    return group;
  }
  
  TimeSeries::_sp sourceTs = this->source();
  time_t fromTime = *(times.begin());
  time_t toTime = *(times.rbegin());
  
  time_t w = this->window()->period();
  
  time_t t_lag  = 0, t_lead = 0;
  
  map<StatsSamplingMode_t, function<void()> > windowSetters({
    {StatsSamplingModeLeading,  [&](){t_lead += w;} },
    {StatsSamplingModeLagging,  [&](){t_lag += w;} },
    {StatsSamplingModeCentered, [&](){time_t h = w / 2; t_lag += h; t_lead += h;} }
  });
  
  windowSetters.at(this->samplingMode())();  
    
  // force a pre-cache on the source time series
  group.retainedCollection = sourceTs->pointCollection(TimeRange(fromTime - t_lag, toTime + t_lead));
  
  auto t1 = *(times.begin());
  // make a fake range to prepopulate the scanning range subgroup thingy
  PointCollection::pvRange previousRange = group.retainedCollection.subRange(TimeRange(t1,t1));
  
  for(const time_t& t : times) {
    // get sub-ranges of the larger pre-fetched collection
    TimeRange subrange(t - t_lag, t + t_lead);
    auto r = group.retainedCollection.subRange(subrange, previousRange);
    if (r.first != r.second) {
      group.ranges[t] = r;
      previousRange = r;
    }
  }
  
  return group;
}






