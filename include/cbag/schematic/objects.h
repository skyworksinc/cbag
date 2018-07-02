//
// Created by erichang on 6/27/18.
//

#ifndef CBAG_SCHEMATIC_OBJECTS_H
#define CBAG_SCHEMATIC_OBJECTS_H

#include <string>
#include <tuple>
#include <utility>

#include <cbag/common.h>

namespace cbag {

    // Range data structure to represent bus terminal indices
    struct Range {
        Range() = default;

        Range(int32_t start, int32_t stop, int32_t step) : start(start), stop(stop), step(step) {}

        int32_t start, stop, step;
    };

    struct CSchTerm {
        CSchTerm() = default;

        explicit CSchTerm(std::string name) : name(std::move(name)), range_list() {}

        CSchTerm(std::string name, const std::list<uint32_t> &idx_list);

        std::string name;
        std::list<Range> range_list;
    };

    struct CSchInstance {
        CSchInstance() = default;

        CSchInstance(std::string name, std::string lib, std::string cell, std::string view,
                     Transform xform) : inst_name(std::move(name)), lib_name(std::move(lib)),
                                        cell_name(std::move(cell)), view_name(std::move(view)),
                                        xform(xform) {}

        std::string inst_name, lib_name, cell_name, view_name;
        Transform xform;
        ParamMap params;
        std::map<std::string, std::string> term_map;
    };

    struct CSchMaster {
        CSchMaster() = default;

        std::list<CSchTerm> in_terms, out_terms, io_terms;
        std::list<CSchInstance> inst_list;
    };

    // YAML stream out functions
    YAML::Emitter &operator<<(YAML::Emitter &out, const Range &v);

    YAML::Emitter &operator<<(YAML::Emitter &out, const CSchTerm &v);

    YAML::Emitter &operator<<(YAML::Emitter &out, const CSchInstance &v);

    YAML::Emitter &operator<<(YAML::Emitter &out, const CSchMaster &v);

}

#endif //CBAG_SCHEMATIC_OBJECTS_H