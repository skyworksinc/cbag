#include <utility>

#include <cbag/common/box_t.h>
#include <cbag/layout/cellview.h>
#include <cbag/layout/instance.h>
#include <cbag/util/overload.h>

namespace cbag {
namespace layout {

cellview_ref::cellview_ref(std::string lib, std::string cell, std::string view)
    : lib(std::move(lib)), cell(std::move(cell)), view(std::move(view)) {}

instance::instance() = default;

instance::instance(std::string name, std::string lib, std::string cell, std::string view,
                   cbag::transformation xform, uint32_t nx, uint32_t ny, coord_t spx, coord_t spy)
    : master(std::in_place_type_t<cellview_ref>{}, std::move(lib), std::move(cell),
             std::move(view)),
      name(std::move(name)), xform(std::move(xform)), nx(nx), ny(ny), spx(spx), spy(spy) {}

instance::instance(std::string name, const cellview *master, cbag::transformation xform,
                   uint32_t nx, uint32_t ny, coord_t spx, coord_t spy)
    : master(std::in_place_type_t<const cellview *>{}, master), name(std::move(name)),
      xform(std::move(xform)), nx(nx), ny(ny), spx(spx), spy(spy) {}

bool instance::is_reference() const { return std::holds_alternative<cellview_ref>(master); }

const cellview *instance::get_cellview() const {
    auto ptr = std::get_if<const cellview *>(&master);
    return (ptr == nullptr) ? nullptr : *ptr;
}

const std::string &instance::get_lib_name(const std::string &output_lib) const {
    return std::visit(
        overload{
            [&output_lib](const cellview *v) -> const std::string & { return output_lib; },
            [](const cellview_ref &v) -> const std::string & { return v.lib; },
        },
        master);
}

const std::string &instance::get_cell_name(const str_map_t *rename_map) const {
    return std::visit(
        overload{
            [&rename_map](const cellview *v) -> const std::string & {
                if (rename_map == nullptr) {
                    return v->cell_name;
                }
                auto iter = rename_map->find(v->cell_name);
                if (iter == rename_map->end())
                    return v->cell_name;
                return iter->second;
            },
            [](const cellview_ref &v) -> const std::string & { return v.cell; },
        },
        master);
} // namespace layout

const std::string &instance::get_view_name(const std::string &default_view) const {
    return std::visit(
        overload{
            [&default_view](const cellview *v) -> const std::string & { return default_view; },
            [](const cellview_ref &v) -> const std::string & { return v.view; },
        },
        master);
}

const param_map *instance::get_params() const {
    return std::visit(
        overload{
            [](const cellview *v) { return (const param_map *)nullptr; },
            [](const cellview_ref &v) { return &(v.params); },
        },
        master);
}

box_t instance::get_bbox(const std::string &layer, const std::string &purpose) const {
    box_t r = std::visit(
        overload{
            [&layer, &purpose](const cellview *v) { return v->get_bbox(layer, purpose); },
            [](const cellview_ref &v) { return box_t(0, 0, 0, 0); },
        },
        master);

    return r.transform(xform);
}

void instance::set_master(const cellview *new_master) { master = new_master; }

void instance::set_param(const std::string &name,
                         const std::variant<int32_t, double, bool, std::string> &val) {
    auto *cv_ref = std::get_if<cellview_ref>(&master);
    if (cv_ref != nullptr) {
        cbag::set_param(cv_ref->params, name, val);
    }
}

} // namespace layout
} // namespace cbag
