#ifndef TR_OSM_NETWORK_WRITER
#define TR_OSM_NETWORK_WRITER

#include <ostream>

namespace TrRouting
{
  class StaticTransitNetwork;

  /**
   * @brief Write a static transit network in the OSM XML format (version 0.6),
   * without any time information.
   *
   * Each stop of the network is an OSM node, whose id is the stop's Node::uid
   * and with a trRouting:uuid tag to map it back to the stop. Each path is a
   * oneway OSM way through its stops, in order, tagged with its line and path
   * data. Each walking transfer is a oneway highway=footway way. Way ids are
   * sequential, starting at 1: the paths in the network's order, then the
   * transfers.
   *
   * @param output The stream to write the OSM XML to
   * @param network The network to write
   */
  void writeOsmNetwork(std::ostream &output, const StaticTransitNetwork &network);

}

#endif // TR_OSM_NETWORK_WRITER
