#include <set>
#include <boost/uuid/uuid_generators.hpp>

#include "gtest/gtest.h"
#include "../benchmark_csa/random_nodes.hpp"
#include "node.hpp"
#include "point.hpp"

using namespace TrRouting;

static const size_t NODES_COUNT = 5;

class RandomNodesFixtureTest : public ::testing::Test
{
protected:
  std::map<boost::uuids::uuid, Node> nodes;

public:
  void SetUp() override
  {
    boost::uuids::random_generator uuidGenerator;
    for (size_t i = 0; i < NODES_COUNT; i++)
    {
      boost::uuids::uuid uuid = uuidGenerator();
      nodes.emplace(uuid, Node(uuid, i, "code" + std::to_string(i), "name" + std::to_string(i), std::make_unique<Point>(45.5 + i * 0.01, -73.6 + i * 0.01)));
    }
  }
};

// Parameters: requested count, expected number of sampled nodes
class RandomNodesCountTest : public RandomNodesFixtureTest, public ::testing::WithParamInterface<std::tuple<size_t, size_t>> {};

TEST_P(RandomNodesCountTest, SampleSizeAndUniqueness)
{
  auto [count, expectedSize] = GetParam();

  auto sampledNodes = sampleRandomNodes(nodes, count, 42);

  ASSERT_EQ(expectedSize, sampledNodes.size());
  std::set<boost::uuids::uuid> sampledUuids;
  for (const Node &node : sampledNodes)
  {
    sampledUuids.insert(node.uuid);
  }
  EXPECT_EQ(expectedSize, sampledUuids.size());
}

INSTANTIATE_TEST_SUITE_P(
  RandomNodesCounts, RandomNodesCountTest,
  ::testing::Values(
    std::make_tuple(0, 0),
    std::make_tuple(1, 1),
    std::make_tuple(3, 3),
    std::make_tuple(NODES_COUNT, NODES_COUNT),
    std::make_tuple(NODES_COUNT + 10, NODES_COUNT)
  )
);

TEST_F(RandomNodesFixtureTest, SameSeedSameSample)
{
  auto firstSample = sampleRandomNodes(nodes, 3, 1234);
  auto secondSample = sampleRandomNodes(nodes, 3, 1234);

  ASSERT_EQ(firstSample.size(), secondSample.size());
  for (size_t i = 0; i < firstSample.size(); i++)
  {
    EXPECT_EQ(firstSample[i].get().uuid, secondSample[i].get().uuid);
  }
}
