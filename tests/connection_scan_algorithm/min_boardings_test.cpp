#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"
#include "min_boardings.hpp"

using BoardingsByUuid = std::map<boost::uuids::uuid, int>;

// Parameters: destination stop uuid, maximum number of rounds, uuids of the unusable stops, expected boardings by stop uuid
class MinBoardingsTest : public BaseCsaFixtureTests, public ::testing::WithParamInterface<std::tuple<boost::uuids::uuid, int, std::vector<boost::uuids::uuid>, BoardingsByUuid>> {};

TEST_P(MinBoardingsTest, BoardingsToDestination)
{
  auto [destinationUuid, maxRounds, unusableUuids, expectedBoardings] = GetParam();
  TrRouting::StaticTransitNetwork network(TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(TestDataFetcher::scenarioUuid)));
  std::unordered_set<TrRouting::Node::uid_t> unusableUids;
  for (const auto &uuid : unusableUuids)
  {
    unusableUids.insert(transitData.getNodes().at(uuid).uid);
  }

  auto boardingsByNode = TrRouting::getMinBoardings(network, {transitData.getNodes().at(destinationUuid).uid},
    [maxRounds = maxRounds](int round, const TrRouting::BoardingsByNode &) { return round <= maxRounds; },
    [&unusableUids](TrRouting::Node::uid_t uid) { return unusableUids.count(uid) == 0; });

  BoardingsByUuid boardings;
  for (const auto &[uid, count] : boardingsByNode)
  {
    boardings[network.getNodes().at(uid).get().uuid] = count;
  }
  EXPECT_EQ(expectedBoardings, boardings);
}

INSTANTIATE_TEST_SUITE_P(
  MinBoardings, MinBoardingsTest,
  ::testing::Values(
    // South/North to North2, then East/West to MidNode or to West1, which has a walking transfer to MidNode
    std::make_tuple(TestDataFetcher::nodeNorth2Uuid, 10, std::vector<boost::uuids::uuid>(), BoardingsByUuid({
      {TestDataFetcher::nodeSouth2Uuid, 1}, {TestDataFetcher::nodeSouth1Uuid, 1}, {TestDataFetcher::nodeMidNodeUuid, 1}, {TestDataFetcher::nodeNorth1Uuid, 1},
      {TestDataFetcher::nodeEast2Uuid, 2}, {TestDataFetcher::nodeEast1Uuid, 2}})),
    // Extra from East1, then East/West to East1 or MidNode, walking to East1, and South/North to MidNode. MidNode cannot be boarded to reach Extra1.
    std::make_tuple(TestDataFetcher::nodeExtra1Uuid, 10, std::vector<boost::uuids::uuid>(), BoardingsByUuid({
      {TestDataFetcher::nodeEast1Uuid, 1}, {TestDataFetcher::nodeEast2Uuid, 2}, {TestDataFetcher::nodeSouth2Uuid, 2}, {TestDataFetcher::nodeSouth1Uuid, 2}})),
    // Only the rounds needed
    std::make_tuple(TestDataFetcher::nodeExtra1Uuid, 1, std::vector<boost::uuids::uuid>(), BoardingsByUuid({{TestDataFetcher::nodeEast1Uuid, 1}})),
    // Unusable stops are neither boarded nor left
    std::make_tuple(TestDataFetcher::nodeExtra1Uuid, 10, std::vector<boost::uuids::uuid>({TestDataFetcher::nodeEast2Uuid, TestDataFetcher::nodeMidNodeUuid}), BoardingsByUuid({{TestDataFetcher::nodeEast1Uuid, 1}})),
    std::make_tuple(TestDataFetcher::nodeExtra1Uuid, 10, std::vector<boost::uuids::uuid>({TestDataFetcher::nodeExtra1Uuid}), BoardingsByUuid())
  )
);
