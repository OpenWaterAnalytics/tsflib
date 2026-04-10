# 
#   /$$$$$$$$  /$$$$$$  /$$$$$$$$ /$$       /$$$$$$ /$$$$$$$ 
#  |__  $$__/ /$$__  $$| $$_____/| $$      |_  $$_/| $$__  $$
#     | $$   | $$  \__/| $$      | $$        | $$  | $$  \ $$
#     | $$   |  $$$$$$ | $$$$$   | $$        | $$  | $$$$$$$ 
#     | $$    \____  $$| $$__/   | $$        | $$  | $$__  $$
#     | $$    /$$  \ $$| $$      | $$        | $$  | $$  \ $$
#     | $$   |  $$$$$$/| $$      | $$$$$$$$ /$$$$$$| $$$$$$$/
#     |__/    \______/ |__/      |________/|______/|_______/ 
#                                                          
#                                                          
                                                          
                                                          
                                                          
cmake_minimum_required (VERSION 3.15)

project (TSFLIB)
SET (CMAKE_C_FLAGS "-std=c99")
SET (CMAKE_CXX_FLAGS "-Wall -std=c++17")
SET (CMAKE_POSITION_INDEPENDENT_CODE ON)
add_definitions(-DNO_MYSQL)

message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

IF(APPLE)
   FIND_LIBRARY(SECURITY_FRAMEWORK Security)
   FIND_LIBRARY(CORE_FRAMEWORK CoreFoundation)
   SET(EXTRA_LIBS ${SECURITY_FRAMEWORK} ${CORE_FRAMEWORK})
ENDIF (APPLE)

find_package(CURL REQUIRED)
find_package(Boost REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(sqlite_modern_cpp REQUIRED)
find_package(oatpp REQUIRED)
find_package(oatpp-openssl REQUIRED)
find_package(nlohmann_json REQUIRED)

# the tsf library
include_directories(
	./src
	)

add_library(tsflib
./src/AutoRunner.cpp
./src/AggregatorTimeSeries.cpp
./src/BaseStatsTimeSeries.cpp
./src/BufferPointRecord.cpp
./src/Clock.cpp
./src/ConcreteDbRecords.cpp
./src/ConstantTimeSeries.cpp
./src/CorrelatorTimeSeries.cpp
./src/Curve.cpp
./src/CurveFunction.cpp
./src/DbPointRecord.cpp
./src/FailoverTimeSeries.cpp
./src/FirstDerivative.cpp
./src/GainTimeSeries.cpp
./src/IdentifierUnitsList.cpp
./src/InfluxAdapter.cpp
./src/IntegratorTimeSeries.cpp
./src/InversionTimeSeries.cpp
./src/LagTimeSeries.cpp
./src/LogicTimeSeries.cpp
./src/MathOpsTimeSeries.cpp
./src/MetaTimeSeries.cpp
./src/MetricInfo.cpp
./src/MovingAverage.cpp
./src/MultiplierTimeSeries.cpp
./src/OffsetTimeSeries.cpp
./src/OutlierExclusionTimeSeries.cpp
./src/Point.cpp
./src/PointCollection.cpp
./src/PointRecord.cpp
./src/PointRecordTime.cpp
./src/SineTimeSeries.cpp
./src/SquareWaveTimeSeries.cpp
./src/SqliteAdapter.cpp
./src/StatsTimeSeries.cpp
./src/ThresholdTimeSeries.cpp
./src/TimeRange.cpp
./src/TimeSeries.cpp
./src/TimeSeriesFilter.cpp
./src/TimeSeriesFilterSecondary.cpp
./src/TimeSeriesFilterSinglePoint.cpp
./src/TimeSeriesLowess.cpp
./src/TimeSeriesQuery.cpp
./src/TimeSeriesSynthetic.cpp
./src/Units.cpp
./src/ValidRangeTimeSeries.cpp
./src/WhereClause.cpp
)

set_target_properties(
  tsflib PROPERTIES
  CXX_STANDARD 17
)

target_compile_definitions(tsflib PRIVATE MAXFLOAT=3.40282347e+38F)
set(tsflib_deps
	CURL::libcurl
	boost::boost
	SQLite::SQLite3
	sqlite_modern_cpp::sqlite_modern_cpp
	oatpp::oatpp
	oatpp::oatpp-openssl
	nlohmann_json::nlohmann_json
	)


target_link_libraries(
	tsflib
	${tsflib_deps}
)


add_executable(tsflib_test 
  ./test/test_main.cpp
	./test/test_units.cpp
	./test/test_curve.cpp
	./test/test_clock.cpp
	./test/test_record.cpp
	./test/test_influx.cpp
	./test/test_filters.cpp
)

target_link_libraries(tsflib_test
	tsflib
	${tsflib_deps}
)


add_test(tsflib_test tsflib_test)


install(DIRECTORY ./src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ./src/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(TARGETS tsflib DESTINATION lib)
