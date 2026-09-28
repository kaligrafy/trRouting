#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <boost/uuid/string_generator.hpp>

#include "gtest/gtest.h"
#include "cache_fetcher.hpp"
#include "transit_data.hpp"
#include "scenario.hpp"
#include "path.hpp"
#include "scenario_paths.hpp"
#include "osm_network_writer.hpp"

/**
 * Generate the static OSM network of the paths usable in a scenario and time
 * it. Skipped unless these environment variables are set:
 * - TR_BENCHMARK_CACHE_PATH: directory of the cache data (*.capnpbin files)
 * - TR_BENCHMARK_SCENARIO_ID: uuid of the scenario
 * - TR_BENCHMARK_OSM_OUTPUT: path of the .osm file to write
 */
TEST(OsmNetworkBenchmarkTest, GenerateScenarioOsmNetwork)
{
  const char *cachePath = std::getenv("TR_BENCHMARK_CACHE_PATH");
  const char *scenarioId = std::getenv("TR_BENCHMARK_SCENARIO_ID");
  const char *outputPath = std::getenv("TR_BENCHMARK_OSM_OUTPUT");
  if (cachePath == nullptr || scenarioId == nullptr || outputPath == nullptr)
  {
    GTEST_SKIP() << "Set TR_BENCHMARK_CACHE_PATH, TR_BENCHMARK_SCENARIO_ID and TR_BENCHMARK_OSM_OUTPUT to generate the OSM network";
  }

  TrRouting::CacheFetcher cacheFetcher(cachePath);
  TrRouting::TransitData transitData(cacheFetcher);
  ASSERT_EQ(TrRouting::DataStatus::READY, transitData.getDataStatus());
  const TrRouting::Scenario &scenario = transitData.getScenarios().at(boost::uuids::string_generator()(scenarioId));

  auto startTime = std::chrono::steady_clock::now();
  auto paths = TrRouting::getScenarioPaths(transitData, scenario);
  std::ofstream output(outputPath);
  ASSERT_TRUE(output.is_open()) << "Cannot open " << outputPath;
  TrRouting::writeOsmNetwork(output, paths);
  output.close();
  ASSERT_FALSE(output.fail()) << "Cannot write " << outputPath;
  auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

  std::cout << "Wrote " << paths.size() << " paths of scenario " << scenario.name << " to " << outputPath << " in " << durationMs << " ms" << std::endl;
}
