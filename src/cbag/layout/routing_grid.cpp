#include <numeric>
#include <unordered_map>

#include <fmt/core.h>

#include <yaml-cpp/yaml.h>

#include <cbag/layout/routing_grid.h>
#include <cbag/yaml/common.h>

namespace cbag {
namespace layout {

void update_blk_pitch_helper(std::vector<track_info> &info_list, int start, int stop) {
    using p_type = std::array<offset_t, 2>;
    using p_vec = std::vector<p_type>;
    auto num_lay = stop - start;
    std::array<p_vec, 2> pitch_list = {p_vec{p_type{1, 1}}, p_vec{p_type{1, 1}}};
    pitch_list[0].reserve(num_lay);
    pitch_list[1].reserve(num_lay);

    for (auto idx = start; idx < stop; ++idx) {
        auto &info = info_list[idx];
        info.blk_pitch[0] = info.w + info.sp;
        info.blk_pitch[1] = info.blk_pitch[0] / 2;
        auto didx = static_cast<orient_2d_t>(info.dir);
        for (const auto &[bp, bp2] : pitch_list[didx]) {
            info.blk_pitch[0] = std::lcm(info.blk_pitch[0], bp);
            info.blk_pitch[1] = std::lcm(info.blk_pitch[1], bp2);
        }
        pitch_list[didx].push_back(info.blk_pitch);
    }
}

void update_block_pitch(std::vector<track_info> &info_list, int bot_layer, int top_private,
                        int top_ignore) {
    // set ignore layers block pitch
    for (decltype(bot_layer) idx = 0; idx <= top_ignore - bot_layer; ++idx) {
        info_list[idx].blk_pitch = std::array<offset_t, 2>{-1, -1};
    }

    // set private layers block pitch
    auto start = top_ignore + 1 - bot_layer;
    auto stop = top_private + 1 - bot_layer;
    if (stop > start)
        update_blk_pitch_helper(info_list, start, stop);

    // set public layers block pitch
    start = std::max(start, stop);
    stop = info_list.size();
    if (stop > start)
        update_blk_pitch_helper(info_list, start, stop);
}

routing_grid::routing_grid() = default;

routing_grid::routing_grid(const tech *t, const std::string &fname) : tech_ptr(t) {
    auto node = YAML::LoadFile(fname);

    auto tmp = cbagyaml::int_map_to_vec<track_info>(node["routing_grid"]);
    bot_layer = std::get<0>(tmp);
    top_ignore_layer = bot_layer - 1;
    top_private_layer = bot_layer - 1;
    info_list = std::move(std::get<1>(tmp));
    update_block_pitch(info_list, bot_layer, top_private_layer, top_ignore_layer);
}

bool routing_grid::operator==(const routing_grid &rhs) const noexcept {
    return tech_ptr == rhs.tech_ptr && bot_layer == rhs.bot_layer && info_list == rhs.info_list;
}

const tech *routing_grid::get_tech() const noexcept { return tech_ptr; }

orient_2d routing_grid::get_direction(int level) const {
    auto idx = static_cast<std::size_t>(level - bot_layer);
    if (idx >= info_list.size())
        throw std::out_of_range("Undefined routing grid level: " + std::to_string(level));
    return info_list[idx].dir;
}

} // namespace layout
} // namespace cbag
