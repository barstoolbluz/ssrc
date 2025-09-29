#ifndef BGEXECUTOR_HPP
#define BGEXECUTOR_HPP

#include <queue>
#include <functional>

namespace shibatch {
  class Runnable {
    class BGExecutor* belongsTo = nullptr;
  public:
    virtual void run() = 0;
    virtual ~Runnable() {}

    friend class BGExecutor;
    friend class BGExecutorStatic;

    static class std::shared_ptr<Runnable> factory(std::function<void(void *)>, void *p=nullptr);
  };

  class BGExecutor {
    std::queue<std::shared_ptr<Runnable>> que;
  public:
    void push(std::shared_ptr<Runnable> job);
    std::shared_ptr<Runnable> pop();

    friend class BGExecutorStatic;
  };
}
#endif // #ifndef BGEXECUTOR_HPP
