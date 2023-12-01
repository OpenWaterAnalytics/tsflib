//
//  StatsTimeSeries.h
//  tsflib
//
//  Created by Sam Hatchett on 7/18/14.
//
//

#ifndef __tsflib__StatsTimeSeries__
#define __tsflib__StatsTimeSeries__

#include <iostream>
#include "BaseStatsTimeSeries.h"
#include "Units.h"

namespace TSF {
  
  /*!
   \class StatsTimeSeries
   \brief A modular timeseries class for delivering statistical information.
   
   The StatsTimeSeries class is designed to provide a moving window (specified by the period length of a Clock in base-class parameter BaseStatsTimeSeries::window ), evaluated at regular intervals (also as a Clock), over which statistical measures are taken. The full list of statistical options is below.
   
   \sa BaseStatsTimeSeries::window
   */
  
  /*!
   \fn StatsTimeSeries::StatsTimeSeriesType StatsTimeSeries::statsType()
   \brief Get the statistic type.
   \return The type of statistic that this object is configured to return.
   \sa StatsTimeSeries::setStatsType
   */
  /*!
   \fn void StatsTimeSeries::setStatsType(StatsTimeSeriesType type)
   \brief Set the statistic type.
   \param type The type of statistic to calculate.
   \sa StatsTimeSeries::statsType
   */
  
  
  
  
  
  
  class StatsTimeSeries : public BaseStatsTimeSeries {
  public:
    /*! Statistic summary types */
    typedef enum {
      StatsTimeSeriesMean   = 0,  /*!< Mean value of population. */
      StatsTimeSeriesStdDev = 1,  /*!< Std Deviation of population. */
      StatsTimeSeriesMedian = 2,  /*!< A 2nd quartile estimate. Low populations may result in bad estimates. */
      StatsTimeSeriesQ25    = 3,  /*!< A 1st quartile estimate. */
      StatsTimeSeriesQ75    = 4,  /*!< A 3rd quartile estimate. */
      StatsTimeSeriesIQR = 5,  /*!< IQR == Q75 - Q25 */
      StatsTimeSeriesMax    = 6,  /*!< Maximum value in a population. */
      StatsTimeSeriesMin    = 7,  /*!< Minimum value in a population. */
      StatsTimeSeriesCount  = 8,  /*!< The number of points in the stats window. */
      StatsTimeSeriesVar    = 9,  /*!< The variance of the values in the population. */
      StatsTimeSeriesRMS    = 10,  /*!< Root mean squared. */
      StatsTimeSeriesPercentile = 11 /*!< Arbitrary percentile */
    } StatsTimeSeriesType;
    
    TSF_BASE_PROPS(StatsTimeSeries);
    StatsTimeSeries();
    
    StatsTimeSeriesType statsType();
    void setStatsType(StatsTimeSeriesType type);
    
    double arbitraryPercentile();
    void setArbitraryPercentile(double p);
    
    // chainable
    StatsTimeSeries::_sp type(StatsTimeSeriesType t) {this->setStatsType(t); return share_me(this);};
    StatsTimeSeries::_sp percentile(double p) {this->setArbitraryPercentile(p); return share_me(this);};
    
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    StatsTimeSeriesType _statsType;
    Units statsUnits(Units sourceUnits, StatsTimeSeriesType type);
    double _percentile;
    
  };
}

#endif /* defined(__tsflib__StatsTimeSeries__) */
