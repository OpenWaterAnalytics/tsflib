//
//  BufferPointRecord.cpp
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "BufferPointRecord.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>

#include "WhereClause.h"

using namespace TSF;
using namespace std;



BufferPointRecord::BufferPointRecord(int defaultCapacity) {
  _defaultCapacity = defaultCapacity;
}


std::ostream& TSF::operator<< (std::ostream &out, BufferPointRecord &pr) {
  return pr.toStream(out);
}


std::ostream& BufferPointRecord::toStream(std::ostream &stream) {
  stream << "Buffered Point Record" << std::endl;
  return stream;
}


bool BufferPointRecord::registerAndGetIdentifierForSeriesWithUnits(std::string recordName, Units units) {
  // register the recordName internally and generate a buffer and mutex
  std::lock_guard lock(_buffer_readwrite); // get a write lock
  if (_keyedBuffers.find(recordName) != _keyedBuffers.end()) {
    // got the name - do the units match?
    Buffer b = _keyedBuffers[recordName];
    if (b.units == units) {
      // total match! use it.
      return true;
    }
    else {
      _keyedBuffers.erase(recordName);
    }
  }
  
  // check to see that it's not there
  if (_keyedBuffers.find(recordName) == _keyedBuffers.end()) {
    Buffer b;
    b.circularBuffer.set_capacity(_defaultCapacity);
    b.units = units;
    _keyedBuffers[recordName] = b;
  }
  
  return true;
}

IdentifierUnitsList BufferPointRecord::identifiersAndUnits() {
  IdentifierUnitsList list;
  std::map<std::string,pair<Units,string> > *ids = list.get();
  for (const auto &p : _keyedBuffers) {
    (*ids)[p.first] = {p.second.units, p.second.units.to_string()};
  }
  return list;
}

Point BufferPointRecord::point(const string& identifier, time_t time) {
  
  Point bp = PointRecord::point(identifier,time);
  if (bp.isValid) {
    return bp;
  }
  
  std::shared_lock lock(_buffer_readwrite); // get a read lock
  
  auto it = _keyedBuffers.find(identifier);
  if (it == _keyedBuffers.end()) {
    // nobody here by that name
    return Point();
  }
  
  // get the constituents
  PointBuffer& buffer = (it->second.circularBuffer);
  
  if (buffer.empty()) {
    return Point();
  }
  
  Point pFirst = buffer.front();
  Point pLast = buffer.back();
  
  if (pFirst.time <= time && time <= pLast.time) {
    // search the buffer
    Point finder(time, 0);
    PointBuffer::iterator startIterator = buffer.begin();
    PointBuffer::iterator pbIt = std::lower_bound(startIterator, buffer.end(), finder, &Point::comparePointTime);
    if (pbIt != buffer.end() && pbIt->time == time) {
      Point p = *pbIt;
      PointRecord::addPoint(identifier, p);
      return p;
    }
    else {
      return Point();
    }
  }
  
  
  return Point();
}



Point BufferPointRecord::pointBefore(const string& identifier, time_t time, WhereClause q) {
  
  std::shared_lock lock(_buffer_readwrite); // get a read lock
  
  Point foundPoint;
  
  auto vRange = this->range(identifier);
  if (!vRange.contains(time)) {
    // don't bother if its' not in range
    return foundPoint;
  }
  
  //TimePointPair_t finder(time, PointPair_t(0,0));
  Point finder(time, 0);
  
  auto it = _keyedBuffers.find(identifier);
  if (it != _keyedBuffers.end()) {
    // get the constituents
    PointBuffer& buffer = (it->second.circularBuffer);
    
    PointBuffer::const_iterator it  = lower_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    if (it != buffer.begin()) {
      // we're not at the beginning, so there is a point before time
      if (it != buffer.end()) {
        // and we're not at the end, so the point is within the continuous buffer
        // we want the previous point
        foundPoint = *(--it);
        PointRecord::addPoint(identifier, foundPoint);
      }
      else if ((--it)->time == time - 1) {
        // edge case where end of buffer is adjacent to requested time
        foundPoint = *it;
        PointRecord::addPoint(identifier, foundPoint);
      }
    }
    
    if (q.clauses.empty()) {
      return foundPoint;
    }
    else {
      // there is a clause! use the time-bound point to start looking back...
      while (it != buffer.begin() && !q.filter(*it)) {
        --it;
      }
      if (it != buffer.begin()) {
        foundPoint = *it;
        PointRecord::addPoint(identifier, foundPoint);
        return foundPoint;
      }
    }
    
  }
  
  return foundPoint;
}

// pre-supposes that the time supplied is within my buffer.
Point BufferPointRecord::pointAfter(const string& identifier, time_t time, WhereClause q) {
  
  std::shared_lock lock(_buffer_readwrite); // get a read lock
  
  Point foundPoint;
  
  auto vRange = this->range(identifier);
  if (!vRange.contains(time)) {
    // don't bother if its' not in range
    return foundPoint;
  }
  
  //TimePointPair_t finder(time, PointPair_t(0,0));
  Point finder(time, 0);
  
  auto it = _keyedBuffers.find(identifier);
  if (it != _keyedBuffers.end()) {
    // get the constituents
    PointBuffer& buffer = (it->second.circularBuffer);
    
    PointBuffer::const_iterator it = upper_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    if (it != buffer.end()) {
      // OK we're not at the end, so there is a point after time
      if (it != buffer.begin() || (it->time == time + 1)) {
        // exclude the first point of the buffer, since this means that the requested time is outside my range.
        // either we're not at the beginning, so the point is within the continuous buffer -
        // or edge case where beginning of buffer is adjacent to requested time
        foundPoint = *it;
        PointRecord::addPoint(identifier, foundPoint); // single point cache layer
      }
    }
    
    if (q.clauses.empty()) {
      return foundPoint;
    }
    else {
      // there is a clause! use the time-bound point to start looking back...
      while (it != buffer.end() && !q.filter(*it)) {
        ++it;
      }
      if (it != buffer.end()) {
        foundPoint = *it;
        PointRecord::addPoint(identifier, foundPoint);
        return foundPoint;
      }
    }
    
  }
  
  return foundPoint;
}


std::vector<Point> BufferPointRecord::pointsInRange(const string& identifier, TimeRange range) {
  
  std::shared_lock lock(_buffer_readwrite); // get a read lock
  
  std::vector<Point> pointVector;
  
  //TimePointPair_t finder(startTime, PointPair_t(0,0));
  Point finder(range.start, 0);
  
  auto it = _keyedBuffers.find(identifier);
  if (it != _keyedBuffers.end()) {
    // get the constituents
    PointBuffer& buffer = (it->second.circularBuffer);
    
    PointBuffer::const_iterator it = lower_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    while ( (it != buffer.end()) && (it->time <= range.end) ) {
      Point newPoint = *it;
      pointVector.push_back(newPoint);
      it++;
    }
  }
  
  return pointVector;
}


void BufferPointRecord::addPoint(const string& identifier, Point point) {
  
  PointRecord::addPoint(identifier, point);
  
  // do something more interesting in your derived class.
  // why nothing smart? because how can you ensure that a point you want to insert here is contiguous?
  // there's no way without knowing about clocks and all that business.
  
}


void BufferPointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  
  std::lock_guard lock(_buffer_readwrite); // get a write lock
  
  // check the cache size, and upgrade if needed.
  auto it = _keyedBuffers.find(identifier);
  if (it != _keyedBuffers.end()) {
    PointBuffer& buffer = (it->second.circularBuffer);
    size_t capacity = buffer.capacity();
    if (capacity < points.size()) {
      // plenty of room
      buffer.set_capacity(points.size() + capacity);
    }
    
    // figure out the insert order...
    // if the set we're inserting has to be prepended to the buffer...
    
    time_t firstInsertionTime = points.front().time;
    time_t lastInsertionTime = points.back().time;
    
    TimeRange existingRange = BufferPointRecord::range(identifier);
    
    // make sure they're in order
    std::sort(points.begin(), points.end(), &Point::comparePointTime);
    
    // scoped for clarity
    {
      // more gap detection? right on!
      bool gap = true;
      
      if (firstInsertionTime <= existingRange.end && existingRange.end < lastInsertionTime) {
        // some of these new points need to be inserted ono the end.
        // fast-fwd along the new points vector, ignoring any points that will not be appended.
        gap = false;
        Point finder(existingRange.end, 0);
        vector<Point>::const_iterator pIt = upper_bound(points.begin(), points.end(), finder, &Point::comparePointTime);
        // now insert these trailing points.
        while (pIt != points.end()) {
          if (pIt->time > existingRange.end) {
            //BufferPointRecord::addPoint(identifier, *pIt);
            // append to circular buffer.
            buffer.push_back(*pIt);
          }
          else {
            cerr << "BufferPointRecord: inserted points not ordered correctly" << endl;
          }
          ++pIt;
        }
      } // appending
      
      if (firstInsertionTime < existingRange.start && existingRange.start <= lastInsertionTime) {
        // some of the new points need to be pre-pended to the buffer.
        // insert onto front (reverse iteration)
        gap = false;
        vector<Point>::const_reverse_iterator pIt = points.rbegin();

        while (pIt != points.rend()) {
          // make this smarter? using upper_bound maybe? todo - figure out upper_bound with a reverse_iterator
          // skip overlapping points.
          
          if (pIt->time < existingRange.start) {
            buffer.push_front(*pIt);
          }
          // else { /* skip */ }
          
          ++pIt;
        }
      } // prepending
      
      if (existingRange.start <= firstInsertionTime && lastInsertionTime <= existingRange.end) {
        // complete overlap -- why did we even try to add these?
        gap = false;
      } // complete overlap
      
      // gap means that the points we're trying to insert do not overlap the existing points.
      // therefore, there's no guarantee of contiguity. so our only option is to clear out the existing cache
      // and insert the new points all by themselves.
      if (gap) {
        // clear the buffer and set the capacity to something more conservative.
        buffer.clear();
        buffer.set_capacity(points.size());
        
        // add new points.
        for(const Point &p : points) {
          buffer.push_back(p);
        }
      } // gap
    }// scoped
  }
  //else {
  //  DebugLog << "keyed buffer not found for id: " << identifier << EOL;
  //}
}



void BufferPointRecord::reset() {
  for (auto kb : _keyedBuffers) {
    auto bufferName = kb.first;
    auto buff = kb.second;
    auto cb = kb.second.circularBuffer;
    cb.clear();
  }
}

void BufferPointRecord::reset(const string& identifier) {
  std::lock_guard lock(_buffer_readwrite); // get a write lock
  auto it = _keyedBuffers.find(identifier);
  if (it != _keyedBuffers.end()) {
    PointBuffer& buffer = (it->second.circularBuffer);
    buffer.clear();
  }
}


Point BufferPointRecord::firstPoint(const string& id) {
  Point foundPoint;
  auto it = _keyedBuffers.find(id);
  if (it != _keyedBuffers.end()) {
    PointBuffer& buffer = (it->second.circularBuffer);
    if (buffer.empty()) {
      return foundPoint;
    }
    foundPoint = buffer.front(); // Point(tpPair.first, tpPair.second.first);
  }
  return foundPoint;
}

Point BufferPointRecord::lastPoint(const string& id) {
  Point foundPoint;
  auto it = _keyedBuffers.find(id);
  if (it != _keyedBuffers.end()) {
    // get the constituents
    PointBuffer& buffer = (it->second.circularBuffer);
    
    if (buffer.empty()) {
      return foundPoint;
    }
    
    //TimePointPair_t tpPair = buffer.back();
    foundPoint = buffer.back(); // Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}

TimeRange BufferPointRecord::range(const string& id) {
  return TimeRange(BufferPointRecord::firstPoint(id).time, BufferPointRecord::lastPoint(id).time);
}
