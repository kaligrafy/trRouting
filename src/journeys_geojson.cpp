#include <algorithm>
#include <cmath>
#include <cstdio>

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

    // Hexadecimal color of a journey, with hues spaced by the golden angle so that successive journeys have distinct colors
    std::string getJourneyColor(size_t journeyIndex)
    {
      const double hue = std::fmod(journeyIndex * 137.508, 360.0);
      const double saturation = 0.8;
      const double lightness = 0.45;
      // HSL to RGB conversion, n being 0 for red, 8 for green and 4 for blue
      const auto getChannel = [&](double n) {
        const double k = std::fmod(n + hue / 30, 12);
        const double value = lightness - saturation * std::min(lightness, 1 - lightness) * std::max(-1.0, std::min({k - 3, 9 - k, 1.0}));
        return (int) std::lround(value * 255);
      };
      char color[8];
      std::snprintf(color, sizeof(color), "#%02x%02x%02x", getChannel(0), getChannel(8), getChannel(4));
      return color;
    }
  }

  nlohmann::json getJourneysGeoJson(const std::vector<std::vector<JourneyLeg>> &journeys, const Point &origin, const Point &destination)
  {
    nlohmann::json features = {getPointFeature(origin, "origin"), getPointFeature(destination, "destination")};
    for (size_t journeyIndex = 0; journeyIndex < journeys.size(); journeyIndex++)
    {
      const std::vector<JourneyLeg> &legs = journeys[journeyIndex];
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
        {"properties", {{"transfers", legs.empty() ? 0 : legs.size() - 1}, {"lines", lines}, {"stroke", getJourneyColor(journeyIndex)}}}
      });
    }
    return {{"type", "FeatureCollection"}, {"features", features}};
  }

}
