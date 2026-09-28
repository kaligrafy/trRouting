#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
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
#include "random_nodes.hpp"

/**
 * Find the journeys with the minimum number of transfers between random pairs
 * of stops served by a scenario and time the search. Prints one CSV line per
 * pair, then a summary by minimum number of transfers. Skipped unless these
 * environment variables are set:
 * - TR_BENCHMARK_CACHE_PATH: directory of the cache data (*.capnpbin files)
 * - TR_BENCHMARK_SCENARIO_ID: uuid of the scenario
 * - TR_BENCHMARK_PAIRS_COUNT (optional, 100 by default): number of pairs of stops
 */
TEST(MinTransfersJourneysBenchmarkTest, JourneysBetweenRandomStops)
{
  const char *cachePath = std::getenv("TR_BENCHMARK_CACHE_PATH");
  const char *scenarioId = std::getenv("TR_BENCHMARK_SCENARIO_ID");
  const char *pairsCountValue = std::getenv("TR_BENCHMARK_PAIRS_COUNT");
  if (cachePath == nullptr || scenarioId == nullptr)
  {
    GTEST_SKIP() << "Set TR_BENCHMARK_CACHE_PATH and TR_BENCHMARK_SCENARIO_ID to find the journeys";
  }
  const size_t pairsCount = pairsCountValue == nullptr ? 100 : std::stoul(pairsCountValue);

  TrRouting::CacheFetcher cacheFetcher(cachePath);
  TrRouting::TransitData transitData(cacheFetcher);
  ASSERT_EQ(TrRouting::DataStatus::READY, transitData.getDataStatus());
  const TrRouting::Scenario &scenario = transitData.getScenarios().at(boost::uuids::string_generator()(scenarioId));
  TrRouting::StaticTransitNetwork network(TrRouting::getScenarioPaths(transitData, scenario));

  auto origins = TrRouting::sampleRandomNodes(network.getNodes(), pairsCount, 1);
  auto destinations = TrRouting::sampleRandomNodes(network.getNodes(), pairsCount, 2);
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
    auto journeys = TrRouting::getMinTransfersJourneys(network, origin, destination);
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

  std::cout << "Network of " << network.getPaths().size() << " paths and " << network.getNodes().size() << " stops for scenario " << scenario.name << std::endl;
  std::cout << "min_transfers,pairs,mean_journeys,max_journeys,mean_duration_ms,max_duration_ms" << std::endl;
  for (const auto &[minTransfers, statistics] : statisticsByMinTransfers)
  {
    std::cout << minTransfers << "," << statistics.pairs << "," << (double) statistics.journeys / statistics.pairs << "," << statistics.maxJourneys << ","
              << statistics.durationMs / statistics.pairs << "," << statistics.maxDurationMs << std::endl;
  }
}
