#include <cbag/layout/polygon.h>

namespace cbag {
namespace layout {

polygon::polygon() : wdir(winding_dir::unknown_winding) {}

polygon::polygon(std::size_t n) : wdir(winding_dir::unknown_winding) { data.reserve(n); }

polygon::polygon(point_vector_t data, winding_dir wdir) : data(std::move(data)), wdir(wdir) {}

winding_dir polygon::winding() const {
    if (wdir == winding_dir::unknown_winding) {
        area_type pt_area = bp::point_sequence_area<iterator_type, area_type>(begin(), end());
        wdir =
            (pt_area < 0) ? winding_dir::counterclockwise_winding : winding_dir::clockwise_winding;
    }
    return wdir;
}

} // namespace layout
} // namespace cbag
