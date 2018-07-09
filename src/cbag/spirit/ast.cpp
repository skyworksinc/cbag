//
// Created by erichang on 7/5/18.
//

#include <sstream>
#include <limits>

#include <boost/functional/hash.hpp>

#include <cbag/spirit/ast.h>


namespace cbag {
    namespace spirit {
        namespace ast {
            uint32_t range::size() const {
                if (step == 0)
                    return 0;
                if (stop >= start) {
                    return (stop - start + step) / step;
                } else {
                    return (start - stop + step) / step;
                }
            }

            uint32_t range::get_stop_exclude() const {
                if (stop >= start) {
                    return start + size() * step;
                } else {
                    return start - size() * step;
                }
            }

            std::string range::to_string() const {
                if (step == 0) {
                    return "";
                }
                std::ostringstream out;

                out << "<" << start;
                if (start == stop) {
                    out << ">";
                } else {
                    out << ':' << stop;
                    if (step == 1) {
                        out << '>';
                    } else {
                        out << ':' << step << '>';
                    }
                }
                return out.str();
            }

            std::string name_bit::to_string() const {
                std::ostringstream out;
                out << base;
                if (index) {
                    out << '<' << (*index) << '>';
                }
                return out.str();
            }

            bool range::operator==(const range &other) const {
                return start == other.start && stop == other.stop && step == other.step;
            }

            bool range::operator<(const range &other) const {
                return start < other.start
                       || (start == other.start && (stop < other.stop
                                                    || (stop == other.stop && step < other.step)));
            }

            bool name_bit::operator==(const name_bit &other) const {
                return base == other.base && index == other.index;
            }

            bool name_bit::operator<(const name_bit &other) const {
                if (base < other.base) {
                    return true;
                } else if (base == other.base) {
                    if (index) {
                        return bool(other.index) && (*index < *other.index);
                    } else {
                        return bool(other.index);
                    }
                } else {
                    return false;
                }
            }

            std::string name_unit::to_string() const {
                std::ostringstream out;
                if (mult > 1) {
                    out << "<*" << mult << '>';
                }
                out << base;
                out << idx_range.to_string();

                return out.str();
            }

            name_bit name_unit::operator[](uint32_t index) const {
                uint32_t range_size = idx_range.size();
                return (range_size == 0) ? name_bit(base) : name_bit(base, idx_range[index % range_size]);
            }

            bool name_unit::operator==(const name_unit &other) const {
                return base == other.base && idx_range == other.idx_range && mult == other.mult;
            }

            bool name_unit::operator<(const name_unit &other) const {
                return base < other.base
                       || (base == other.base && (idx_range < other.idx_range
                                                  || (idx_range == other.idx_range && mult < other.mult)));
            }

            name::const_iterator &name::const_iterator::operator++() {
                if (bit_index < (ptr->unit_list[unit_index]).size() - 1) {
                    ++bit_index;
                } else {
                    ++unit_index;
                    bit_index = 0;
                }

                return *this;
            }

            bool name::operator==(const name &other) const {
                unsigned long size1 = unit_list.size();
                unsigned long size2 = other.unit_list.size();

                if (size1 == size2) {
                    for (unsigned long idx = 0; idx < size1; idx++) {
                        if (unit_list[idx] != other.unit_list[idx]) {
                            return false;
                        }
                    }
                    return true;
                } else {
                    return false;
                }
            }

            bool name::operator<(const name &other) const {
                unsigned long size1 = unit_list.size();
                unsigned long size2 = other.unit_list.size();

                if (size1 < size2) {
                    return true;
                } else if (size1 == size2) {
                    for (unsigned long idx = 0; idx < size1; idx++) {
                        if (unit_list[idx] < other.unit_list[idx]) {
                            return true;
                        }
                    }
                    return false;
                } else {
                    return false;
                }
            }

        }
    }
}


namespace std {
    // define hash function for NameUnit
    template<>
    struct hash<cbag::spirit::ast::name_bit> {
        size_t operator()(const cbag::spirit::ast::name_bit &v) const {

            size_t seed = 0;
            boost::hash_combine(seed, v.base);
            boost::hash_combine(seed, (v.index) ? *(v.index) :
                                      std::numeric_limits<std::size_t>::max());

            return seed;
        }
    };

}