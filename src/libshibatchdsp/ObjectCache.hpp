#ifndef OBJECTCACHE_HPP
#define OBJECTCACHE_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace ssrc {
  template<typename T>
  class ObjectCache {
    struct Internal {
      std::unordered_map<std::string, std::shared_ptr<T>> theCache;
      std::mutex mtx;
    };
    static Internal internal;
  public:
    static size_t count(const std::string &key) {
      std::unique_lock lock(internal.mtx);
      return internal.theCache.count(key);
    }

    static std::shared_ptr<T> at(const std::string &key) {
      std::unique_lock lock(internal.mtx);
      if (internal.theCache.count(key) == 0) return nullptr;
      return internal.theCache.at(key);
    }

    static void insert(const std::string &key, std::shared_ptr<T> value) {
      std::unique_lock lock(internal.mtx);
      internal.theCache[key] = value;
    }

    static void erase(const std::string &key) {
      std::unique_lock lock(internal.mtx);
      internal.theCache.erase(key);
    }
  };

  template<typename T> ObjectCache<T>::Internal ssrc::ObjectCache<T>::internal;
}
#endif //#ifndef OBJECTCACHE_HPP
