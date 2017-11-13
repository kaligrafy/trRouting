#ifndef TR_CACHE_FETCHER
#define TR_CACHE_FETCHER

#include "data_fetcher.hpp"
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace TrRouting
{
  
  class CacheFetcher: public DataFetcher
  {
  
  public:
    
    CacheFetcher() {}
    CacheFetcher(std::string applicationShortname) {
      
    }

    template<class T>
    static const T loadFromCacheFile(T& data, std::string applicationShortname, std::string cacheFileName) {
      std::ifstream iCacheFile;
      iCacheFile.open(applicationShortname + "_" + cacheFileName + ".cache", std::ios::in | std::ios::binary);
      boost::archive::binary_iarchive iarch(iCacheFile);
      iarch >> data;
      iCacheFile.close();
      return data;
    }
    
  private:
    
  };
    
}

#endif // TR_CACHE_FETCHER