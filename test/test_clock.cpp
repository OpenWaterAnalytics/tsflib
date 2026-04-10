#include "test_main.h"
#include "Clock.h"

using namespace TSF;
using namespace std;
using namespace nlohmann;

////////////////////////
// clock serialization
BOOST_AUTO_TEST_SUITE(clock_serialization)

BOOST_AUTO_TEST_CASE(to_json_basic) {
  Clock c(3600, 60);
  c.setName("hourly");

  json j = c;

  BOOST_CHECK_EQUAL(j["name"].get<string>(), "hourly");
  BOOST_CHECK_EQUAL(j["period"].get<int>(), 3600);
  BOOST_CHECK_EQUAL(j["offset"].get<int>(), 60);
}

BOOST_AUTO_TEST_CASE(from_json_basic) {
  json j = {
    {"name", "five_min"},
    {"period", 300},
    {"offset", 0}
  };

  Clock c = j.get<Clock>();

  BOOST_CHECK_EQUAL(c.name(), "five_min");
  BOOST_CHECK_EQUAL(c.period(), 300);
  BOOST_CHECK_EQUAL(c.start(), 0);
}

BOOST_AUTO_TEST_CASE(roundtrip) {
  Clock original(900, 30);
  original.setName("quarter_hour");

  // serialize then deserialize
  json j = original;
  Clock restored = j.get<Clock>();

  BOOST_CHECK_EQUAL(restored.name(), original.name());
  BOOST_CHECK_EQUAL(restored.period(), original.period());
  BOOST_CHECK_EQUAL(restored.start(), original.start());
}

BOOST_AUTO_TEST_CASE(roundtrip_preserves_isRegular) {
  // setPeriod sets _isRegular = true when period > 0
  // make sure roundtrip through json preserves this behavior
  Clock c(7200, 0);
  c.setName("two_hour");

  json j = c;
  Clock restored = j.get<Clock>();

  // isValid depends on _isRegular being set correctly via setPeriod
  BOOST_CHECK_EQUAL(restored.isValid(0), true);
  BOOST_CHECK_EQUAL(restored.isValid(7200), true);
  BOOST_CHECK_EQUAL(restored.isValid(100), false);
}

BOOST_AUTO_TEST_CASE(to_json_default_clock) {
  Clock c;  // defaults: period=3600, start=0

  json j = c;

  BOOST_CHECK_EQUAL(j["name"].get<string>(), "");
  BOOST_CHECK_EQUAL(j["period"].get<int>(), 3600);
  BOOST_CHECK_EQUAL(j["offset"].get<int>(), 0);
}

BOOST_AUTO_TEST_CASE(from_json_missing_key_throws) {
  json j = {
    {"name", "bad"},
    {"period", 300}
    // missing "offset"
  };

  BOOST_CHECK_THROW(j.get<Clock>(), json::out_of_range);
}

BOOST_AUTO_TEST_CASE(to_json_zero_period) {
  Clock c(0, 0);
  c.setName("irregular");

  json j = c;

  BOOST_CHECK_EQUAL(j["period"].get<int>(), 0);
}

BOOST_AUTO_TEST_CASE(to_json_only_contains_expected_keys) {
  Clock c(3600, 0);
  c.setName("test");

  json j = c;

  BOOST_CHECK_EQUAL(j.size(), 3);
  BOOST_CHECK(j.contains("name"));
  BOOST_CHECK(j.contains("period"));
  BOOST_CHECK(j.contains("offset"));
}

BOOST_AUTO_TEST_CASE(from_json_ignores_extra_keys) {
  json j = {
    {"name", "test"},
    {"period", 600},
    {"offset", 10},
    {"class", "clock"},
    {"self", {{"uid", 1}, {"ref", "clock"}}}
  };

  // should not throw — extra keys are ignored
  Clock c = j.get<Clock>();

  BOOST_CHECK_EQUAL(c.name(), "test");
  BOOST_CHECK_EQUAL(c.period(), 600);
  BOOST_CHECK_EQUAL(c.start(), 10);
}

BOOST_AUTO_TEST_SUITE_END()
// clock_serialization
/////////////////////////

