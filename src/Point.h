//
//  Point.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef tsflib_point_h
#define tsflib_point_h

#include <time.h>
#include <map>
#include "tsfMacros.h"
#include "Units.h"

namespace TSF {
    
//!   A Point Class to store data tuples (date, value, quality, confidence)
/*!
      The point class keeps track of a piece of measurement data; time, value, and quality.
*/
  class Point {    
  public:
    
    // quality. an amalgamation of OPC standard codes and special "RTX" codes (identified by the 0b10xxxxxx prefix, otherwise unused by OPC)
    //
    
    enum PointQuality: uint8_t {
      
      opc_good         = 0b11000000,  // 192
      opc_bad          = 0b00000000,  // 0
      opc_uncertain    = 0b01000000,  // 64
      opc_tsf_override = 0b10000000,  // 128
      
      tsf_constant     = 0b00000001,
      tsf_interpolated = 0b00000010,
      tsf_averaged     = 0b00000100,
      tsf_aggregated   = 0b00001000,
      tsf_forecasted   = 0b00010000,
      tsf_integrated   = 0b00100000
      
    };
    
    
    //! Empty Constructor, equivalent to Point(0,0,Point::missing,0)
    Point();
    //! Full Constructor, for explicitly setting all internal data within the point object.
    Point(time_t time, double value = 0., PointQuality qual = opc_tsf_override, double confidence = 0.);
    // dtor
    ~Point();
    
    // operators
    Point operator+(const Point& point) const;
    Point& operator+=(const Point& point);
    Point operator+(const double value) const;
    Point& operator+=(const double value);
    Point operator*(const double factor) const;
    Point& operator*=(const double factor);
    Point operator*(const Point p) const;
    Point operator/(const double factor) const;
    Point operator/(const Point p) const;
//    virtual std::ostream& toStream(std::ostream& stream);

    // simple tuple class, so no getters/setters
    time_t time;
    double value;
    PointQuality quality;
    double confidence;
    bool isValid;
    
    // convenience
    const bool hasQual(PointQuality qual) const;
    void addQualFlag(PointQuality qual);
    bool notFound() const { return this->time == 0; };
    Point inverse() const;
    Point converted(const Units& fromUnits, const Units& toUnits) const;
    
    // static class methods
    static Point convertPoint(const Point& point, const Units& fromUnits, const Units& toUnits);
    static bool comparePointTime(const Point& left, const Point& right);
    static Point linearInterpolate(const Point& p1, const Point& p2, const time_t& t);
    
    
    
    friend std::ostream& operator<<(std::ostream& outputStream, const Point& p)
    {
      return outputStream << p.time << " - " << p.value << " - " << (p.isValid ? "valid" : "invalid") << std::endl;
    }
    
    
  };

  //std::ostream& operator<< (std::ostream &out, Point &point);

}

#endif