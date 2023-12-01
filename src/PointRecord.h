//
//  PointRecord.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef tsflib_PointRecord_h
#define tsflib_PointRecord_h

// basics
#include <string>
#include <vector>
#include <deque>
#include <fstream>
#include <map>


#include "Point.h"
#include "Units.h"
#include "tsfMacros.h"
#include "tsfExceptions.h"
#include "TimeRange.h"
#include "IdentifierUnitsList.h"
#include "WhereClause.h"


using std::string;

namespace TSF {
  
  /*! 
   \class PointRecord
   \brief A Point Record Class for storing and retrieving Points.
   
   The base PointRecord class just keeps short-term records. Derive to add specific persistence implementations
   */
  
  /*! 
   \fn virtual Point PointRecord::point(const std::string &name, time_t time) 
   \brief Get a Point with a specific name at a specific time.
   \param name The name of the data source (tag name).
   \param time The requested time.
   \return The requested Point (as a shared pointer).
   \sa Point
   */
  /*! 
   \fn std::vector<Point> PointRecord::pointsInRange(const std::string &name, time_t startTime, time_t endTime) 
   \brief Get a vector of Points with a specific name within a specific time range.
   \param name The name of the data source (tag name).
   \param startTime The beginning of the requested time range.
   \param endTime The end of the requested time range.
   \return The requested Points (as a vector of shared pointers)
   \sa Point
   */
  
    
  class PointRecord : public TSF_object {
    
  public:
    TSF_BASE_PROPS(PointRecord);
    
    PointRecord();
    virtual ~PointRecord() {};
    
    std::string name();
    void setName(std::string name);
    
    virtual bool registerAndGetIdentifierForSeriesWithUnits(std::string recordName, Units units);    // registering record names.
    virtual IdentifierUnitsList identifiersAndUnits();
    
    bool exists(const std::string& name, const Units& units);
    
    //virtual bool isPointAvailable(const string& identifier, time_t time);
    virtual Point point(const string& identifier, time_t time);
    virtual Point pointBefore(const string& identifier, time_t time, WhereClause q = WhereClause());
    virtual Point pointAfter(const string& identifier, time_t time, WhereClause q = WhereClause());
    virtual std::vector<Point> pointsInRange(const string& identifier, TimeRange range);
    virtual void addPoint(const string& identifier, Point point);
    virtual void addPoints(const string& identifier, std::vector<Point> points);
    virtual void reset(); // clear memcache for all ids
    virtual void reset(const string& identifier); // clear memcache for just this id
    virtual void invalidate(const string& identifier) {reset(identifier);}; // alias here, override for database implementations
    virtual Point firstPoint(const string& id);
    virtual Point lastPoint(const string& id);
    virtual TimeRange range(const string& id);
    virtual bool supportsQualifiedQuery() { return false; };
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    virtual void beginBulkOperation() {};
    virtual void endBulkOperation() {};
    
    
  protected:
    std::map<std::string,Point> _singlePointCache;
    IdentifierUnitsList _idsCache;
    
  private:
    std::string _name;
  
  };
  
  std::ostream& operator<< (std::ostream &out, PointRecord &pr);

}

#endif
