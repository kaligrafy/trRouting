#include <sstream>
#include <boost/uuid/uuid_generators.hpp>

#include "gtest/gtest.h"
#include "csa_test_base.hpp"
#include "osm_network_writer.hpp"
#include "scenario_paths.hpp"

// Count the non-overlapping occurrences of a pattern in a text
static size_t countOccurrences(const std::string &text, const std::string &pattern)
{
  size_t count = 0;
  for (size_t position = text.find(pattern); position != std::string::npos; position = text.find(pattern, position + pattern.size()))
  {
    count++;
  }
  return count;
}

// Parameters: scenario uuid, expected number of OSM nodes (distinct stops), expected number of OSM ways (paths)
class OsmNetworkWriterTest : public BaseCsaFixtureTests, public ::testing::WithParamInterface<std::tuple<boost::uuids::uuid, size_t, size_t>> {};

TEST_P(OsmNetworkWriterTest, NodesAndWaysOfScenario)
{
  auto [scenarioUuid, expectedNodesCount, expectedWaysCount] = GetParam();
  auto paths = TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(scenarioUuid));

  std::ostringstream output;
  TrRouting::writeOsmNetwork(output, paths);
  std::string osm = output.str();

  EXPECT_EQ(0u, osm.find("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm version=\"0.6\" generator=\"trRouting\" upload=\"never\">\n"));
  EXPECT_EQ(expectedNodesCount, countOccurrences(osm, "\" version=\"1\" lat=\""));
  EXPECT_EQ(expectedWaysCount, countOccurrences(osm, "\" version=\"1\">\n    <nd "));
  EXPECT_EQ(expectedNodesCount, countOccurrences(osm, "<node "));
  EXPECT_EQ(expectedWaysCount, countOccurrences(osm, "<way "));
  EXPECT_EQ(expectedWaysCount, countOccurrences(osm, "<tag k=\"oneway\" v=\"yes\"/>"));
  EXPECT_NE(std::string::npos, osm.find("</osm>\n"));

  // Each way lists its path's stops, in order
  for (const TrRouting::Path &path : paths)
  {
    std::string expectedNodeRefs;
    for (const TrRouting::Node &node : path.nodesRef)
    {
      expectedNodeRefs += "    <nd ref=\"" + std::to_string(node.uid) + "\"/>\n";
    }
    EXPECT_NE(std::string::npos, osm.find(expectedNodeRefs)) << "Missing node refs of path " << path.uuid;
  }
}

INSTANTIATE_TEST_SUITE_P(
  OsmNetworkWriter, OsmNetworkWriterTest,
  ::testing::Values(
    // All 10 stops, 3 paths
    std::make_tuple(TestDataFetcher::scenarioUuid, 10, 3),
    // Without the East/West line: the 5 South/North stops, East1 and Extra1
    std::make_tuple(TestDataFetcher::scenario2Uuid, 7, 2)
  )
);

TEST(OsmNetworkWriterFormatTest, EscapesValuesSkipsEmptyTagsAndRestoresStream)
{
  boost::uuids::random_generator uuidGenerator;
  TrRouting::Mode mode("bus", "Bus", 3, 700);
  TrRouting::Agency agency;
  agency.uuid = uuidGenerator();
  TrRouting::Line line(uuidGenerator(), agency, mode, "10A", "Rue <A> & \"B\"", 0);
  TrRouting::Node node1(uuidGenerator(), 0, "", "D'Iberville", std::make_unique<TrRouting::Point>(45.5, -73.6));
  TrRouting::Node node2(uuidGenerator(), 0, "123", "Other", std::make_unique<TrRouting::Point>(45.51, -73.61));
  TrRouting::Path path(uuidGenerator(), line, "outbound", {node1, node2}, {}, {60, -1}, {500, -1});

  std::ostringstream output;
  TrRouting::writeOsmNetwork(output, {path});
  std::string osm = output.str();

  EXPECT_NE(std::string::npos, osm.find("<tag k=\"name\" v=\"Rue &lt;A&gt; &amp; &quot;B&quot;\"/>"));
  EXPECT_NE(std::string::npos, osm.find("<tag k=\"name\" v=\"D&apos;Iberville\"/>"));
  EXPECT_NE(std::string::npos, osm.find("lat=\"45.5000000\" lon=\"-73.6000000\""));
  // Only node2 has a code, the empty code of node1 is not written
  EXPECT_EQ(1u, countOccurrences(osm, "<tag k=\"ref\" v=\"123\"/>"));
  EXPECT_EQ(2u, countOccurrences(osm, "<tag k=\"ref\""));

  // The fixed coordinates format must not leak to the caller's stream
  output.str("");
  output << 1.5;
  EXPECT_EQ("1.5", output.str());
}
