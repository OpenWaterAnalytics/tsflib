//
//  CurveFunctionTimeSeries.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef tsflib_CurveFunctionTimeSeries_h
#define tsflib_CurveFunctionTimeSeries_h

#include <vector>
#include <boost/foreach.hpp>

#include "TimeSeriesFilter.h"
#include "CurveFunction.h"
#include "Curve.h"

namespace TSF {
  
  //!   A Curve Function Class to perform arbitrary mapping of values.
  /*!
   This time series class allows you to specify points on a curve for value transformation, for instance
   transforming a tank level time series into a volume time series. Generally, can be used for dimensional conversions.
   */
  
  class CurveFunction : public TimeSeriesFilter {
    
  public:
    TSF_BASE_PROPS(CurveFunction);
    CurveFunction();
    
    Curve::_sp curve();
    void setCurve(Curve::_sp curve);
    void clearCurve();
    
    // chainable
    CurveFunction::_sp curve(Curve::_sp curve) {this->setCurve(curve); return share_me(this);};
    
    void setDoesSaturate(bool saturate);
    
  protected:
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    bool willResample();
    PointCollection filterPointsInRange(TimeRange range);
    bool canDropPoints();
    
  private:
    Point convertWithCurve(Point p, Units sourceU);
    Curve::_sp _curve;
    bool _doesSaturate;
    
  };
}



#endif
