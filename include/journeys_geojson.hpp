#ifndef TR_JOURNEYS_GEOJSON
#define TR_JOURNEYS_GEOJSON

#include <vector>
#include <nlohmann/json.hpp>

#include "journey_legs.hpp"

namespace TrRouting
{

  /**
   * @brief Get journeys as a GeoJSON FeatureCollection, with one Feature per
   * journey. The geometry is a MultiLineString with, in travel order, the
   * stops traveled on each leg and the walking transfers between legs, if any.
   * The properties are the number of transfers and the short names of the
   * lines of the legs.
   *
   * @param journeys The journeys, as their legs, for example from getFewestSegmentsLegs
   * @return nlohmann::json The GeoJSON FeatureCollection
   */
  nlohmann::json getJourneysGeoJson(const std::vector<std::vector<JourneyLeg>> &journeys);

}

#endif // TR_JOURNEYS_GEOJSON
