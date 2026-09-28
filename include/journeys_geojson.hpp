#ifndef TR_JOURNEYS_GEOJSON
#define TR_JOURNEYS_GEOJSON

#include <vector>
#include <nlohmann/json.hpp>

#include "journey_legs.hpp"

namespace TrRouting
{

  class Point;

  /**
   * @brief Get journeys as a GeoJSON FeatureCollection. The first two
   * Features are the origin and destination Points, with "origin" and
   * "destination" as name property. Then each journey is a Feature whose
   * geometry is a MultiLineString with, in travel order, the stops traveled on
   * each leg and the walking, as straight lines, from the origin, between legs
   * and to the destination, where the positions differ. The properties
   * are the number of transfers, the short names of the lines of the legs and
   * a distinct color per journey, as the simplestyle `stroke` used by map
   * viewers like geojson.io.
   *
   * @param journeys The journeys, as their legs, for example from getFewestSegmentsLegs
   * @param origin The origin of the journeys, a place or a stop
   * @param destination The destination of the journeys, a place or a stop
   * @return nlohmann::json The GeoJSON FeatureCollection
   */
  nlohmann::json getJourneysGeoJson(const std::vector<std::vector<JourneyLeg>> &journeys, const Point &origin, const Point &destination);

}

#endif // TR_JOURNEYS_GEOJSON
