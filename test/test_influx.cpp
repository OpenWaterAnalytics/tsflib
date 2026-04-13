//
//  test_influx.cpp
//  tsf-tests
//
//  Created by Devine, Cameron - Xylem on 2/22/22.
//
#include "test_main.h"
#include "ConcreteDbRecords.h"

#include "InfluxAdapter.h"

using namespace TSF;
using namespace std;

#include <iostream>
BOOST_AUTO_TEST_SUITE(influx)


BOOST_AUTO_TEST_CASE(influx_basic) {
  
  const string recordName("test record");
  const string connection("proto=HTTP&host=localhost&port=8086&db=DB&u=USER&p=PASS&validate=1");
  
  // create demo record
  DbPointRecord::_sp record(new InfluxDbPointRecord);
  record->setName(recordName);
  record->setConnectionString(connection);
  
  BOOST_CHECK_EQUAL(record->name(), recordName);
  BOOST_CHECK_EQUAL(record->connectionString(), connection);
}

BOOST_AUTO_TEST_SUITE_END()
