//
//  MultiplierTimeSeries.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __tsflib__MultiplierTimeSeries__
#define __tsflib__MultiplierTimeSeries__

#include <iostream>
#include "TimeSeriesFilterSecondary.h"

namespace TSF {
  class MultiplierTimeSeries : public TimeSeriesFilterSecondary {
    
  public:
    enum MultiplierMode : unsigned int {
      MultiplierModeMultiply =  0,
      MultiplierModeDivide =  1
    };
    
    TSF_BASE_PROPS(MultiplierTimeSeries);
    MultiplierTimeSeries();
    
    MultiplierMode multiplierMode();
    void setMultiplierMode(MultiplierMode mode);
    
    time_t timeBefore(time_t t);
    time_t timeAfter(time_t t);
    
    // chainable
    MultiplierTimeSeries::_sp mode(MultiplierMode mode) {this->setMultiplierMode(mode); return share_me(this);};
    
  protected:
    void didSetSecondary(TimeSeries::_sp secondary);
    PointCollection filterPointsInRange(TimeRange range);
    std::set<time_t> timeValuesInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    bool willResample();
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    MultiplierMode _mode;
    Units nativeUnits();
  };
}



#endif /* defined(__tsflib__MultiplierTimeSeries__) */
