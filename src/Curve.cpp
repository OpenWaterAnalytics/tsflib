#include "Curve.h"


using namespace TSF;
using namespace std;
#define TINY 1.E-4;

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

bool Curve::operator==(const Curve& curve) const {
  if (this->name != curve.name) {
    return false;
  }

  if (this->curveData.size() != curve.curveData.size()) {
    return false;
  }

  // Create vectors from the curves
  struct Point {
      double x;
      double y;
  };
  std::vector<Point> vec1;
  for (const auto &p : curveData) {
    vec1.push_back({p.first, p.second});
  }
  std::vector<Point> vec2;
  for (const auto &p : curve.curveData) {
    vec2.push_back({p.first, p.second});
  }

  auto areDoublesEqual = [&](double x, double y) {
    double diff = fabs(x - y);
    return diff <= TINY;
  };
  auto arePointsEqual = [&](const Point& p1, const Point& p2) {
    return areDoublesEqual(p1.x, p2.x) && areDoublesEqual(p1.y, p2.y);
  };

  auto size = vec1.size();
  for (int i = 0; i < size; i++) {
    if (!arePointsEqual(vec1[i], vec2[i])) {
      return false;
    }
  }
  return true;
}

bool Curve::operator!=(const Curve& curve) const {
  return !(*this == curve);
}