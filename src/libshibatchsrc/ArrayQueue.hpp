#ifndef ARRAYQUEUE_HPP
#define ARRAYQUEUE_HPP

#include <deque>
#include <vector>
#include <cstring>

namespace shibatch {
  template<typename T>
  class ArrayQueue {
    std::deque<std::vector<T>> queue;
    size_t pos = 0;

  public:
    size_t size() {
      size_t ret = 0;
      for(auto v : queue) ret += v.size();
      return ret - pos;
    }

    void write(std::vector<T> &&v) {
      queue.push_back(std::move(v));
    }

    void write(T *ptr, size_t n) {
      std::vector<T> v(n);
      memcpy(v.data(), ptr, n * sizeof(T));
      write(std::move(v));
    }

    size_t read(T *ptr, size_t n) {
      size_t s = std::min(size(), n);

      for(size_t r = s;r > 0;) {
	size_t cs = std::min(queue.front().size() - pos, r);
	memcpy(ptr, queue.front().data() + pos, cs * sizeof(T));
	pos += cs;
	ptr += cs;
	r -= cs;
	if (pos >= queue.front().size()) {
	  queue.pop_front();
	  pos = 0;
	}
      }

      return s;
    }
  };
}
#endif //#ifndef ARRAYQUEUE_HPP
