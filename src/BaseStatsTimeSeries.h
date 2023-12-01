//
//  BaseStatsTimeSeries.h
//  tsflib
//
//  Created by Sam Hatchett on 7/17/14.
//
//

#ifndef __tsflib__BaseStatsTimeSeries__
#define __tsflib__BaseStatsTimeSeries__

#include <iostream>
#include "TimeSeriesFilter.h"
#include "PointCollection.h"

namespace TSF {
  
  
  /*!
   \class BaseStatsTimeSeries
   \brief An abstract timeseries class for computing statistical data from point values.
   
   The BaseStatsTimeSeries class is designed to provide a moving window (specified by the period length of a Clock in #window ), evaluated at regular intervals (also as a Clock), over which statistical measures are taken. The window can lead, lag, or be centered on the time series' clock.
   
   */
  
  /*!
   \fn Clock::_sp BaseStatsTimeSeries::window();
   \brief Get the statistical sampling window clock.
   \return A shared pointer to a Clock object.
   \sa BaseStatsTimeSeries::setWindow
   */
  /*!
   \fn void BaseStatsTimeSeries::setWindow(Clock::_sp window);
   \brief Set the moving statistical window.
   \param window A moving window to use for statistical calculations
   \sa BaseStatsTimeSeries::window
   */

  
    
  class BaseStatsTimeSeries : public TimeSeriesFilter {
    
  public:
    //! sampling window mode (lead, lag, center)
    typedef enum {
      StatsSamplingModeLeading  = 0,  /*!< Use a leading window about the time series' clock */
      StatsSamplingModeLagging  = 1,  /*!< Use a lagging window */
      StatsSamplingModeCentered = 2   /*!< Use a centered sampling window */
    } StatsSamplingMode_t;
    
    typedef std::map< time_t,PointCollection::pvRange > subrangeMap; 
    
    struct rangeGroup {
      PointCollection retainedCollection;
      subrangeMap ranges;
    };
    
    TSF_BASE_PROPS(BaseStatsTimeSeries);
    BaseStatsTimeSeries();
    
    void setWindow(Clock::_sp window);
    Clock::_sp window();
    
    void setSamplingMode(StatsSamplingMode_t mode);
    StatsSamplingMode_t samplingMode();
    
    bool summaryOnly();
    void setSummaryOnly(bool summaryOnly);
    
    // chaining methods
    BaseStatsTimeSeries::_sp window(Clock::_sp w) {this->setWindow(w); return share_me(this);};
    BaseStatsTimeSeries::_sp mode(StatsSamplingMode_t mode) {this->setSamplingMode(mode); return share_me(this);};
    
  protected:
    virtual PointCollection filterPointsInRange(TimeRange range) = 0; // pure virtual. don't use this class directly.
    rangeGroup subRanges(std::set<time_t> times);
    
  private:
    Clock::_sp _window;
    bool _summaryOnly;
    StatsSamplingMode_t _samplingMode;
  };
}



#endif /* defined(__tsflib__QuartileTimeSeries__) */
