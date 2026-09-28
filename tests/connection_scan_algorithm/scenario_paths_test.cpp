#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "scenario_paths.hpp"

// Parameters: scenario uuid, expected path uuids, sorted by uuid
class ScenarioPathsTest : public BaseCsaFixtureTests, public ::testing::WithParamInterface<std::tuple<boost::uuids::uuid, std::vector<boost::uuids::uuid>>> {};

TEST_P(ScenarioPathsTest, PathsOfScenarioTrips)
{
  auto [scenarioUuid, expectedPathUuids] = GetParam();

  auto paths = TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(scenarioUuid));

  std::vector<boost::uuids::uuid> pathUuids;
  for (const TrRouting::Path &path : paths)
  {
    pathUuids.push_back(path.uuid);
  }
  EXPECT_EQ(expectedPathUuids, pathUuids);
}

INSTANTIATE_TEST_SUITE_P(
  ScenarioPaths, ScenarioPathsTest,
  ::testing::Values(
    // All lines
    std::make_tuple(TestDataFetcher::scenarioUuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathSNUuid, TestDataFetcher::pathExtraUuid, TestDataFetcher::pathEWUuid})),
    // Without the East/West line
    std::make_tuple(TestDataFetcher::scenario2Uuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathSNUuid, TestDataFetcher::pathExtraUuid}))
  )
);
