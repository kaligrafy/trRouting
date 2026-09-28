#ifndef TR_MIN_BOARDINGS
#define TR_MIN_BOARDINGS

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "node.hpp"

namespace TrRouting
{
  class StaticTransitNetwork;

  // Number of boardings needed to reach a destination when boarding at a stop, by stop uid
  using BoardingsByNode = std::unordered_map<Node::uid_t, int>;

  /**
   * @brief Find the minimum number of boardings to reach one of the
   * destinations when boarding a path at each stop of a static transit
   * network, without any time information.
   *
   * Rounds from the destinations back: round r finds the stops where boarding
   * reaches a destination with r boardings, by boarding a path before a stop
   * where round r - 1 can be boarded (or before a destination for round 1),
   * or before a stop with a walking transfer to it.
   *
   * @param network The network to search
   * @param destinationUids The uids of the stops where the last path can be left
   * @param isRoundNeeded Whether to compute a round, given its number (from 1)
   * and the boardings found by the previous rounds
   * @param isStopUsable Whether a stop can be boarded or left, all the stops by
   * default
   * @return BoardingsByNode The minimum number of boardings of the stops
   * reached by the computed rounds. The other stops are absent.
   */
  BoardingsByNode getMinBoardings(const StaticTransitNetwork &network,
                                  const std::unordered_set<Node::uid_t> &destinationUids,
                                  const std::function<bool(int, const BoardingsByNode &)> &isRoundNeeded,
                                  const std::function<bool(Node::uid_t)> &isStopUsable = [](Node::uid_t) { return true; });

}

#endif // TR_MIN_BOARDINGS
