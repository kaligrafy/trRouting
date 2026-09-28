#ifndef TR_MIN_TRANSFERS_JOURNEYS
#define TR_MIN_TRANSFERS_JOURNEYS

#include <limits>
#include <vector>
#include <functional>

namespace TrRouting
{
  class Node;
  class Path;
  class StaticTransitNetwork;

  /**
   * @brief Find the journeys between origin and destination stops of a static
   * transit network with the minimum number of transfers, without any time
   * information.
   *
   * A journey is a distinct sequence of paths: the first path is boarded at
   * one of the origins, the last one is left at one of the destinations and,
   * between two consecutive paths, the transfer is at the same stop or by a
   * walking transfer. Paths are traveled in their stops order, and a path is
   * used at most once per journey. Only the number of transfers (number of
   * paths - 1) is minimized: the number of stops and the walking transfers do
   * not count, and all the origins and destinations are equivalent.
   *
   * @param network The network to search
   * @param origins The stops where the first path can be boarded, for example
   * the stops accessible from a place. Stops not in the network are ignored.
   * @param destinations The stops where the last path can be left. Stops not in
   * the network are ignored.
   * @param maxJourneys Maximum number of journeys to return, all by default
   * @return std::vector<std::vector<std::reference_wrapper<const Path>>> The
   * journeys with the minimum number of transfers, as sequences of paths. Empty
   * if no destination can be reached.
   */
  std::vector<std::vector<std::reference_wrapper<const Path>>> getMinTransfersJourneys(const StaticTransitNetwork &network, const std::vector<std::reference_wrapper<const Node>> &origins, const std::vector<std::reference_wrapper<const Node>> &destinations, size_t maxJourneys = std::numeric_limits<size_t>::max());

}

#endif // TR_MIN_TRANSFERS_JOURNEYS
