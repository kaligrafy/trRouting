#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <boost/io/ios_state.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "osm_network_writer.hpp"
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

  void writeOsmNetwork(std::ostream &output, const std::vector<std::reference_wrapper<const Path>> &paths)
  {
    // Restore the caller's stream formatting when leaving
    boost::io::ios_all_saver streamStateSaver(output);

    // Stops used by the paths, sorted by OSM id, since OSM tools expect sorted nodes
    std::map<Node::uid_t, std::reference_wrapper<const Node>> nodesByUid;
    for (const Path &path : paths)
    {
      for (const Node &node : path.nodesRef)
      {
        nodesByUid.emplace(node.uid, node);
      }
    }

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    // upload="never" prevents editors like JOSM from uploading this generated network to OpenStreetMap
    output << "<osm version=\"0.6\" generator=\"trRouting\" upload=\"never\">\n";
    // 7 decimals is the OSM coordinates precision
    output << std::fixed << std::setprecision(7);
    for (const auto &uidAndNode : nodesByUid)
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
    for (const Path &path : paths)
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

    // Walking transfers between exported stops, as (from uid, to uid), to write each direction once, in order.
    // A transfer listed in only one direction is also added in the reverse direction.
    std::set<std::pair<Node::uid_t, Node::uid_t>> transfers;
    for (const auto &uidAndNode : nodesByUid)
    {
      for (const NodeTimeDistance &transfer : uidAndNode.second.get().transferableNodes)
      {
        const Node::uid_t toUid = transfer.node.uid;
        if (toUid != uidAndNode.first && nodesByUid.count(toUid) > 0)
        {
          transfers.emplace(uidAndNode.first, toUid);
          transfers.emplace(toUid, uidAndNode.first);
        }
      }
    }
    for (const auto &[fromUid, toUid] : transfers)
    {
      output << "  <way id=\"" << ++wayId << "\" version=\"1\">\n";
      output << "    <nd ref=\"" << fromUid << "\"/>\n";
      output << "    <nd ref=\"" << toUid << "\"/>\n";
      writeTag(output, "highway", "footway");
      writeTag(output, "oneway", "yes");
      output << "  </way>\n";
    }
    output << "</osm>\n";
  }

}
