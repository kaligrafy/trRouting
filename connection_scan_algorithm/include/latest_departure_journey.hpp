#ifndef TR_LATEST_DEPARTURE_JOURNEY
#define TR_LATEST_DEPARTURE_JOURNEY

#include <deque>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>

#include "node.hpp"
#include "bounded_journeys_enumerator.hpp"

// journey_step.hpp needs Connection and Trip to be declared before it
namespace TrRouting
{
  class CommonParameters;
  class Connection;
}

#include "journey_step.hpp"

namespace TrRouting
{

  /**
   * @brief Find the steps of the journey of a sequence of paths arriving at
   * its arrival time, with the latest departure from the origin, by a reverse
   * pass on the paths of the sequence: on each path, the latest trip leaving
   * each stop in time for the transfer to the next path or for the egress.
   *
   * @param pathsJourney The sequence of paths and its arrival time at the destination
   * @param tripsByPath The usable trips of each path, sorted by departure time
   * @param nodesAccess The walking time and distance from the origin to the
   * access stops, by uid
   * @param nodesEgress The walking time and distance from the egress stops to
   * the destination, by uid
   * @param parameters The parameters of the calculation, for the departure
   * time, the waiting and walking times
   * @return The access step, the in-vehicle steps with the transfer after
   * each of them and the egress step, with the departure time from the
   * origin, or nothing if the sequence cannot arrive at this time
   */
  std::optional<std::pair<std::deque<JourneyStep>, int>> getLatestDepartureJourney(const PathsJourney &pathsJourney,
                                                                                  const TripsByPath &tripsByPath,
                                                                                  const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesAccess,
                                                                                  const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesEgress,
                                                                                  const CommonParameters &parameters);

}

#endif // TR_LATEST_DEPARTURE_JOURNEY
