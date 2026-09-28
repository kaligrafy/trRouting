#include <map>
#include <boost/uuid/uuid.hpp>

#include "scenario_paths.hpp"
#include "transit_data.hpp"
#include "connection_set.hpp"
#include "path.hpp"
#include "trip.hpp"

namespace TrRouting
{

  std::vector<std::reference_wrapper<const Path>> getScenarioPaths(const TransitData &transitData, const Scenario &scenario)
  {
    // Many trips share the same path, the map keeps each path once, sorted by uuid
    std::map<boost::uuids::uuid, std::reference_wrapper<const Path>> pathsByUuid;
    for (const Trip &trip : transitData.getConnectionsForScenario(scenario)->getTrips())
    {
      pathsByUuid.emplace(trip.path.uuid, trip.path);
    }

    std::vector<std::reference_wrapper<const Path>> paths;
    paths.reserve(pathsByUuid.size());
    for (const auto &uuidAndPath : pathsByUuid)
    {
      paths.push_back(uuidAndPath.second);
    }
    return paths;
  }

}
