#include <iomanip>
#include <string>
#include <boost/io/ios_state.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "osm_network_writer.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"
#include "line.hpp"
#include "mode.hpp"
#include "node.hpp"
#include "point.hpp"

namespace TrRouting
{

  // Escape the characters that are not allowed in an XML attribute value
  static std::string escapeXmlAttribute(const std::string &value)
  {
    std::string escaped;
    escaped.reserve(value.size());
    for (char character : value)
    {
      switch (character)
      {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += character;
      }
    }
    return escaped;
  }

  // Empty values are skipped, as OSM does not allow tags without value
  static void writeTag(std::ostream &output, const std::string &key, const std::string &value)
  {
    if (!value.empty())
    {
      output << "    <tag k=\"" << key << "\" v=\"" << escapeXmlAttribute(value) << "\"/>\n";
    }
  }

  void writeOsmNetwork(std::ostream &output, const StaticTransitNetwork &network)
  {
    // Restore the caller's stream formatting when leaving
    boost::io::ios_all_saver streamStateSaver(output);

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    // upload="never" prevents editors like JOSM from uploading this generated network to OpenStreetMap
    output << "<osm version=\"0.6\" generator=\"trRouting\" upload=\"never\">\n";
    // 7 decimals is the OSM coordinates precision
    output << std::fixed << std::setprecision(7);
    // The network nodes are sorted by uid, as OSM tools expect sorted nodes
    for (const auto &uidAndNode : network.getNodes())
    {
      const Node &node = uidAndNode.second;
      // Editors like JOSM require a version for objects with positive ids
      output << "  <node id=\"" << node.uid << "\" version=\"1\" lat=\"" << node.point->latitude << "\" lon=\"" << node.point->longitude << "\">\n";
      writeTag(output, "name", node.name);
      writeTag(output, "ref", node.code);
      writeTag(output, "trRouting:uuid", boost::uuids::to_string(node.uuid));
      output << "  </node>\n";
    }

    int wayId = 0;
    for (const Path &path : network.getPaths())
    {
      output << "  <way id=\"" << ++wayId << "\" version=\"1\">\n";
      for (const Node &node : path.nodesRef)
      {
        output << "    <nd ref=\"" << node.uid << "\"/>\n";
      }
      writeTag(output, "ref", path.line.shortname);
      writeTag(output, "name", path.line.longname);
      writeTag(output, "route", path.line.mode.shortname);
      writeTag(output, "direction", path.direction);
      // Paths are only traveled in their stops order
      writeTag(output, "oneway", "yes");
      writeTag(output, "trRouting:path_uuid", boost::uuids::to_string(path.uuid));
      writeTag(output, "trRouting:line_uuid", boost::uuids::to_string(path.line.uuid));
      output << "  </way>\n";
    }

    for (const auto &[fromUid, toUids] : network.getTransfers())
    {
      for (const Node::uid_t toUid : toUids)
      {
        output << "  <way id=\"" << ++wayId << "\" version=\"1\">\n";
        output << "    <nd ref=\"" << fromUid << "\"/>\n";
        output << "    <nd ref=\"" << toUid << "\"/>\n";
        writeTag(output, "highway", "footway");
        writeTag(output, "oneway", "yes");
        output << "  </way>\n";
      }
    }
    output << "</osm>\n";
  }

}
