#ifndef TimeSeriesQuery_h
#define TimeSeriesQuery_h

#include <stdio.h>

#include "TimeSeries.h"
#include "DbPointRecord.h"


namespace TSF {
  class TimeSeriesQuery : public TimeSeries {
  public:
    TSF_BASE_PROPS(TimeSeriesQuery);
    TimeSeriesQuery();
    
    time_t timeAfter(time_t t);
    time_t timeBefore(time_t t);
    
    Point pointBefore(time_t time);
    Point pointAfter(time_t time);
    std::vector< Point > points(TimeRange range);
    
    PointRecord::_sp record();
    void setRecord(PointRecord::_sp record);
    
    std::string query();
    void setQuery(const std::string& query);
    
    std::string lastError();
    
  private:
    DbPointRecord::_sp _qRecord;
    std::string _query;
    std::string _error;
  };
}


#endif /* TimeSeriesQuery_h */
