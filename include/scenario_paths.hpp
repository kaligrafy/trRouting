#ifndef TR_SCENARIO_PATHS
#define TR_SCENARIO_PATHS

#include <vector>
#include <functional>

namespace TrRouting
{
  class TransitData;
  class Scenario;
  class Path;

  /**
   * @brief Get the paths that can be used in a calculation for a scenario, ie
   * the paths of the trips enabled by the scenario's connection set (services,
   * lines, agencies and modes filters).
   *
   * @param transitData The transit data containing the scenario
   * @param scenario The scenario for which to get the paths
   * @return std::vector<std::reference_wrapper<const Path>> The distinct paths,
   * sorted by uuid
   */
  std::vector<std::reference_wrapper<const Path>> getScenarioPaths(const TransitData &transitData, const Scenario &scenario);

}

#endif // TR_SCENARIO_PATHS
