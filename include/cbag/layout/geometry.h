#ifndef CBAG_LAYOUT_GEOMETRY_H
#define CBAG_LAYOUT_GEOMETRY_H

#include <cstdint>
#include <variant>
#include <vector>

#include <boost/geometry/index/rtree.hpp>

#include <cbag/layout/geo_object.h>
#include <cbag/layout/polygon45_fwd.h>
#include <cbag/layout/polygon45_set.h>
#include <cbag/layout/polygon90_fwd.h>
#include <cbag/layout/polygon90_set.h>
#include <cbag/layout/polygon_fwd.h>
#include <cbag/layout/polygon_set.h>
#include <cbag/layout/pt_vector_fwd.h>
#include <cbag/util/overload.h>

namespace bgi = boost::geometry::index;

namespace cbag {
namespace layout {

class rectangle;
class tech;
class transformation;

using geometry_index = bgi::rtree<geo_object, bgi::quadratic<32, 16>>;

/** A class representing layout geometries on the same layer.
 */
class geometry {
  private:
    using geometry_data = std::variant<polygon90_set, polygon45_set, polygon_set>;

    uint8_t mode = 0;
    geometry_data data;
    geometry_index index;
    std::string lay_type = "";
    tech *tech_ptr = nullptr;
    struct helper;

  public:
    geometry();

    geometry(std::string &&lay_type, tech *tech_ptr, uint8_t mode = 0);

    rectangle get_bbox() const;
    rectangle &get_bbox(rectangle &r) const;

    void reset_to_mode(uint8_t m);

    void add_shape(const rectangle &obj, bool is_horiz);
    void add_shape(const polygon90 &obj, bool is_horiz);
    void add_shape(const polygon45 &obj, bool is_horiz);
    void add_shape(const polygon45_set &obj, bool is_horiz);
    void add_shape(const polygon &obj, bool is_horiz);

    void record_instance(const geometry *master, transformation xform);

    template <typename T> void write_geometry(T &output) const {
        std::visit(
            overload{
                [&](const polygon90_set &d) { d.get(output); },
                [&](const polygon45_set &d) { d.get(output); },
                [&](const polygon_set &d) { d.get(output); },
            },
            data);
    }

    static polygon45_set make_path(const pt_vector &data, offset_t half_width, uint8_t style0,
                                   uint8_t style1, uint8_t stylem);

    static polygon45_set make_path45_bus(const pt_vector &data, const std::vector<offset_t> &widths,
                                         const std::vector<offset_t> &spaces, uint8_t style0,
                                         uint8_t style1, uint8_t stylem);
};

} // namespace layout
} // namespace cbag

#endif
