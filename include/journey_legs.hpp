#ifndef TR_JOURNEY_LEGS
#define TR_JOURNEY_LEGS

#include <vector>
#include <functional>

namespace TrRouting
{
  class Node;
  class Path;
  class StaticTransitNetwork;

  /** @brief A leg of a journey: a path, boarded and left at stops given by their index in the path's nodesRef */
  struct JourneyLeg
  {
    std::reference_wrapper<const Path> path;
    size_t boardingIndex;
    size_t alightingIndex;
  };

  /**
   * @brief Get where to board and leave each path of a journey, for example
   * from getMinTransfersJourneys, to travel the fewest segments between stops.
   * Walking transfers do not count, as the network has no time information.
   * On equal number of segments, the earliest boarding and alighting stops are
   * kept.
   *
   * @param network The network of the journey
   * @param journey The sequence of paths of the journey
   * @param origins The stops where the first path can be boarded
   * @param destinations The stops where the last path can be left
   * @return std::vector<JourneyLeg> The legs of the journey, in travel order.
   * Empty if the paths cannot be traveled in sequence from an origin to a
   * destination.
   */
  std::vector<JourneyLeg> getFewestSegmentsLegs(const StaticTransitNetwork &network, const std::vector<std::reference_wrapper<const Path>> &journey, const std::vector<std::reference_wrapper<const Node>> &origins, const std::vector<std::reference_wrapper<const Node>> &destinations);

}

#endif // TR_JOURNEY_LEGS
