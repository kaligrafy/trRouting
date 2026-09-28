#ifndef TR_BOUNDED_JOURNEYS_ENUMERATOR
#define TR_BOUNDED_JOURNEYS_ENUMERATOR

#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

#include "node.hpp"
#include "min_boardings.hpp"

namespace TrRouting
{
  class CommonParameters;
  class Path;
  class StaticTransitNetwork;
  class Trip;

  // Usable trips of each path, sorted by departure time
  using TripsByPath = std::map<const Path *, std::vector<const Trip *>>;

  // Sequence of paths of a journey and its earliest arrival time at the destination
  struct PathsJourney
  {
    std::vector<std::reference_wrapper<const Path>> paths;
    int arrivalTime;
  };

  /**
   * @brief Depth-first enumeration of the sequences of paths from the origin to
   * the destination within time bounds.
   *
   * Only the stops where the arrival is at or before the latest arrival from
   * which the destination can still be reached in time are followed, and only
   * the transfers to stops from which the destination can be reached with the
   * remaining boardings. Each next path is boarded on its earliest trip from
   * any of the stops reachable on the current path. A path is used at most
   * once per sequence.
   */
  class BoundedJourneysEnumerator
  {
  public:
    /**
     * @param network The static network of the paths
     * @param tripsByPath The usable trips of each path, sorted by departure time
     * @param latestArrivals Latest arrival at each stop, by uid, to still reach
     * the destination in time, negative if it cannot be reached
     * @param nodesEgress The walking time and distance from the egress stops to
     * the destination, by uid
     * @param minBoardings The minimum number of boardings to reach the
     * destination from each stop, by uid
     * @param parameters The parameters of the calculation, for the departure
     * time, the waiting and walking times
     * @param maxArrivalTime The latest arrival at the destination
     * @param maxJourneys Maximum number of journeys to enumerate
     */
    BoundedJourneysEnumerator(const StaticTransitNetwork &network,
                              const TripsByPath &tripsByPath,
                              const std::vector<int> &latestArrivals,
                              const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesEgress,
                              const BoardingsByNode &minBoardings,
                              const CommonParameters &parameters,
                              int maxArrivalTime,
                              size_t maxJourneys);

    /**
     * @brief Enumerate the sequences of paths by increasing number of paths,
     * from the minimum up to `extraTransfers` more, so that `maxJourneys` only
     * drops those with the most transfers.
     *
     * @param nodesAccess The walking time and distance from the origin to the
     * access stops, by uid
     * @param maxPaths Maximum number of paths if no journey is found with fewer
     * @param extraTransfers Number of transfers more than the minimum
     * @return std::vector<PathsJourney> The sequences of paths, with their
     * earliest arrival
     */
    std::vector<PathsJourney> enumerate(const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesAccess, size_t maxPaths, size_t extraTransfers);

  private:
    // Earliest arrival at each stop of a boarded path, with the trip of this arrival. MAX_INT if not reachable.
    struct PathArrivals
    {
      std::vector<int> times;
      std::vector<const Trip *> trips;
    };

    void board(size_t pathIndex, size_t boardingIndex, int readyTime, int maxWaitingTime, std::map<size_t, PathArrivals> &nextPaths) const;
    void extendWithNextPaths(const std::map<size_t, PathArrivals> &nextPaths);
    void extend(size_t pathIndex, const PathArrivals &arrivals);

    const StaticTransitNetwork &network;
    const TripsByPath &tripsByPath;
    const std::vector<int> &latestArrivals;
    const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesEgress;
    const BoardingsByNode &minBoardings;
    const CommonParameters &parameters;
    const int maxArrivalTime;
    const size_t maxJourneys;
    std::vector<PathsJourney> journeys;
    std::vector<size_t> currentPaths;
    // Maximum number of paths of the current level
    size_t maxLevelPaths = 1;
  };

}

#endif // TR_BOUNDED_JOURNEYS_ENUMERATOR
