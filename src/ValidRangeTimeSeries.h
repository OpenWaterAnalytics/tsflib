//
//  ValidRangeTimeSeries.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __tsflib__ValidRangeTimeSeries__
#define __tsflib__ValidRangeTimeSeries__

#include <iostream>
#include "TimeSeriesFilter.h"

namespace TSF {
  class ValidRangeTimeSeries : public TimeSeriesFilter {
  public:
    TSF_BASE_PROPS(ValidRangeTimeSeries);
    ValidRangeTimeSeries();
    
    void setRange(double min, double max);
    std::pair<double, double> range();
    
    enum filterMode_t : int {saturate=0,drop=1};
    filterMode_t mode();
    void setMode(filterMode_t mode);
    
    // specific overrides for optimization
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
    // chainable
    ValidRangeTimeSeries::_sp range(double min, double max) {this->setRange(min,max); return share_me(this);};
    ValidRangeTimeSeries::_sp mode(filterMode_t m) {this->setMode(m); return share_me(this);};
    
  protected:
    virtual bool willResample(); // we are special !
    PointCollection filterPointsInRange(TimeRange range);
    bool canDropPoints();
    
  private:
    std::pair<double, double> _range;
    filterMode_t _mode;
  };
}

#endif /* defined(__tsflib__ValidRangeTimeSeries__) */
