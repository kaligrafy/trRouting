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
   * @brief Find the journeys between two stops of a static transit network
   * with the minimum number of transfers, without any time information.
   *
   * A journey is a distinct sequence of paths: the first path is boarded at
   * the origin, the last one is left at the destination and, between two
   * consecutive paths, the transfer is at the same stop or by a walking
   * transfer. Paths are traveled in their stops order, and a path is used at
   * most once per journey. Only the number of transfers (number of paths - 1)
   * is minimized: the number of stops and the walking transfers do not count.
   *
   * @param network The network to search
   * @param origin The stop where the first path is boarded
   * @param destination The stop where the last path is left
   * @param maxJourneys Maximum number of journeys to return, all by default
   * @return std::vector<std::vector<std::reference_wrapper<const Path>>> The
   * journeys with the minimum number of transfers, as sequences of paths. Empty
   * if the destination cannot be reached or is the origin.
   */
  std::vector<std::vector<std::reference_wrapper<const Path>>> getMinTransfersJourneys(const StaticTransitNetwork &network, const Node &origin, const Node &destination, size_t maxJourneys = std::numeric_limits<size_t>::max());

}

#endif // TR_MIN_TRANSFERS_JOURNEYS
