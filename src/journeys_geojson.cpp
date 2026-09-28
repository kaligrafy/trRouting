#include "journeys_geojson.hpp"
#include "path.hpp"
#include "line.hpp"
#include "node.hpp"
#include "point.hpp"

namespace TrRouting
{

  namespace
  {
    // GeoJSON position, as [longitude, latitude]
    nlohmann::json getPosition(const Point &point)
    {
      return {point.longitude, point.latitude};
    }

    nlohmann::json getPosition(const Node &node)
    {
      return getPosition(*node.point);
    }

    nlohmann::json getPointFeature(const Point &point, const std::string &name)
    {
      return {{"type", "Feature"}, {"geometry", {{"type", "Point"}, {"coordinates", getPosition(point)}}}, {"properties", {{"name", name}}}};
    }
  }

  nlohmann::json getJourneysGeoJson(const std::vector<std::vector<JourneyLeg>> &journeys, const Point &origin, const Point &destination)
  {
    nlohmann::json features = {getPointFeature(origin, "origin"), getPointFeature(destination, "destination")};
    for (const std::vector<JourneyLeg> &legs : journeys)
    {
      nlohmann::json lines = nlohmann::json::array();
      nlohmann::json lineStrings = nlohmann::json::array();
      // Walking from the origin, between legs and to the destination, only where the positions differ
      const auto addWalking = [&lineStrings](const nlohmann::json &from, const nlohmann::json &to) {
        if (from != to)
        {
          lineStrings.push_back({from, to});
        }
      };
      nlohmann::json previousPosition = getPosition(origin);
      for (const JourneyLeg &leg : legs)
      {
        const Path &path = leg.path;
        nlohmann::json positions = nlohmann::json::array();
        for (size_t index = leg.boardingIndex; index <= leg.alightingIndex; index++)
        {
          positions.push_back(getPosition(path.nodesRef[index]));
        }
        addWalking(previousPosition, positions.front());
        lineStrings.push_back(positions);
        previousPosition = positions.back();
        lines.push_back(path.line.shortname);
      }
      addWalking(previousPosition, getPosition(destination));

      features.push_back({
        {"type", "Feature"},
        {"geometry", {{"type", "MultiLineString"}, {"coordinates", lineStrings}}},
        {"properties", {{"transfers", legs.empty() ? 0 : legs.size() - 1}, {"lines", lines}}}
      });
    }
    return {{"type", "FeatureCollection"}, {"features", features}};
  }

}
