#include "Curve.h"


using namespace TSF;
using namespace std;


PointCollection Curve::convert(const PointCollection &pc, bool saturate) {
  PointCollection out;
  out.units = this->outputUnits;
  vector<Point> outp;
  PointCollection input = pc;
  if (!input.convertToUnits(this->inputUnits)) {
    return out;
  }
  
  double minX = curveData.cbegin()->first;
  double minY = curveData.cbegin()->second;
  double maxX = curveData.crbegin()->first;
  
  input.apply([&](Point& p){
    Point op;
    op.time = p.time;
    double inValue = p.value;
    double outValue = minY;
    
    double  x1 = minX,
            y1 = minY,
            x2 = minX,
            y2 = minY;
    
    if (saturate) {
      if (inValue < minX)
        inValue = minX;
      else if (inValue > maxX)
        inValue = maxX;
    }
    
    if (minX <= inValue && inValue <= maxX) {
      for (auto pp : curveData) {
        x2 = pp.first;
        y2 = pp.second;
        if (x2 > inValue) {
          break;
        }
        else {
          x1 = pp.first;
          y1 = pp.second;
        }
      }
      
      if (x1 == inValue) {
        outValue = y1;
      }
      else {
        outValue = y1 + ( (inValue - x1) * (y2 - y1) / (x2 - x1) );
      }
      
      op.value = outValue;
      op.quality = p.quality;
      op.addQualFlag(TSF::Point::tsf_interpolated);
      outp.push_back(op);
    }
  });
  out.setPoints(outp);
  return out;
}
