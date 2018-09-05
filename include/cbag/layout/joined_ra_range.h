/** \file shapes.h
 *  \brief This file defines various shapes used by the database.
 *
 *  \author Eric Chang
 *  \date   2018/07/19
 */
#ifndef CBAG_LAYOUT_JOINED_RA_RANGE_H
#define CBAG_LAYOUT_JOINED_RA_RANGE_H

#include <iterator>

namespace cbag {
namespace layout {

/** A view of the concatenation of two containers.
 *
 * This class allows you to iterate over the concatenation of two containers.
 * It is different than boost::range::join because this class acts as a view,
 * meaning that it reflects any changes to the underlying containers.
 *
 * The requirements of the uderlying contains are:
 * 1. Define value_type and const_iterator tags.
 * 2. the const_iterator must be random access iterator.
 * 3. the value_type of the second container must derived from the
 *    value_type of the first container.
 * 4. the first container cannot be null (the second container can)
 *
 * The iterator returns value_type of the first container.
 */
template <typename ltype, typename rtype,
          typename std::enable_if_t<
              std::is_same_v<
                  typename std::iterator_traits<typename ltype::const_iterator>::iterator_category,
                  std::random_access_iterator_tag> &&
              std::is_same_v<
                  typename std::iterator_traits<typename rtype::const_iterator>::iterator_category,
                  std::random_access_iterator_tag> &&
              std::is_base_of<typename ltype::value_type, typename rtype::value_type>::value> * =
              nullptr>
class joined_ra_range {
  private:
    class joined_ra_iterator {
      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = typename ltype::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

      private:
        std::size_t idx = 0;
        const std::size_t lsize = 0;
        const typename ltype::const_iterator lstart;
        const typename rtype::const_iterator rstart;

      public:
        joined_ra_iterator() {}

        joined_ra_iterator(typename ltype::const_iterator lstart,
                           typename rtype::const_iterator rstart, std::size_t idx,
                           std::size_t lsize)
            : idx(idx), lsize(lsize), lstart(std::move(lstart)), rstart(std::move(rstart)) {}

        joined_ra_iterator &operator+=(difference_type rhs) {
            idx += rhs;
            return *this;
        }
        joined_ra_iterator &operator-=(difference_type rhs) {
            idx -= rhs;
            return *this;
        }
        reference operator*() const {
            return (idx < lsize) ? lstart[idx] : rstart[idx - lsize];
        }
        pointer operator->() const {
            return (idx < lsize) ? lstart + idx : rstart + (idx - lsize);
        }
        reference operator[](difference_type rhs) const {
            std::size_t tmp = idx + rhs;
            return (tmp < lsize) ? lstart[tmp] : rstart[tmp - lsize];
        }
        joined_ra_iterator &operator++() {
            ++idx;
            return *this;
        }
        joined_ra_iterator &operator--() {
            --idx;
            return *this;
        }
        joined_ra_iterator operator++(int) {
            joined_ra_iterator tmp(lstart, rstart, idx, lsize);
            ++idx;
            return tmp;
        }
        joined_ra_iterator operator--(int) {
            joined_ra_iterator tmp(lstart, rstart, idx, lsize);
            --idx;
            return tmp;
        }
        difference_type operator-(const joined_ra_iterator &rhs) const { return idx - rhs.idx; }
        joined_ra_iterator operator+(difference_type rhs) const {
            return {lstart, rstart, idx + rhs, lsize};
        }
        joined_ra_iterator operator-(difference_type rhs) const {
            return {lstart, rstart, idx - rhs, lsize};
        }
        friend joined_ra_iterator operator+(difference_type lhs, const joined_ra_iterator &rhs) {
            return {rhs.lstart, rhs.rstart, rhs.idx + lhs, rhs.lsize};
        }
        bool operator==(const joined_ra_iterator &rhs) const {
            return idx == rhs.idx && lsize == rhs.lsize && lstart == rhs.lstart &&
                   rstart == rhs.rstart;
        }
        bool operator!=(const joined_ra_iterator &rhs) const { return !((*this) == rhs); }
        bool operator>(const joined_ra_iterator &rhs) const { return idx > rhs.idx; }
        bool operator<(const joined_ra_iterator &rhs) const { return idx < rhs.idx; }
        bool operator>=(const joined_ra_iterator &rhs) const { return !((*this) < rhs.idx); }
        bool operator<=(const joined_ra_iterator &rhs) const { return !((*this) > rhs.idx); }
    };

  public:
    using iterator = joined_ra_iterator;
    using const_iterator = iterator;
    using value_type = typename joined_ra_iterator::value_type;

  private:
    const ltype *lval;
    const rtype *rval;

  public:
    joined_ra_range(const ltype &lval, const rtype &rval) : lval(&lval), rval(&rval) {}

    const_iterator begin() const {
        return const_iterator(lval->begin(), rval->begin(), 0, lval->size());
    }
    const_iterator end() const {
        std::size_t tmp = lval->size();
        return const_iterator(lval->begin(), rval->begin(), tmp + rval->size(), tmp);
    }
    std::size_t size() const { return lval->size() + rval->size(); }
};

} // namespace layout
} // namespace cbag

#endif