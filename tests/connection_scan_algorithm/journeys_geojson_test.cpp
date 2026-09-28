#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "journeys_geojson.hpp"

class JourneysGeoJsonTest : public BaseCsaFixtureTests
{
protected:
  nlohmann::json position(const boost::uuids::uuid &nodeUuid)
  {
    const TrRouting::Node &node = transitData.getNodes().at(nodeUuid);
    return {node.point->longitude, node.point->latitude};
  }
};

TEST_F(JourneysGeoJsonTest, FeaturePerJourneyWithLegsAndWalkingTransfers)
{
  const TrRouting::Path &pathSN = transitData.getPaths().at(TestDataFetcher::pathSNUuid);
  const TrRouting::Path &pathEW = transitData.getPaths().at(TestDataFetcher::pathEWUuid);
  // South2 to North1 on South/North, then South2 to MidNode on South/North, walk to West1 and West1 to West2 on East/West
  std::vector<std::vector<TrRouting::JourneyLeg>> journeys = {
    {{pathSN, 0, 3}},
    {{pathSN, 0, 2}, {pathEW, 3, 4}}
  };

  nlohmann::json expected = {
    {"type", "FeatureCollection"},
    {"features", {
      {
        {"type", "Feature"},
        {"geometry", {{"type", "MultiLineString"}, {"coordinates", {
          {position(TestDataFetcher::nodeSouth2Uuid), position(TestDataFetcher::nodeSouth1Uuid), position(TestDataFetcher::nodeMidNodeUuid), position(TestDataFetcher::nodeNorth1Uuid)}
        }}}},
        {"properties", {{"transfers", 0}, {"lines", {"01"}}}}
      },
      {
        {"type", "Feature"},
        {"geometry", {{"type", "MultiLineString"}, {"coordinates", {
          {position(TestDataFetcher::nodeSouth2Uuid), position(TestDataFetcher::nodeSouth1Uuid), position(TestDataFetcher::nodeMidNodeUuid)},
          {position(TestDataFetcher::nodeMidNodeUuid), position(TestDataFetcher::nodeWest1Uuid)},
          {position(TestDataFetcher::nodeWest1Uuid), position(TestDataFetcher::nodeWest2Uuid)}
        }}}},
        {"properties", {{"transfers", 1}, {"lines", {"01", "02"}}}}
      }
    }}
  };
  EXPECT_EQ(expected.dump(2), TrRouting::getJourneysGeoJson(journeys).dump(2));
}

TEST_F(JourneysGeoJsonTest, NoJourney)
{
  EXPECT_EQ(nlohmann::json({{"type", "FeatureCollection"}, {"features", nlohmann::json::array()}}), TrRouting::getJourneysGeoJson({}));
}
