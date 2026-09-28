#ifndef TR_SCENARIO_NETWORK_BENCHMARK
#define TR_SCENARIO_NETWORK_BENCHMARK

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <boost/uuid/string_generator.hpp>

#include "gtest/gtest.h"
#include "cache_fetcher.hpp"
#include "transit_data.hpp"
#include "scenario.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"

/**
 * Base of the benchmarks on the static network of a scenario, loaded once
 * for all the tests of a suite. The tests are skipped unless these
 * environment variables are set:
 * - TR_BENCHMARK_CACHE_PATH: directory of the cache data (*.capnpbin files)
 * - TR_BENCHMARK_SCENARIO_ID: uuid of the scenario
 */
class ScenarioNetworkBenchmarkTest : public ::testing::Test
{
protected:
  inline static std::unique_ptr<TrRouting::CacheFetcher> cacheFetcher;
  inline static std::unique_ptr<TrRouting::TransitData> transitData;
  inline static std::unique_ptr<TrRouting::StaticTransitNetwork> network;
  inline static const TrRouting::Scenario *scenario = nullptr;

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
    scenario = &transitData->getScenarios().at(boost::uuids::string_generator()(scenarioId));
    network = std::make_unique<TrRouting::StaticTransitNetwork>(TrRouting::getScenarioPaths(*transitData, *scenario));
    std::cout << "Network of " << network->getPaths().size() << " paths and " << network->getNodes().size() << " stops for scenario " << scenario->name << std::endl;
  }

  static void TearDownTestSuite()
  {
    network.reset();
    scenario = nullptr;
    transitData.reset();
    cacheFetcher.reset();
  }

  void SetUp() override
  {
    if (network == nullptr)
    {
      GTEST_SKIP() << "Set TR_BENCHMARK_CACHE_PATH and TR_BENCHMARK_SCENARIO_ID to run the benchmark";
    }
  }
};

#endif // TR_SCENARIO_NETWORK_BENCHMARK
