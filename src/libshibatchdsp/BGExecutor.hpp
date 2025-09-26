#ifndef BGEXECUTOR_HPP
#define BGEXECUTOR_HPP

#include <functional>

#include "BlockingQueue.hpp"

namespace shibatch {
  class Runnable {
  public:
    class BGExecutor* belongsTo = nullptr;
    virtual void run() = 0;
    virtual ~Runnable() {}

    friend class BGExecutor;
    friend class BGExecutorStatic;

    static class std::shared_ptr<Runnable> factory(std::function<void(void *)>, void *p=nullptr);
  };

  class BGExecutor {
  public:
    BlockingQueue<std::shared_ptr<Runnable>> queue;
    BGExecutor();
    ~BGExecutor();
    void push(std::shared_ptr<Runnable> job);
    std::shared_ptr<Runnable> pop();
  };
}
#endif // #ifndef BGEXECUTOR_HPP
