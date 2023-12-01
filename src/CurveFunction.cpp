//
//  CurveFunction.cpp
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include <boost/foreach.hpp>

#include "CurveFunction.h"

using namespace TSF;
using namespace std;

CurveFunction::CurveFunction() {
  _doesSaturate = false;
}


Curve::_sp CurveFunction::curve() {
  return _curve;
}

void CurveFunction::setCurve(Curve::_sp curve) {
  _curve = curve;
  this->invalidate();
}

void CurveFunction::clearCurve() {
  Curve::_sp blank;
  this->setCurve(blank);
}

PointCollection CurveFunction::filterPointsInRange(TSF::TimeRange range) {
  
  if (!_curve || !this->source()) {
    return PointCollection();
  }
  
  TimeRange sourceQuery = range;
  if (this->willResample()) {
    sourceQuery = this->expandedRange(range);
  }
  
  // valid curve:
  PointCollection input = this->source()->pointCollection(sourceQuery);
  PointCollection output = _curve->convert(input, _doesSaturate);
  if (!output.convertToUnits(this->units())) {
    return PointCollection();
  }
  
  if (this->willResample()) {
    set<time_t> resTimes = this->timeValuesInRange(range);
    output.resample(resTimes);
  }
  
  output = output.trimmedToRange(range);
  
  return output;
}


bool CurveFunction::canSetSource(TimeSeries::_sp ts) {
  if (!_curve) {
    return true;
  }
  else if (_curve->inputUnits.isSameDimensionAs(ts->units())) {
    return true;
  }
  else {
    return false;
  }
}

void CurveFunction::didSetSource(TimeSeries::_sp ts) {
  
}

bool CurveFunction::canChangeToUnits(Units units) {
  if (_curve) {
    return _curve->outputUnits.isSameDimensionAs(units);
  }
  else {
    return true;
  }
}


bool CurveFunction::willResample() {
  return this->clock() ? true : false;
}

bool CurveFunction::canDropPoints() {
  return !_doesSaturate;
}

void CurveFunction::setDoesSaturate(bool saturate) {
  _doesSaturate = saturate;
}
