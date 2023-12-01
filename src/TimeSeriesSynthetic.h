//
//  TimeSeriesSynthetic.h
//  tsflib
//
//  Created by Sam Hatchett on 1/19/15.
//
//

#ifndef __tsflib__TimeSeriesSynthetic__
#define __tsflib__TimeSeriesSynthetic__

#include <stdio.h>

#include "TimeSeries.h"
#include "Clock.h"

namespace TSF {
  class TimeSeriesSynthetic : public TimeSeries {
  public:
    TimeSeriesSynthetic();
    Point point(time_t time);
    Point pointBefore(time_t time);
    Point pointAfter(time_t time);
    std::vector< Point > points(TimeRange range);
    
    Clock::_sp clock();
    void setClock(Clock::_sp clock);
    
  protected:
    virtual Point syntheticPoint(time_t time) = 0;
    
  private:
    Clock::_sp _clock;
    
  };
}



#endif /* defined(__tsflib__TimeSeriesSynthetic__) */
