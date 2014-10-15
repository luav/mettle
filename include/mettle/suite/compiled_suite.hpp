#ifndef INC_METTLE_COMPILED_SUITE_HPP
#define INC_METTLE_COMPILED_SUITE_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "attributes.hpp"

namespace mettle {

namespace detail {
  inline uint64_t generate_id() {
    static std::atomic<uint64_t> id_(0);
    return id_++;
  }

  template<typename Container, typename Element>
  inline auto move_if(Element &&value) {
    using Value = typename std::remove_reference<Element>::type;
    using ReturnType = typename std::conditional<
      std::is_lvalue_reference<Container>::value, Value &, Value &&
    >::type;
    return static_cast<ReturnType>(value);
  }
}

struct test_result {
  bool passed;
  std::string message;
};

template<typename Ret, typename ...T>
class compiled_suite {
  template<typename Ret2, typename ...T2>
  friend class compiled_suite;
public:
  struct test_info {
    using function_type = std::function<Ret(T&...)>;

    test_info(std::string name, function_type function, attributes attrs)
      : name(std::move(name)), function(std::move(function)),
        attrs(std::move(attrs)), id(detail::generate_id()) {}

    std::string name;
    function_type function;
    attributes attrs;
    uint64_t id;
  };

  using iterator = typename std::vector<test_info>::const_iterator;

  template<typename String, typename Tests, typename Subsuites, typename Func>
  compiled_suite(
    String &&name, Tests &&tests, Subsuites &&subsuites,
    const attributes &attrs, const Func &f
  ) : name_(std::forward<String>(name)) {
    for(auto &&test : tests) {
      tests_.emplace_back(
        detail::move_if<Tests>(test.name),
        f(detail::move_if<Tests>(test.function)),
        unite(detail::move_if<Tests>(test.attrs), attrs)
      );
    }
    for(auto &&ss : subsuites)
      subsuites_.emplace_back(detail::move_if<Subsuites>(ss), attrs, f);
  }

  template<typename Ret2, typename ...T2, typename Func>
  compiled_suite(const compiled_suite<Ret2, T2...> &suite,
                 const attributes &attrs, const Func &f)
    : compiled_suite(suite.name_, suite.tests_, suite.subsuites_, attrs, f) {}

  template<typename Ret2, typename ...T2, typename Func>
  compiled_suite(compiled_suite<Ret2, T2...> &&suite,
                 const attributes &attrs, const Func &f)
    : compiled_suite(std::move(suite.name_), std::move(suite.tests_),
                     std::move(suite.subsuites_), attrs, f) {}

  const std::string & name() const {
    return name_;
  }

  iterator begin() const {
    return tests_.begin();
  }

  iterator end() const {
    return tests_.end();
  }

  size_t size() const {
    return tests_.size();
  }

  const std::vector<compiled_suite> & subsuites() const {
    return subsuites_;
  }
private:
  std::string name_;
  std::vector<test_info> tests_;
  std::vector<compiled_suite> subsuites_;
};

using runnable_suite = compiled_suite<test_result>;
using test_function = std::function<test_result(void)>;
using suites_list = std::vector<runnable_suite>;

} // namespace mettle

#endif
