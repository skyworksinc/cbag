/** \file yaml_primitives.h
 *  \brief This file declares YAML serialization methods for primitive objects.
 *
 *  \author Eric Chang
 *  \date   2018/07/12
 */

#ifndef CBAG_YAML_BOX_T_H
#define CBAG_YAML_BOX_T_H

#include <yaml-cpp/yaml.h>

#include <cbag/common/dim_t.h>

namespace YAML {

template <> struct convert<cbag::dim_t> {
    static Node encode(const cbag::dim_t &rhs);

    static bool decode(const Node &node, cbag::dim_t &rhs);
};

} // namespace YAML

#endif
