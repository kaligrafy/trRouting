#include <boost/uuid/uuid_generators.hpp>

#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"
#include "min_transfers_journeys.hpp"

using PathUuidsJourneys = std::vector<std::vector<boost::uuids::uuid>>;

static PathUuidsJourneys toPathUuids(const std::vector<std::vector<std::reference_wrapper<const TrRouting::Path>>> &journeys)
{
  PathUuidsJourneys pathUuidsJourneys;
  for (const auto &journey : journeys)
  {
    pathUuidsJourneys.emplace_back();
    for (const TrRouting::Path &path : journey)
    {
      pathUuidsJourneys.back().push_back(path.uuid);
    }
  }
  return pathUuidsJourneys;
}

// Parameters: scenario uuid, origin stop uuid, destination stop uuid, expected journeys
class MinTransfersJourneysTest : public BaseCsaFixtureTests, public ::testing::WithParamInterface<std::tuple<boost::uuids::uuid, boost::uuids::uuid, boost::uuids::uuid, PathUuidsJourneys>> {};

TEST_P(MinTransfersJourneysTest, JourneysOfScenario)
{
  auto [scenarioUuid, originUuid, destinationUuid, expectedJourneys] = GetParam();
  TrRouting::StaticTransitNetwork network(TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(scenarioUuid)));

  auto journeys = TrRouting::getMinTransfersJourneys(network, {transitData.getNodes().at(originUuid)}, {transitData.getNodes().at(destinationUuid)});

  EXPECT_EQ(expectedJourneys, toPathUuids(journeys));
}

INSTANTIATE_TEST_SUITE_P(
  MinTransfersJourneys, MinTransfersJourneysTest,
  ::testing::Values(
    // No transfer, on the South/North path
    std::make_tuple(TestDataFetcher::scenarioUuid, TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeNorth2Uuid, PathUuidsJourneys({{TestDataFetcher::pathSNUuid}})),
    // Transfer at MidNode from South/North to East/West
    std::make_tuple(TestDataFetcher::scenarioUuid, TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeWest2Uuid, PathUuidsJourneys({{TestDataFetcher::pathSNUuid, TestDataFetcher::pathEWUuid}})),
    // Transfer at MidNode from East/West to South/North
    std::make_tuple(TestDataFetcher::scenarioUuid, TestDataFetcher::nodeEast2Uuid, TestDataFetcher::nodeNorth2Uuid, PathUuidsJourneys({{TestDataFetcher::pathEWUuid, TestDataFetcher::pathSNUuid}})),
    // Walking transfer from MidNode to East1, since East/West does not go back to East1
    std::make_tuple(TestDataFetcher::scenarioUuid, TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeExtra1Uuid, PathUuidsJourneys({{TestDataFetcher::pathSNUuid, TestDataFetcher::pathExtraUuid}})),
    std::make_tuple(TestDataFetcher::scenario2Uuid, TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeExtra1Uuid, PathUuidsJourneys({{TestDataFetcher::pathSNUuid, TestDataFetcher::pathExtraUuid}})),
    // Paths are only traveled in their stops order
    std::make_tuple(TestDataFetcher::scenarioUuid, TestDataFetcher::nodeNorth2Uuid, TestDataFetcher::nodeSouth2Uuid, PathUuidsJourneys()),
    // West2 is not served without the East/West line
    std::make_tuple(TestDataFetcher::scenario2Uuid, TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeWest2Uuid, PathUuidsJourneys()),
    // Same origin and destination
    std::make_tuple(TestDataFetcher::scenarioUuid, TestDataFetcher::nodeSouth2Uuid, TestDataFetcher::nodeSouth2Uuid, PathUuidsJourneys())
  )
);

// Network of 5 stops, A to E, where the paths are only defined by their stops
class MinTransfersJourneysNetworkTest : public ::testing::Test
{
protected:
  boost::uuids::random_generator uuidGenerator;
  TrRouting::Mode mode{"bus", "Bus", 3, 700};
  TrRouting::Agency agency;
  TrRouting::Line line{uuidGenerator(), agency, mode, "1", "Line", 0};
  std::vector<std::unique_ptr<TrRouting::Node>> nodes;
  std::vector<std::unique_ptr<TrRouting::Path>> paths;

  void SetUp() override
  {
    for (int i = 0; i < 5; i++)
    {
      nodes.push_back(std::make_unique<TrRouting::Node>(uuidGenerator(), 0, "", std::string(1, 'A' + i), std::make_unique<TrRouting::Point>(45.5, -73.6)));
    }
  }

  // Add a path through the stops, given by their indexes, and return it
  const TrRouting::Path &addPath(const std::vector<int> &nodeIndexes)
  {
    std::vector<std::reference_wrapper<const TrRouting::Node>> pathNodes;
    for (int index : nodeIndexes)
    {
      pathNodes.push_back(*nodes[index]);
    }
    paths.push_back(std::make_unique<TrRouting::Path>(uuidGenerator(), line, "outbound", pathNodes, std::vector<std::reference_wrapper<const TrRouting::Trip>>(), std::vector<int>(), std::vector<int>()));
    return *paths.back();
  }
};

TEST_F(MinTransfersJourneysNetworkTest, AllJourneysWithTheMinimumTransfers)
{
  const TrRouting::Path &directAE = addPath({0, 1, 4});
  const TrRouting::Path &directAE2 = addPath({0, 2, 4});
  // A-D then D-E needs a transfer, so it is not in the journeys
  const TrRouting::Path &pathAD = addPath({0, 3});
  const TrRouting::Path &pathDE = addPath({3, 4});
  TrRouting::StaticTransitNetwork network({directAE, directAE2, pathAD, pathDE});

  auto journeys = TrRouting::getMinTransfersJourneys(network, {*nodes[0]}, {*nodes[4]});

  EXPECT_EQ(PathUuidsJourneys({{directAE.uuid}, {directAE2.uuid}}), toPathUuids(journeys));
  EXPECT_EQ(PathUuidsJourneys({{directAE.uuid}}), toPathUuids(TrRouting::getMinTransfersJourneys(network, {*nodes[0]}, {*nodes[4]}, 1)));
}

TEST_F(MinTransfersJourneysNetworkTest, TransfersAtSameStopAndByWalkingAreDistinctSequences)
{
  // A-B then B-E, or A-B, walk from B to C, then C-E. Transfers count, not the walking or the number of stops
  nodes[1]->transferableNodes.push_back(TrRouting::NodeTimeDistance(*nodes[2], 60, 60));
  const TrRouting::Path &pathAB = addPath({0, 1});
  const TrRouting::Path &pathBE = addPath({1, 3, 4});
  const TrRouting::Path &pathCE = addPath({2, 4});
  TrRouting::StaticTransitNetwork network({pathAB, pathBE, pathCE});

  auto journeys = TrRouting::getMinTransfersJourneys(network, {*nodes[0]}, {*nodes[4]});

  EXPECT_EQ(PathUuidsJourneys({{pathAB.uuid, pathBE.uuid}, {pathAB.uuid, pathCE.uuid}}), toPathUuids(journeys));
}

TEST_F(MinTransfersJourneysNetworkTest, SeveralOriginsAndDestinations)
{
  // From A or B to D or E: B-D is direct, while A-C then C-E needs a transfer
  const TrRouting::Path &pathAC = addPath({0, 2});
  const TrRouting::Path &pathCE = addPath({2, 4});
  const TrRouting::Path &pathBD = addPath({1, 3});
  TrRouting::StaticTransitNetwork network({pathAC, pathCE, pathBD});

  EXPECT_EQ(PathUuidsJourneys({{pathBD.uuid}}), toPathUuids(TrRouting::getMinTransfersJourneys(network, {*nodes[0], *nodes[1]}, {*nodes[3], *nodes[4]})));
  EXPECT_EQ(PathUuidsJourneys({{pathAC.uuid, pathCE.uuid}}), toPathUuids(TrRouting::getMinTransfersJourneys(network, {*nodes[0]}, {*nodes[3], *nodes[4]})));
}

TEST_F(MinTransfersJourneysNetworkTest, NoWalkingAtOriginAndDestination)
{
  // B-D could be reached by walking from the origin A to B and from D to the destination E, but A is only the last stop of B-A and E the first stop of E-C
  nodes[0]->transferableNodes.push_back(TrRouting::NodeTimeDistance(*nodes[1], 60, 60));
  nodes[4]->transferableNodes.push_back(TrRouting::NodeTimeDistance(*nodes[3], 60, 60));
  const TrRouting::Path &pathBA = addPath({1, 0});
  const TrRouting::Path &pathBD = addPath({1, 3});
  const TrRouting::Path &pathEC = addPath({4, 2});
  TrRouting::StaticTransitNetwork network({pathBA, pathBD, pathEC});

  EXPECT_TRUE(TrRouting::getMinTransfersJourneys(network, {*nodes[0]}, {*nodes[4]}).empty());
}
