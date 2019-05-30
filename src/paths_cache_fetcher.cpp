
#ifndef TR_PATHS_CACHE_FETCHER
#define TR_PATHS_CACHE_FETCHER

#include <string>
#include <vector>

#include "cache_fetcher.hpp"
#include "path.hpp"
#include "capnp/pathCollection.capnp.h"
//#include "toolbox.hpp"

namespace TrRouting
{

  const std::pair<std::vector<Path>, std::map<boost::uuids::uuid, int>> CacheFetcher::getPaths(std::map<boost::uuids::uuid, int> lineIndexesByUuid, std::map<boost::uuids::uuid, int> nodeIndexesByUuid, Parameters& params, std::string customPath)
  { 

    using T           = Path;
    using TCollection = pathCollection::PathCollection;
    using cT          = pathCollection::Path;

    std::string tStr  = "paths";
    std::string TStr  = "Paths";

    std::vector<T> ts;
    std::string cacheFileName{tStr};
    std::map<boost::uuids::uuid, int> tIndexesByUuid;
    boost::uuids::string_generator uuidGenerator;
    
    std::cout << "Fetching " << tStr << " from cache..." << std::endl;
    
    if (CacheFetcher::capnpCacheFileExists(cacheFileName + ".capnpbin", params, customPath))
    {
      int fd = open((CacheFetcher::getFilePath(cacheFileName, params, customPath) + ".capnpbin").c_str(), O_RDWR);
      ::capnp::PackedFdMessageReader capnpTCollectionMessage(fd, {64 * 1024 * 1024});
      TCollection::Reader capnpTCollection = capnpTCollectionMessage.getRoot<TCollection>();
      for (cT::Reader capnpT : capnpTCollection.getPaths())
      {
        std::string uuid     {capnpT.getUuid()};
        std::string lineUuid {capnpT.getLineUuid()};
        std::vector<int> nodesIdx;
        std::vector<int> tripsIdx;
        boost::uuids::uuid nodeUuid;
        T * t                     = new T();
        t->uuid                   = uuidGenerator(uuid);
        t->direction              = capnpT.getDirection();
        t->lineIdx                = lineIndexesByUuid[uuidGenerator(lineUuid)];
        t->tripsIdx               = tripsIdx;
        for (std::string nodeUuidStr : capnpT.getNodesUuids())
        {
          nodeUuid = uuidGenerator(nodeUuidStr);
          nodesIdx.push_back(nodeIndexesByUuid[nodeUuid]);
        }
        t->nodesIdx = nodesIdx;
        ts.push_back(*t);
        tIndexesByUuid[t->uuid] = ts.size() - 1;
      }
      //std::cout << TStr << ":\n" << Toolbox::prettyPrintStructVector(ts) << std::endl;
      close(fd);
    }
    else
    {
      std::cerr << "missing " << tStr << " cache file!" << std::endl;
    }
    return std::make_pair(ts, tIndexesByUuid);
  }

}

#endif // TR_PATHS_CACHE_FETCHER