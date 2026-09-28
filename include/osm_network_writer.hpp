#ifndef TR_OSM_NETWORK_WRITER
#define TR_OSM_NETWORK_WRITER

#include <ostream>
#include <vector>
#include <functional>

namespace TrRouting
{
  class Path;

  /**
   * @brief Write transit paths as a static network in the OSM XML format
   * (version 0.6), without any time information.
   *
   * Each stop used by at least one path is an OSM node, whose id is the stop's
   * Node::uid (unique for the run, unlike Node::id) and with a trRouting:uuid
   * tag to map it back to the stop. Each path is an OSM way through its stops,
   * in order, tagged with its line and path data. Way ids are sequential,
   * starting at 1, in the order of the paths vector.
   *
   * @param output The stream to write the OSM XML to
   * @param paths The paths to write, for example from getScenarioPaths
   */
  void writeOsmNetwork(std::ostream &output, const std::vector<std::reference_wrapper<const Path>> &paths);

}

#endif // TR_OSM_NETWORK_WRITER
