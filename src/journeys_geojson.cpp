#include "journeys_geojson.hpp"
#include "path.hpp"
#include "line.hpp"
#include "node.hpp"
#include "point.hpp"

namespace TrRouting
{

  namespace
  {
    // GeoJSON position of a stop, as [longitude, latitude]
    nlohmann::json getPosition(const Node &node)
    {
      return {node.point->longitude, node.point->latitude};
    }
  }

  nlohmann::json getJourneysGeoJson(const std::vector<std::vector<JourneyLeg>> &journeys)
  {
    nlohmann::json features = nlohmann::json::array();
    for (const std::vector<JourneyLeg> &legs : journeys)
    {
      nlohmann::json lines = nlohmann::json::array();
      nlohmann::json lineStrings = nlohmann::json::array();
      for (size_t legIndex = 0; legIndex < legs.size(); legIndex++)
      {
        const JourneyLeg &leg = legs[legIndex];
        const Path &path = leg.path;
        if (legIndex > 0)
        {
          const Node &previousAlightingNode = legs[legIndex - 1].path.get().nodesRef[legs[legIndex - 1].alightingIndex];
          const Node &boardingNode = path.nodesRef[leg.boardingIndex];
          if (previousAlightingNode.uid != boardingNode.uid)
          {
            lineStrings.push_back({getPosition(previousAlightingNode), getPosition(boardingNode)});
          }
        }
        nlohmann::json positions = nlohmann::json::array();
        for (size_t index = leg.boardingIndex; index <= leg.alightingIndex; index++)
        {
          positions.push_back(getPosition(path.nodesRef[index]));
        }
        lineStrings.push_back(positions);
        lines.push_back(path.line.shortname);
      }

      features.push_back({
        {"type", "Feature"},
        {"geometry", {{"type", "MultiLineString"}, {"coordinates", lineStrings}}},
        {"properties", {{"transfers", legs.empty() ? 0 : legs.size() - 1}, {"lines", lines}}}
      });
    }
    return {{"type", "FeatureCollection"}, {"features", features}};
  }

}
