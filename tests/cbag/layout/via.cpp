#include <catch2/catch.hpp>

#include <cbag/common/vector.h>
#include <cbag/enum/orient_2d.h>
#include <cbag/layout/tech_util.h>

using c_tech = cbag::layout::tech;
using c_vector = cbag::vector;

TEST_CASE("get_via_param() for 1 via", "[via]") {
    c_tech obj("tests/data/test_layout/tech_params.yaml");
    auto bot_layer = layer_t_at(obj, "M1", "drawing");
    auto top_layer = layer_t_at(obj, "M2", "drawing");
    auto bot_dir = cbag::orient_2d::HORIZONTAL;
    auto top_dir = cbag::orient_2d::VERTICAL;
    auto extend = true;

    auto param =
        obj.get_via_param(c_vector{32, 32}, bot_layer, top_layer, bot_dir, top_dir, extend);
    REQUIRE(param.num == std::array<cbag::cnt_t, 2>{1, 1});
    REQUIRE(param.cut_dim == c_vector{32, 32});
    REQUIRE(param.cut_spacing == c_vector{0, 0});
    REQUIRE(param.enc == std::array<c_vector, 2>{c_vector{40, 0}, c_vector{0, 40}});
    REQUIRE(param.off == std::array<c_vector, 2>{c_vector{0, 0}, c_vector{0, 0}});
}
