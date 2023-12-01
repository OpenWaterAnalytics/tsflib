//
//  MathOpsTimeSeries.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "TimeSeriesFilterSinglePoint.h"

#ifndef __tsflib__MathOpsTimeSeries__
#define __tsflib__MathOpsTimeSeries__

namespace TSF {
  class MathOpsTimeSeries : public TimeSeriesFilterSinglePoint {

  public:
    typedef enum {
      MathOpsTimeSeriesNoOp    = 0,  /*!< No operation; pass through. */
      MathOpsTimeSeriesAbs     = 1,  /*!< Absolute value. */
      MathOpsTimeSeriesLog     = 2,  /*!< Natural logarithm. */
      MathOpsTimeSeriesLog10   = 3,  /*!< Base 10 logarithm. */
      MathOpsTimeSeriesExp     = 4,  /*!< Exponential. */
      MathOpsTimeSeriesSqrt    = 5,  /*!< Square root. */
      MathOpsTimeSeriesPow     = 6,  /*!< Raise value to specified power. */
      MathOpsTimeSeriesExpBase = 7,  /*!< Exponential with specified base. */
      MathOpsTimeSeriesCeil    = 8,  /*!< Round up value. */
      MathOpsTimeSeriesFloor   = 9,  /*!< Round down value. */
      MathOpsTimeSeriesRound   = 10  /*!< Round to nearest. */
    } MathOpsTimeSeriesType;
    
    TSF_BASE_PROPS(MathOpsTimeSeries);
    MathOpsTimeSeries();
    
    MathOpsTimeSeriesType mathOpsType();
    void setMathOpsType(MathOpsTimeSeriesType type);
    
    double argument();
    void setArgument(double arg);

    virtual bool canAlterDimension() { return true; };
    
    // chainable
    MathOpsTimeSeries::_sp type(MathOpsTimeSeriesType t) {this->setMathOpsType(t); return share_me(this);};
    MathOpsTimeSeries::_sp arg(double v) {this->setArgument(v); return share_me(this);};
    
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    MathOpsTimeSeriesType _mathOpsType;
    double _arg;
    Units mathOpsUnits(Units sourceUnits, MathOpsTimeSeriesType type);
    
  };
}

#endif /* defined(__tsflib__MathOpsTimeSeries__) */
