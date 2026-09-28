#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <boost/uuid/string_generator.hpp>

#include "gtest/gtest.h"
#include "cache_fetcher.hpp"
#include "transit_data.hpp"
#include "scenario.hpp"
#include "path.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"
#include "min_transfers_journeys.hpp"
#include "journey_legs.hpp"
#include "journeys_geojson.hpp"
#include "random_nodes.hpp"
#include "euclideangeofilter.hpp"
#include "parameters.hpp"
#include "point.hpp"

/**
 * Benchmarks of the journeys search with the minimum number of transfers, on
 * the static network of a scenario. Skipped unless these environment
 * variables are set:
 * - TR_BENCHMARK_CACHE_PATH: directory of the cache data (*.capnpbin files)
 * - TR_BENCHMARK_SCENARIO_ID: uuid of the scenario
 */
class MinTransfersJourneysBenchmarkTest : public ::testing::Test
{
protected:
  inline static std::unique_ptr<TrRouting::CacheFetcher> cacheFetcher;
  inline static std::unique_ptr<TrRouting::TransitData> transitData;
  inline static std::unique_ptr<TrRouting::StaticTransitNetwork> network;
  inline static std::string scenarioName;

  // Load the scenario's network once for all the tests
  static void SetUpTestSuite()
  {
    const char *cachePath = std::getenv("TR_BENCHMARK_CACHE_PATH");
    const char *scenarioId = std::getenv("TR_BENCHMARK_SCENARIO_ID");
    if (cachePath == nullptr || scenarioId == nullptr)
    {
      return;
    }
    cacheFetcher = std::make_unique<TrRouting::CacheFetcher>(cachePath);
    transitData = std::make_unique<TrRouting::TransitData>(*cacheFetcher);
    ASSERT_EQ(TrRouting::DataStatus::READY, transitData->getDataStatus());
    const TrRouting::Scenario &scenario = transitData->getScenarios().at(boost::uuids::string_generator()(scenarioId));
    network = std::make_unique<TrRouting::StaticTransitNetwork>(TrRouting::getScenarioPaths(*transitData, scenario));
    scenarioName = scenario.name;
    std::cout << "Network of " << network->getPaths().size() << " paths and " << network->getNodes().size() << " stops for scenario " << scenarioName << std::endl;
  }

  static void TearDownTestSuite()
  {
    network.reset();
    transitData.reset();
    cacheFetcher.reset();
  }

  // An end of the journeys: its place and the stops where the journeys can start or end
  struct JourneysEnd
  {
    TrRouting::Point place;
    std::vector<std::reference_wrapper<const TrRouting::Node>> stops;
  };

  /**
   * End of the journeys given by the TR_BENCHMARK_<end>_ID environment
   * variable as a stop uuid, or by TR_BENCHMARK_<end>_LAT_LON as
   * "latitude,longitude" for the stops within walking distance of this place.
   * Without stops if none is set.
   */
  static JourneysEnd getJourneysEnd(const std::string &end)
  {
    const char *id = std::getenv(("TR_BENCHMARK_" + end + "_ID").c_str());
    const char *latLon = std::getenv(("TR_BENCHMARK_" + end + "_LAT_LON").c_str());
    if (id != nullptr)
    {
      const TrRouting::Node &stop = transitData->getNodes().at(boost::uuids::string_generator()(id));
      return {*stop.point, {stop}};
    }
    if (latLon == nullptr)
    {
      return {};
    }
    const std::string latLonValue(latLon);
    const size_t comma = latLonValue.find(',');
    const TrRouting::Point place(std::stod(latLonValue.substr(0, comma)), std::stod(latLonValue.substr(comma + 1)));
    // Same maximum walking time and speed as the default calculation parameters. Only the stops are kept, not the walking time
    const float walkingSpeedMetersPerSecond = 5 / 3.6;
    std::vector<std::reference_wrapper<const TrRouting::Node>> stops;
    for (const TrRouting::NodeTimeDistance &footpath : TrRouting::EuclideanGeoFilter().getAccessibleNodesFootpathsFromPoint(place, transitData->getNodes(), TrRouting::DEFAULT_MAX_ACCESS_TRAVEL_TIME, walkingSpeedMetersPerSecond))
    {
      stops.push_back(footpath.node);
    }
    return {place, stops};
  }

  void SetUp() override
  {
    if (network == nullptr)
    {
      GTEST_SKIP() << "Set TR_BENCHMARK_CACHE_PATH and TR_BENCHMARK_SCENARIO_ID to find the journeys";
    }
  }
};

/**
 * Find the journeys between random pairs of stops served by the scenario.
 * Prints one CSV line per pair, then a summary by minimum number of transfers.
 * TR_BENCHMARK_PAIRS_COUNT (optional, 100 by default) is the number of pairs.
 */
TEST_F(MinTransfersJourneysBenchmarkTest, JourneysBetweenRandomStops)
{
  const char *pairsCountValue = std::getenv("TR_BENCHMARK_PAIRS_COUNT");
  const size_t pairsCount = pairsCountValue == nullptr ? 100 : std::stoul(pairsCountValue);

  auto origins = TrRouting::sampleRandomNodes(network->getNodes(), pairsCount, 1);
  auto destinations = TrRouting::sampleRandomNodes(network->getNodes(), pairsCount, 2);
  // The samples keep the order of the nodes, shuffle the destinations to pair stops independently of their order
  std::shuffle(destinations.begin(), destinations.end(), std::mt19937(3));

  // Statistics by minimum number of transfers, -1 for pairs without journey
  struct Statistics { size_t pairs = 0; size_t journeys = 0; size_t maxJourneys = 0; double durationMs = 0; double maxDurationMs = 0; };
  std::map<int, Statistics> statisticsByMinTransfers;

  std::cout << "origin_code,destination_code,min_transfers,journeys,duration_ms" << std::endl;
  for (size_t i = 0; i < std::min(origins.size(), destinations.size()); i++)
  {
    const TrRouting::Node &origin = origins[i];
    const TrRouting::Node &destination = destinations[i];
    auto startTime = std::chrono::steady_clock::now();
    auto journeys = TrRouting::getMinTransfersJourneys(*network, {origin}, {destination});
    const double durationMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();

    const int minTransfers = journeys.empty() ? -1 : journeys.front().size() - 1;
    std::cout << origin.code << "," << destination.code << "," << minTransfers << "," << journeys.size() << "," << durationMs << std::endl;
    Statistics &statistics = statisticsByMinTransfers[minTransfers];
    statistics.pairs++;
    statistics.journeys += journeys.size();
    statistics.maxJourneys = std::max(statistics.maxJourneys, journeys.size());
    statistics.durationMs += durationMs;
    statistics.maxDurationMs = std::max(statistics.maxDurationMs, durationMs);
  }

  std::cout << "min_transfers,pairs,mean_journeys,max_journeys,mean_duration_ms,max_duration_ms" << std::endl;
  for (const auto &[minTransfers, statistics] : statisticsByMinTransfers)
  {
    std::cout << minTransfers << "," << statistics.pairs << "," << (double) statistics.journeys / statistics.pairs << "," << statistics.maxJourneys << ","
              << statistics.durationMs / statistics.pairs << "," << statistics.maxDurationMs << std::endl;
  }
}

/**
 * Write the journeys between an origin and a destination as GeoJSON, with at
 * most 100 journeys. Skipped unless these environment variables are also set:
 * - TR_BENCHMARK_ORIGIN_ID (uuid of the origin stop) or TR_BENCHMARK_ORIGIN_LAT_LON ("latitude,longitude" of the origin place)
 * - TR_BENCHMARK_DESTINATION_ID or TR_BENCHMARK_DESTINATION_LAT_LON, the same for the destination
 * - TR_BENCHMARK_GEOJSON_OUTPUT: path of the .geojson file to write
 */
TEST_F(MinTransfersJourneysBenchmarkTest, JourneysGeoJson)
{
  const size_t maxJourneys = 100;
  const JourneysEnd origin = getJourneysEnd("ORIGIN");
  const JourneysEnd destination = getJourneysEnd("DESTINATION");
  const auto &origins = origin.stops;
  const auto &destinations = destination.stops;
  const char *outputPath = std::getenv("TR_BENCHMARK_GEOJSON_OUTPUT");
  if (origins.empty() || destinations.empty() || outputPath == nullptr)
  {
    GTEST_SKIP() << "Set the origin, destination and TR_BENCHMARK_GEOJSON_OUTPUT to write the journeys";
  }

  auto startTime = std::chrono::steady_clock::now();
  std::vector<std::vector<TrRouting::JourneyLeg>> journeysLegs;
  for (const auto &journey : TrRouting::getMinTransfersJourneys(*network, origins, destinations, maxJourneys))
  {
    journeysLegs.push_back(TrRouting::getFewestSegmentsLegs(*network, journey, origins, destinations));
  }
  const double durationMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();

  std::ofstream output(outputPath);
  ASSERT_TRUE(output.is_open()) << "Cannot open " << outputPath;
  output << TrRouting::getJourneysGeoJson(journeysLegs, origin.place, destination.place).dump();
  output.close();
  ASSERT_FALSE(output.fail()) << "Cannot write " << outputPath;

  std::cout << "Wrote " << journeysLegs.size() << " journeys from " << origins.size() << " origin stops to " << destinations.size() << " destination stops";
  if (!journeysLegs.empty())
  {
    std::cout << " with " << journeysLegs.front().size() - 1 << " transfers";
  }
  std::cout << " to " << outputPath << " in " << durationMs << " ms" << std::endl;
}
