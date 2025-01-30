#include <math.h>

#include "test_main.h"
#include "Curve.h"
#include "Units.h"

using namespace TSF;
using namespace std;

////////////////////////
// units
BOOST_AUTO_TEST_SUITE(curve)

BOOST_AUTO_TEST_CASE(equals) {
  Curve* curveOne = new Curve();
  Curve* curveTwo = new Curve();

  // try some actual values we see in pump curves
  auto map1 = std::map<double,double>();
  map1.insert({1, 100.1});
  map1.insert({2, 40.300000000000011});
  map1.insert({3, 79128.864962292908});
  map1.insert({4, 182151.50557432874});
  map1.insert({5, 749904.44959719072});
  curveOne->curveData = map1;

  auto map2 = std::map<double,double>();
  map2.insert({1, 100.1});
  map2.insert({2, 40.300000032322211});
  map2.insert({3, 79128.864972625001});
  map2.insert({4, 182151.50555054471});
  map2.insert({5, 749904.44969510799});
  curveTwo->curveData = map2;

  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, true);
  BOOST_CHECK_EQUAL(*curveOne!=*curveTwo, false);

  curveOne->name = "1";
  curveTwo->name = "2";
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, false);
  curveTwo->name = "1";
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, true);


  curveOne->name = "1";
  curveTwo->name = "2";
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, false);
  curveTwo->name = "1";
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, true);

  map1.clear();
  map2.clear();

  map1.insert({1, 100.1000});
  map2.insert({1, 100.1001});
  curveOne->curveData = map1;
  curveTwo->curveData = map2;
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, false);

  map1.clear();
  map2.clear();
  curveOne->curveData = map1;
  curveTwo->curveData = map2;


  Units cfs = Units::unitOfType("cfs");
  Units mgd = Units::unitOfType("mgd");
  curveOne->inputUnits = cfs;
  curveTwo->inputUnits = mgd;
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, false);
  curveTwo->inputUnits = cfs;
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, true);
  curveOne->outputUnits = cfs;
  curveTwo->outputUnits = mgd;
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, false);
  curveTwo->outputUnits = cfs;
  BOOST_CHECK_EQUAL(*curveOne==*curveTwo, true);
}

BOOST_AUTO_TEST_SUITE_END()
// units
/////////////////////////

