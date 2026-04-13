#include "test_main.h"
#include "Clock.h"
#include <glaze/glaze.hpp>

using namespace TSF;
using namespace std;

////////////////////////
// clock serialization
BOOST_AUTO_TEST_SUITE(clock_serialization)

BOOST_AUTO_TEST_CASE(to_json_basic) {
  Clock c(3600, 60);
  c.setName("hourly");

  auto json = glz::write_json(c).value_or("");
  BOOST_CHECK(!json.empty());

  Clock parsed;
  auto err = glz::read_json(parsed, json);
  BOOST_CHECK(!err);
  BOOST_CHECK_EQUAL(parsed.name(), "hourly");
  BOOST_CHECK_EQUAL(parsed.period(), 3600);
  BOOST_CHECK_EQUAL(parsed.start(), 60);
}

BOOST_AUTO_TEST_CASE(from_json_basic) {
  string json = R"({"name":"five_min","period":300,"offset":0})";

  Clock c;
  auto err = glz::read_json(c, json);
  BOOST_CHECK(!err);

  BOOST_CHECK_EQUAL(c.name(), "five_min");
  BOOST_CHECK_EQUAL(c.period(), 300);
  BOOST_CHECK_EQUAL(c.start(), 0);
}

BOOST_AUTO_TEST_CASE(roundtrip) {
  Clock original(900, 30);
  original.setName("quarter_hour");

  // serialize then deserialize
  auto json = glz::write_json(original).value_or("");

  Clock restored;
  auto err = glz::read_json(restored, json);
  BOOST_CHECK(!err);

  BOOST_CHECK_EQUAL(restored.name(), original.name());
  BOOST_CHECK_EQUAL(restored.period(), original.period());
  BOOST_CHECK_EQUAL(restored.start(), original.start());
}

BOOST_AUTO_TEST_CASE(roundtrip_preserves_isRegular) {
  // setPeriod sets _isRegular = true when period > 0
  // make sure roundtrip through json preserves this behavior
  Clock c(7200, 0);
  c.setName("two_hour");

  auto json = glz::write_json(c).value_or("");

  Clock restored;
  auto err = glz::read_json(restored, json);
  BOOST_CHECK(!err);

  // isValid depends on _isRegular being set correctly via setPeriod
  BOOST_CHECK_EQUAL(restored.isValid(0), true);
  BOOST_CHECK_EQUAL(restored.isValid(7200), true);
  BOOST_CHECK_EQUAL(restored.isValid(100), false);
}

BOOST_AUTO_TEST_CASE(to_json_default_clock) {
  Clock c;  // defaults: period=3600, start=0

  auto json = glz::write_json(c).value_or("");
  BOOST_CHECK(!json.empty());

  Clock parsed;
  auto err = glz::read_json(parsed, json);
  BOOST_CHECK(!err);
  BOOST_CHECK_EQUAL(parsed.name(), "");
  BOOST_CHECK_EQUAL(parsed.period(), 3600);
  BOOST_CHECK_EQUAL(parsed.start(), 0);
}

BOOST_AUTO_TEST_CASE(from_json_missing_key_uses_default) {
  // glaze uses default-initialized values for missing keys (no throw)
  string json = R"({"name":"bad","period":300})";

  Clock c;
  auto err = glz::read_json(c, json);
  BOOST_CHECK(!err);
  BOOST_CHECK_EQUAL(c.start(), 0);  // default-initialized
}

BOOST_AUTO_TEST_CASE(to_json_zero_period) {
  Clock c(0, 0);
  c.setName("irregular");

  auto json = glz::write_json(c).value_or("");
  Clock parsed;
  auto err = glz::read_json(parsed, json);
  BOOST_CHECK(!err);
  BOOST_CHECK_EQUAL(parsed.period(), 0);
}

BOOST_AUTO_TEST_CASE(to_json_only_contains_expected_keys) {
  Clock c(3600, 0);
  c.setName("test");

  auto json = glz::write_json(c).value_or("");

  BOOST_CHECK(!json.empty());
  // glaze produces: {"name":"test","period":3600,"offset":0}
  // verify 3 key-value pairs by counting colons
  auto count = std::count(json.begin(), json.end(), ':');
  BOOST_CHECK_EQUAL(count, 3);
}

BOOST_AUTO_TEST_SUITE_END()
// clock_serialization
/////////////////////////
