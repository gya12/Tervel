#ifndef WF_HASHMAP_API_H_
#define WF_HASHMAP_API_H_

#include "tervel/containers/wf/hash-map/wf_hash_map.h"
#include "tervel/util/info.h"
#include "tervel/util/thread_context.h"
#include "tervel/util/tervel.h"
#include "tervel/util/memory/hp/hp_element.h"
#include "tervel/util/memory/hp/hp_list.h"

template<class Key, class Value>
class TestClass {
 public:
  typedef typename tervel::containers::wf::HashMap<Key, Value> Map;
  typedef typename tervel::containers::wf::HashMap<Key, Value>::ValueAccessor Accessor;

  TestClass(size_t num_threads, size_t capacity) {
    tervel_obj = new tervel::Tervel(num_threads);
    attach_thread();
    container = new Map(capacity);
  }

  char * name() {
    return "WF Hash Map";
  }

  void attach_thread() {
    tervel::ThreadContext* thread_context __attribute__((unused));
    thread_context = new tervel::ThreadContext(tervel_obj);
  }

  void detach_thread() {}

  bool find(Key key, Value &value) {
    typename tervel::containers::wf::HashMap<Key, Value>::ValueAccessor va;
    bool res = container->at(key, va);
    if (res) {
      value = (va.atomic_value())->load();
    }
    return res;
  }

  bool insert(Key key, Value value) {
    bool res = container->insert(key, value);
    return res;
  }

  bool update(Key key, Value &value_expected, Value value_new) {
    typename tervel::containers::wf::HashMap<Key, Value>::ValueAccessor va;
    if (container->at(key, va)) {
      return va.atomic_value()->compare_exchange_strong(value_expected, value_new);
    }
    return false;
  }

  bool remove(Key key) {
    bool res = container->remove(key);
    return res;
  }

  size_t size() {
    return container->size();
  }

 private:
  tervel::Tervel* tervel_obj;
  Map *container;
};

#endif  // WF_HASHMAP_API_H_
