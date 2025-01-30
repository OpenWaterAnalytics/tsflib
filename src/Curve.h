#ifndef Curve_hpp
#define Curve_hpp

#include <stdio.h>
#include <map>

#include "tsfMacros.h"
#include "Units.h"
#include "Point.h"
#include "TimeSeries.h"

namespace TSF {
  class Curve : public TSF_object {
  public:
    TSF_BASE_PROPS(Curve);
    bool operator==(const Curve& curve) const;
    bool operator!=(const Curve& curve) const;

    std::string name;
    Units inputUnits;
    Units outputUnits;
    std::map<double,double> curveData;
    
    PointCollection convert(const PointCollection& p, bool saturate = false);

  };
}


#endif /* Curve_hpp */
