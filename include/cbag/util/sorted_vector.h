#ifndef CBAG_UTIL_SORTED_VECTOR_H
#define CBAG_UTIL_SORTED_VECTOR_H

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <utility>
#include <vector>

namespace cbag {
namespace util {

template <class T, class Compare = std::less<T>> class sorted_vector {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type &;
    using iterator = typename std::vector<T>::iterator;
    using reverse_iterator = typename std::vector<T>::reverse_iterator;
    using const_reference = const value_type &;
    using const_iterator = typename std::vector<T>::const_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;
    using difference_type = typename std::vector<T>::difference_type;

    template <typename V>
    using IsValue =
        std::enable_if_t<std::is_same_v<value_type, std::remove_cv_t<std::remove_reference_t<V>>>>;

  private:
    std::vector<value_type> data_;
    Compare comp_;

  public:
    sorted_vector() noexcept = default;

    template <class... Args> sorted_vector(Args &&... args) : data_(std::forward<Args>(args)...) {
        std::sort(data_.begin(), data_.end(), comp_);
    }

    sorted_vector(std::initializer_list<value_type> init) : data_(init) {
        std::sort(data_.begin(), data_.end(), comp_);
    }

    size_type size() const noexcept { return data_.size(); }
    size_type capacity() const noexcept { return data_.capacity(); }
    bool empty() const noexcept { return data_.empty(); }
    const_iterator begin() const noexcept { return data_.begin(); }
    const_iterator end() const noexcept { return data_.end(); }
    const_iterator cbegin() const noexcept { return data_.begin(); }
    const_iterator cend() const noexcept { return data_.end(); }
    iterator begin() noexcept { return data_.begin(); }
    iterator end() noexcept { return data_.end(); }
    const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }
    const_reverse_iterator rend() const noexcept { return data_.rend(); }
    const_reference at_front() const {
        if (data_.empty())
            throw std::out_of_range("Cannot get front of empty vector.");
        return data_.front();
    }
    const_reference at_back() const {
        if (data_.empty())
            throw std::out_of_range("Cannot get back of empty vector.");
        return data_.back();
    }
    const Compare &get_compare() const { return comp_; }

    template <class K> const_iterator lower_bound(const K &x) const {
        return std::lower_bound(data_.begin(), data_.end(), x, comp_);
    }

    template <class K> iterator lower_bound(const K &x) {
        return std::lower_bound(data_.begin(), data_.end(), x, comp_);
    }

    template <class K> const_iterator upper_bound(const K &x) const {
        return std::upper_bound(data_.begin(), data_.end(), x, comp_);
    }

    template <class K> std::pair<const_iterator, const_iterator> equal_range(const K &x) const {
        return std::equal_range(data_.begin(), data_.end(), x, comp_);
    }

    template <class K> std::pair<iterator, iterator> equal_range(const K &x) {
        return std::equal_range(data_.begin(), data_.end(), x, comp_);
    }

    template <class K> difference_type equal_size(const K &x) const {
        auto iter_range = equal_range(x);
        return iter_range.second - iter_range.first;
    }

    template <class K> const_iterator find(const K &x) const {
        auto iter = lower_bound(x);
        auto end = data_.end();
        return (iter == end || comp_(x, *iter)) ? end : iter;
    }

    template <class K> iterator find(const K &x) {
        auto iter = lower_bound(x);
        auto end = data_.end();
        return (iter == end || comp_(x, *iter)) ? end : iter;
    }

    template <class K> const_iterator find_exact(const K &x) const {
        auto iter = lower_bound(x);
        auto end = data_.end();
        return (iter != end && *iter == x) ? iter : end;
    }

    void clear() noexcept { data_.clear(); }
    void reserve(size_type n) { data_.reserve(n); }

    template <class... Args> std::pair<iterator, bool> emplace_unique(Args &&... args) {
        return insert_unique(value_type(std::forward<Args>(args)...));
    }

    template <class... Args> void emplace_back(Args &&... args) {
        push_back(value_type(std::forward<Args>(args)...));
    }

    void push_back(value_type item) {
        if (empty() || !comp_(item, data_.back()))
            data_.emplace_back(std::move(item));
        else
            throw std::invalid_argument("Cannot insert given element at back.");
    }

    template <class V, typename = IsValue<V>> std::pair<iterator, bool> insert_unique(V &&item) {
        auto iter = lower_bound(item);
        if (iter == data_.end() || comp_(item, *iter))
            return {data_.insert(iter, std::forward<V>(item)), true};
        return {iter, false};
    }

    iterator _insert_force(const_iterator iter, value_type &&item) {
        return data_.insert(iter, std::move(item));
    }

    iterator erase(const_iterator pos) { return data_.erase(std::move(pos)); }

    iterator erase(iterator first, iterator last) {
        return data_.erase(std::move(first), std::move(last));
    }

    iterator erase(const_iterator first, const_iterator last) {
        return data_.erase(std::move(first), std::move(last));
    }

    friend bool operator==(const sorted_vector &lhs, const sorted_vector &rhs) {
        return lhs.data_ == rhs.data_;
    }

    friend bool operator==(const sorted_vector &lhs, const std::vector<T> &rhs) {
        return lhs.data_ == rhs;
    }

    friend bool operator==(const std::vector<T> &lhs, const sorted_vector &rhs) {
        return lhs == rhs.data_;
    }

    iterator circ_shift(const iterator &start, const iterator &stop, bool up) {
        if (up) {
            auto last = stop - 1;
            value_type tmp = std::move(*last);
            std::move_backward(start, last, stop);
            *start = std::move(tmp);
            return start;
        } else {
            value_type tmp = std::move(*start);
            std::move(start + 1, stop, start);
            auto ans = stop - 1;
            *ans = std::move(tmp);
            return ans;
        }
    }
};

} // namespace util
} // namespace cbag

#endif
