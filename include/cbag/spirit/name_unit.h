#ifndef CBAG_SPIRIT_NAME_UNIT_H
#define CBAG_SPIRIT_NAME_UNIT_H

#include <boost/spirit/home/x3.hpp>

#include <cbag/spirit/ast.h>

namespace x3 = boost::spirit::x3;

namespace cbag {
namespace spirit {
namespace parser {

struct name_unit_class;

using name_unit_type = x3::rule<name_unit_class, ast::name_unit, true>;

BOOST_SPIRIT_DECLARE(name_unit_type);
} // namespace parser

parser::name_unit_type const &name_unit();
} // namespace spirit
} // namespace cbag

#endif
