#ifndef jb_explicit_cuts_binning_hpp
#define jb_explicit_cuts_binning_hpp

#include <jb/histogram_binning_linear_interpolation.hpp>

#include <algorithm>
#include <limits>
#include <vector>
#include <stdexcept>

namespace jb {

/**
 * A histogram binning_strategy for integer numbers with user defined cuts.
 *
 * This class defines histogram bins at cutting points explicity
 * defined by the user.  For example, cutting at [0, 1, 10, 100] would
 * create 4 buckets, from 0 to 1, from 1 to 10, from 10 to 100.
 * Samples below the minimum or maximum bucket are recorded as
 * underflows or overflows, respectively.
 *
 * Users can then create all kinds of interesting binning just by
 * providing the right initial values, for example:
 *    [0, 1, 2, ..., 9, 10, 20, ..., 90, 100, ... , 900, 1000]
 * offers a good tradeoff between accuracy and memory usage for tail
 * heavy distributions.
 *
 * Insertion takes O(log(n)) on the number of cuts.
 *
 * @tparam sample_type_t the type of samples, should be an integer type.
 */
template<typename sample_type_t>
class explicit_cuts_binning {
 public:
  /// type traits as required by @ref binning_strategy_concept
  typedef sample_type_t sample_type;

  /**
   * Constructor based on an initializer list.
   */
  explicit_cuts_binning(std::initializer_list<sample_type> const& il)
      : explicit_cuts_binning(il.begin(), il.end())
  {}

  /**
   * Constructor based on an iterator range.
   */
  template<typename iterator_t>
  explicit_cuts_binning(iterator_t begin, iterator_t end)
      : cuts_(begin, end) {
    if (cuts_.size() < 2) {
      throw std::invalid_argument(
          "explicit_cuts_binning requires at least 2 cuts");
    }
    if (not std::is_sorted(cuts_.begin(), cuts_.end())) {
      throw std::invalid_argument(
          "explicit_cuts_binning requires a sorted set of cuts");
    }
    if (cuts_.end() != std::adjacent_find(cuts_.begin(), cuts_.end())) {
      throw std::invalid_argument(
          "explicit_cuts_binning requires unique elements");
    }
  }

  //@{
  /**
   * @name Implement binning_strategy_concept interface.
   *
   * Please see @ref binning_strategy_concept for detailed
   * documentation of each member function.
   */
  sample_type histogram_min() const {
    return cuts_.front();
  }
  sample_type histogram_max() const {
    return cuts_.back();
  }
  sample_type theoretical_min() const {
    return std::numeric_limits<sample_type>::min();
  }
  sample_type theoretical_max() const {
    return std::numeric_limits<sample_type>::max();
  }
  std::size_t sample2bin(sample_type t) const {
    auto it = std::upper_bound(cuts_.begin(), cuts_.end(), t);
    return std::distance(cuts_.begin(), it) - 1;
  }
  sample_type bin2sample(std::size_t i) const {
    return cuts_[i];
  }
  sample_type interpolate(
      sample_type x_a, sample_type x_b, double y_a, double s, double q) const {
    return histogram_binning_linear_interpolation(x_a, x_b, y_a, s, q);
  }
  //@}

 private:
  std::vector<sample_type> cuts_;
};

} // namespace jb

#endif // jb_explicit_cuts_binning_hpp
