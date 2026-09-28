#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"
#include "journey_legs.hpp"

// Legs as their path uuid, boarding index and alighting index
using Legs = std::vector<std::tuple<boost::uuids::uuid, size_t, size_t>>;

// Parameters: origin stop uuid, destination stop uuid, journey as path uuids, expected legs
class JourneyLegsTest : public BaseCsaFixtureTests, public ::testing::WithParamInterface<std::tuple<boost::uuids::uuid, boost::uuids::uuid, std::vector<boost::uuids::uuid>, Legs>> {};

TEST_P(JourneyLegsTest, FewestSegmentsLegs)
{
  auto [originUuid, destinationUuid, pathUuids, expectedLegs] = GetParam();
  TrRouting::StaticTransitNetwork network(TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(TestDataFetcher::scenarioUuid)));
  std::vector<std::reference_wrapper<const TrRouting::Path>> journey;
  for (const boost::uuids::uuid &pathUuid : pathUuids)
  {
    journey.push_back(transitData.getPaths().at(pathUuid));
  }

  Legs legs;
  for (const TrRouting::JourneyLeg &leg : TrRouting::getFewestSegmentsLegs(network, journey, {transitData.getNodes().at(originUuid)}, {transitData.getNodes().at(destinationUuid)}))
  {
    legs.emplace_back(leg.path.get().uuid, leg.boardingIndex, leg.alightingIndex);
  }
  EXPECT_EQ(expectedLegs, legs);
}

INSTANTIATE_TEST_SUITE_P(
  JourneyLegs, JourneyLegsTest,
  ::testing::Values(
    // South2 to North2, the whole South/North path
    std::make_tuple(TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeNorth2Uuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathSNUuid}), Legs({{TestDataFetcher::pathSNUuid, 0, 4}})),
    // Leave South/North at MidNode and walk to West1, which saves the MidNode to West1 segment of East/West
    std::make_tuple(TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeWest2Uuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathSNUuid, TestDataFetcher::pathEWUuid}), Legs({{TestDataFetcher::pathSNUuid, 0, 2}, {TestDataFetcher::pathEWUuid, 3, 4}})),
    // Leaving East/West at East1 and walking to MidNode, or at MidNode and walking to North1, travel 3 segments, the earliest alighting is kept
    std::make_tuple(TestDataFetcher::nodeEast2Uuid, TestDataFetcher::nodeNorth2Uuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathEWUuid, TestDataFetcher::pathSNUuid}), Legs({{TestDataFetcher::pathEWUuid, 0, 1}, {TestDataFetcher::pathSNUuid, 2, 4}})),
    // Walk from MidNode to East1 to board Extra
    std::make_tuple(TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeExtra1Uuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathSNUuid, TestDataFetcher::pathExtraUuid}), Legs({{TestDataFetcher::pathSNUuid, 0, 2}, {TestDataFetcher::pathExtraUuid, 0, 1}})),
    // East/West does not serve South2
    std::make_tuple(TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeWest2Uuid, std::vector<boost::uuids::uuid>({TestDataFetcher::pathEWUuid}), Legs()),
    // Empty journey
    std::make_tuple(TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeNorth2Uuid, std::vector<boost::uuids::uuid>(), Legs())
  )
);

TEST_F(BaseCsaFixtureTests, FewestSegmentsLegsFromSeveralOriginsAndDestinations)
{
  TrRouting::StaticTransitNetwork network(TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(TestDataFetcher::scenarioUuid)));
  const TrRouting::Path &pathSN = transitData.getPaths().at(TestDataFetcher::pathSNUuid);
  const auto node = [this](const boost::uuids::uuid &uuid) -> const TrRouting::Node & { return transitData.getNodes().at(uuid); };

  // South1 to North1 travels 2 segments, the fewest from South2 or South1 to North1 or North2
  auto legs = TrRouting::getFewestSegmentsLegs(network, {pathSN}, {node(TestDataFetcher::nodeSouth2Uuid), node(TestDataFetcher::nodeSouth1Uuid)}, {node(TestDataFetcher::nodeNorth1Uuid), node(TestDataFetcher::nodeNorth2Uuid)});

  ASSERT_EQ(1u, legs.size());
  EXPECT_EQ(1u, legs[0].boardingIndex);
  EXPECT_EQ(3u, legs[0].alightingIndex);
}
