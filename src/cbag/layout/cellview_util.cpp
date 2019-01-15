#include <cbag/common/box_t_util.h>
#include <cbag/common/transformation_util.h>
#include <cbag/layout/cellview_util.h>
#include <cbag/layout/cv_obj_ref.h>
#include <cbag/layout/tech_util.h>
#include <cbag/layout/track_info_util.h>
#include <cbag/layout/via_wrapper.h>
#include <cbag/layout/wire_width.h>

namespace cbag {
namespace layout {

box_t get_bbox(const cellview &cv, const std::string &layer, const std::string &purpose) {
    auto ans = box_t::get_invalid_box();
    // merge geometry bounding box
    auto iter = cv.find_geometry(layer_t_at(*cv.get_tech(), layer, purpose));
    if (iter != cv.end_geometry()) {
        merge(ans, iter->second.get_bbox());
    }
    // merge instance bounding box
    for (auto it = cv.begin_inst(); it != cv.end_inst(); ++it) {
        merge(ans, it->second.get_bbox(layer, purpose));
    }
    return ans;
}

void add_pin(cellview &cv, const std::string &layer, const std::string &net,
             const std::string &label, const box_t &bbox) {
    auto lay_id = layer_id_at(*cv.get_tech(), layer);
    cv.add_pin(lay_id, std::string(net), std::string(label), box_t(bbox));
}

void add_pin_arr(cellview &cv, const std::string &net, const std::string &label, level_t level,
                 htr_t htr, offset_t lower, offset_t upper, cnt_t ntr, cnt_t n,
                 offset_t htr_pitch) {
    auto cur_htr = htr;
    auto &tinfo = cv.get_grid()->track_info_at(level);
    auto winfo = tinfo.get_wire_width(ntr);
    auto winfo_end = winfo.end();
    auto tr_dir = tinfo.get_direction();
    for (decltype(n) idx = 0; idx < n; ++idx, htr += htr_pitch) {
        auto [lay, purp] = get_layer_t(*cv.get_grid(), level, cur_htr);
        for (auto witer = winfo.begin(); witer != winfo_end; ++witer) {
            auto &[rel_htr, wire_w] = *witer;
            auto half_w = wire_w / 2;
            auto center = htr_to_coord(tinfo, cur_htr + rel_htr);
            cv.add_pin(lay, std::string(net), std::string(label),
                       box_t(tr_dir, lower, upper, center - half_w, center + half_w));
        }
    }
}

cv_obj_ref<via_wrapper> add_via(cellview &cv, transformation xform, std::string via_id,
                                bool add_layers, bool bot_horiz, bool top_horiz, cnt_t vnx,
                                cnt_t vny, offset_t w, offset_t h, offset_t vspx, offset_t vspy,
                                offset_t enc1l, offset_t enc1r, offset_t enc1t, offset_t enc1b,
                                offset_t enc2l, offset_t enc2r, offset_t enc2t, offset_t enc2b,
                                bool commit) {
    return {&cv,
            via_wrapper(via(std::move(xform), std::move(via_id),
                            via_param(vnx, vny, w, h, vspx, vspy, enc1l, enc1r, enc1t, enc1b, enc2l,
                                      enc2r, enc2t, enc2b)),
                        add_layers, bot_horiz, top_horiz),
            commit};
}

void add_via_arr(cellview &cv, const transformation &xform, const std::string &via_id,
                 bool add_layers, bool bot_horiz, bool top_horiz, cnt_t vnx, cnt_t vny, offset_t w,
                 offset_t h, offset_t vspx, offset_t vspy, offset_t enc1l, offset_t enc1r,
                 offset_t enc1t, offset_t enc1b, offset_t enc2l, offset_t enc2r, offset_t enc2t,
                 offset_t enc2b, cnt_t nx, cnt_t ny, offset_t spx, offset_t spy) {
    via_param param{vnx,   vny,   w,     h,     vspx,  vspy,  enc1l,
                    enc1r, enc1t, enc1b, enc2l, enc2r, enc2t, enc2b};

    offset_t dx = 0;
    for (decltype(nx) xidx = 0; xidx < nx; ++xidx, dx += spx) {
        offset_t dy = 0;
        for (decltype(ny) yidx = 0; yidx < ny; ++yidx, dy += spy) {
            cv.add_object(via_wrapper(via(get_move_by(xform, dx, dy), via_id, param), add_layers,
                                      bot_horiz, top_horiz));
        }
    }
}

void add_label(cellview &cv, const std::string &layer, const std::string &purpose,
               transformation xform, std::string label) {
    cv.add_label(layer_t_at(*cv.get_tech(), layer, purpose), std::move(xform), std::move(label));
}

cv_obj_ref<instance> add_prim_instance(cellview &cv, std::string lib, std::string cell,
                                       std::string view, std::string name,
                                       cbag::transformation xform, cnt_t nx, cnt_t ny, offset_t spx,
                                       offset_t spy, bool commit) {
    return {&cv,
            instance(std::move(name), std::move(lib), std::move(cell), std::move(view),
                     std::move(xform), nx, ny, spx, spy),
            commit};
}

cv_obj_ref<instance> add_instance(cellview &cv, const cellview *master, std::string name,
                                  cbag::transformation xform, cnt_t nx, cnt_t ny, offset_t spx,
                                  offset_t spy, bool commit) {
    return {&cv, instance(std::move(name), master, std::move(xform), nx, ny, spx, spy), commit};
}

} // namespace layout
} // namespace cbag
