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

  nlohmann::json pointFeature(const nlohmann::json &position, const std::string &name)
  {
    return {{"type", "Feature"}, {"geometry", {{"type", "Point"}, {"coordinates", position}}}, {"properties", {{"name", name}}}};
  }

  // Place near South2 as origin, and West2 as destination
  const TrRouting::Point origin{45.527, -73.589};
};

TEST_F(JourneysGeoJsonTest, FeaturePerJourneyWithLegsAndWalkingTransfers)
{
  const TrRouting::Path &pathSN = transitData.getPaths().at(TestDataFetcher::pathSNUuid);
  const TrRouting::Path &pathEW = transitData.getPaths().at(TestDataFetcher::pathEWUuid);
  // From the origin, near South2, to West2: walk to South2 then South2 to North1 on South/North and walk to West2,
  // or walk to South2, South2 to MidNode on South/North, walk to West1 and West1 to West2 on East/West
  std::vector<std::vector<TrRouting::JourneyLeg>> journeys = {
    {{pathSN, 0, 3}},
    {{pathSN, 0, 2}, {pathEW, 3, 4}}
  };

  nlohmann::json expected = {
    {"type", "FeatureCollection"},
    {"features", {
      pointFeature({-73.589, 45.527}, "origin"),
      pointFeature(position(TestDataFetcher::nodeWest2Uuid), "destination"),
      {
        {"type", "Feature"},
        {"geometry", {{"type", "MultiLineString"}, {"coordinates", {
          {{-73.589, 45.527}, position(TestDataFetcher::nodeSouth2Uuid)},
          {position(TestDataFetcher::nodeSouth2Uuid), position(TestDataFetcher::nodeSouth1Uuid), position(TestDataFetcher::nodeMidNodeUuid), position(TestDataFetcher::nodeNorth1Uuid)},
          {position(TestDataFetcher::nodeNorth1Uuid), position(TestDataFetcher::nodeWest2Uuid)}
        }}}},
        {"properties", {{"transfers", 0}, {"lines", {"01"}}, {"stroke", "#cf1717"}}}
      },
      {
        {"type", "Feature"},
        {"geometry", {{"type", "MultiLineString"}, {"coordinates", {
          {{-73.589, 45.527}, position(TestDataFetcher::nodeSouth2Uuid)},
          {position(TestDataFetcher::nodeSouth2Uuid), position(TestDataFetcher::nodeSouth1Uuid), position(TestDataFetcher::nodeMidNodeUuid)},
          {position(TestDataFetcher::nodeMidNodeUuid), position(TestDataFetcher::nodeWest1Uuid)},
          {position(TestDataFetcher::nodeWest1Uuid), position(TestDataFetcher::nodeWest2Uuid)}
        }}}},
        {"properties", {{"transfers", 1}, {"lines", {"01", "02"}}, {"stroke", "#17cf4d"}}}
      }
    }}
  };
  EXPECT_EQ(expected.dump(2), TrRouting::getJourneysGeoJson(journeys, origin, *transitData.getNodes().at(TestDataFetcher::nodeWest2Uuid).point).dump(2));
}

TEST_F(JourneysGeoJsonTest, NoJourney)
{
  const TrRouting::Point destination{45.53, -73.62};
  nlohmann::json expected = {{"type", "FeatureCollection"}, {"features", {pointFeature({-73.589, 45.527}, "origin"), pointFeature({-73.62, 45.53}, "destination")}}};
  EXPECT_EQ(expected.dump(2), TrRouting::getJourneysGeoJson({}, origin, destination).dump(2));
}
