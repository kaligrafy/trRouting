#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"

// Parameters: scenario uuid, expected number of stops, expected transfers as pairs of stop uuids, in both directions
class StaticTransitNetworkTest : public BaseCsaFixtureTests, public ::testing::WithParamInterface<std::tuple<boost::uuids::uuid, size_t, std::vector<std::pair<boost::uuids::uuid, boost::uuids::uuid>>>> {};

TEST_P(StaticTransitNetworkTest, NodesAndTransfersOfScenario)
{
  auto [scenarioUuid, expectedNodesCount, expectedTransferUuids] = GetParam();
  auto paths = TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(scenarioUuid));

  TrRouting::StaticTransitNetwork network(paths);

  EXPECT_EQ(paths.size(), network.getPaths().size());
  EXPECT_EQ(expectedNodesCount, network.getNodes().size());
  std::set<std::pair<TrRouting::Node::uid_t, TrRouting::Node::uid_t>> expectedTransfers;
  for (const auto &[fromUuid, toUuid] : expectedTransferUuids)
  {
    expectedTransfers.emplace(transitData.getNodes().at(fromUuid).uid, transitData.getNodes().at(toUuid).uid);
    expectedTransfers.emplace(transitData.getNodes().at(toUuid).uid, transitData.getNodes().at(fromUuid).uid);
  }
  std::set<std::pair<TrRouting::Node::uid_t, TrRouting::Node::uid_t>> transfers;
  for (const auto &[fromUid, toUids] : network.getTransfers())
  {
    for (const TrRouting::Node::uid_t toUid : toUids)
    {
      transfers.emplace(fromUid, toUid);
    }
  }
  EXPECT_EQ(expectedTransfers, transfers);
}

INSTANTIATE_TEST_SUITE_P(
  StaticTransitNetwork, StaticTransitNetworkTest,
  ::testing::Values(
    // All 10 stops, with all transfers
    std::make_tuple(TestDataFetcher::scenarioUuid, 10, std::vector<std::pair<boost::uuids::uuid, boost::uuids::uuid>>({
      {TestDataFetcher::nodeWest1Uuid, TestDataFetcher::nodeWest2Uuid},
      {TestDataFetcher::nodeWest1Uuid, TestDataFetcher::nodeMidNodeUuid},
      {TestDataFetcher::nodeEast1Uuid, TestDataFetcher::nodeMidNodeUuid},
      {TestDataFetcher::nodeEast1Uuid, TestDataFetcher::nodeEast2Uuid},
      {TestDataFetcher::nodeNorth1Uuid, TestDataFetcher::nodeMidNodeUuid}})),
    // Without the East/West line, the transfers to West1, West2 and East2 are ignored, since these stops are not served
    std::make_tuple(TestDataFetcher::scenario2Uuid, 7, std::vector<std::pair<boost::uuids::uuid, boost::uuids::uuid>>({
      {TestDataFetcher::nodeEast1Uuid, TestDataFetcher::nodeMidNodeUuid},
      {TestDataFetcher::nodeNorth1Uuid, TestDataFetcher::nodeMidNodeUuid}}))
  )
);
