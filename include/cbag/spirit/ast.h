/** \file ast.h
 *  \brief This file defines various abstract syntax tree (AST) structures
 * parsed by Spirit X3.
 *
 *  \author Eric Chang
 *  \date   2018/07/10
 */

#ifndef CBAG_SPIRIT_AST_H
#define CBAG_SPIRIT_AST_H

#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

namespace x3 = boost::spirit::x3;

namespace cbag {
namespace spirit {
namespace ast {

struct namespace_info {
    char bus_begin = '<';
    char bus_end = '>';
    char bus_delim = ':';
    char list_delim = ',';
    char rep_grp_begin = '(';
    char rep_grp_end = ')';
    std::string rep_begin = "<*";
    std::string rep_end = ">";

    bool operator==(const namespace_info &rhs) const {
        return (bus_begin == rhs.bus_begin && bus_end == rhs.bus_end &&
                bus_delim == rhs.bus_delim && list_delim == rhs.list_delim &&
                rep_grp_begin == rhs.rep_grp_begin && rep_grp_end == rhs.rep_grp_end &&
                rep_begin == rhs.rep_begin && rep_end == rhs.rep_end);
    }
};

/** Represents a range of indices at regular interval.
 *
 *  step size of 0 means that this range is empty; it doesn't contain any item.
 *  the step field is always non-negative.  However, if stop < start, then it is
 *  implied that the index decreases.
 *
 *  the stop field is inclusive.  However, get_stop_exclude() method will return
 *  an exclusive stop value if needed.
 */
struct range : x3::position_tagged {
  private:
    mutable std::optional<uint32_t> size_;

  public:
    uint32_t start = 0;
    uint32_t stop = 0;
    uint32_t step = 0;

    class const_iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = const value_type &;

      private:
        uint32_t val_ = 0;
        uint32_t step_ = 0;
        bool up_ = false;

      public:
        const_iterator();

        const_iterator(uint32_t val, uint32_t step, bool up);

        const_iterator &operator++();
        reference operator*() const;
        bool operator==(const const_iterator &rhs) const;
        bool operator!=(const const_iterator &rhs) const;
    };

    range();

    range(uint32_t start, uint32_t stop, uint32_t step);

    uint32_t size() const;

    uint32_t get_stop_exclude() const;

    const_iterator begin() const;

    const_iterator end() const;

    uint32_t operator[](uint32_t index) const;

    uint32_t at(uint32_t index) const;
};

/** Represents a unit name; either a scalar or vector name.
 *
 *  POssible formats are "foo", "bar[2]", "baz[3:1]"
 */
struct name_unit : x3::position_tagged {
    std::string base;
    range idx_range;

    class const_iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

      private:
        const namespace_info *info_ = nullptr;
        const name_unit *parent_ = nullptr;
        range::const_iterator iter_;

      public:
        const_iterator();

        const_iterator(const namespace_info *info, const name_unit *parent,
                       range::const_iterator iter);

        const_iterator &operator++();
        value_type operator*() const;
        bool operator==(const const_iterator &rhs) const;
        bool operator!=(const const_iterator &rhs) const;
    };

    name_unit();

    uint32_t size() const;

    bool is_vector() const;

    const_iterator begin(const namespace_info *info) const;

    const_iterator end(const namespace_info *info) const;
};

struct name_rep;

class const_name_rep_iterator;

using rep_vec_iter = std::vector<name_rep>::const_iterator;

/** Represents a list of name_rep's.
 */
struct name : x3::position_tagged {
  private:
    mutable std::optional<uint32_t> size_;

  public:
    std::vector<name_rep> rep_list;

    class const_iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

      private:
        const namespace_info *info_ = nullptr;
        rep_vec_iter vbegin_;
        rep_vec_iter vend_;
        std::unique_ptr<const_name_rep_iterator> rbegin_ = nullptr;
        std::unique_ptr<const_name_rep_iterator> rend_ = nullptr;

      public:
        const_iterator();

        const_iterator(const namespace_info *info, rep_vec_iter vbegin, rep_vec_iter vend,
                       std::unique_ptr<const_name_rep_iterator> &&rbegin,
                       std::unique_ptr<const_name_rep_iterator> &&rend);
        const_iterator(const const_iterator &rhs);
        const_iterator(const_iterator &&rhs);

        const_iterator &operator=(const const_iterator &rhs);
        const_iterator &operator=(const_iterator &&rhs);

        const_iterator &operator++();
        value_type operator*() const;
        bool operator==(const const_iterator &rhs) const;
        bool operator!=(const const_iterator &rhs) const;
    };

    name();

    const_iterator begin(const namespace_info *info) const;

    const_iterator end(const namespace_info *info) const;

    uint32_t size() const;
};

using nu_iter_tuple =
    std::tuple<name_unit::const_iterator, name_unit::const_iterator, name_unit::const_iterator>;

using na_iter_tuple = std::tuple<name::const_iterator, name::const_iterator, name::const_iterator>;

class const_name_rep_iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::string;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

  private:
    const namespace_info *info_ = nullptr;
    uint32_t cnt_ = 0;
    std::variant<nu_iter_tuple, na_iter_tuple> iter_;

  public:
    const_name_rep_iterator();

    const_name_rep_iterator(const namespace_info *info, uint32_t cnt, nu_iter_tuple iter);

    const_name_rep_iterator(const namespace_info *info, uint32_t cnt, na_iter_tuple iter);

    const_name_rep_iterator &operator++();
    value_type operator*() const;
    bool operator==(const const_name_rep_iterator &rhs) const;
    bool operator!=(const const_name_rep_iterator &rhs) const;
};

struct name_rep_value : x3::variant<name_unit, name> {
    using base_type::base_type;
    using base_type::operator=;
};

/** Represents a repeated name
 *
 *  Possible formats are "<*3>foo", "<*3>(a,b)", or just name_unit.
 */
struct name_rep : x3::position_tagged {
    using const_iterator = const_name_rep_iterator;

    uint32_t mult = 1;
    name_rep_value data;

    name_rep();

    uint32_t size() const;

    bool is_vector() const;

    const_iterator begin(const namespace_info *info) const;

    const_iterator end(const namespace_info *info) const;
};

} // namespace ast
} // namespace spirit
} // namespace cbag

#endif // CBAG_SPIRIT_AST_H
