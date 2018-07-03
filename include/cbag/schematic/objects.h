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

    /** An instance in a schematic.
     */
    struct CSchInstance {
        /** Create an empty instance.
         */
        CSchInstance() = default;

        /** Create an instance with empty parameter and terminal mappings.
         *
         * @param name name of the instance.
         * @param lib the library name.
         * @param cell the cell name.
         * @param view the view name.
         * @param xform the instance location.
         */
        CSchInstance(NameUnit name, std::string lib, std::string cell, std::string view, Transform xform)
                : inst_name(std::move(name)), lib_name(std::move(lib)),
                  cell_name(std::move(cell)), view_name(std::move(view)),
                  xform(xform) {}

        /** Returns true if this instance is a vector instance.
         *
         *  @returns true if this instance is a vector instance.
         */
        inline bool is_vector() { return inst_name.is_vector(); }

        /** Returns the number of instances this schematic instance represents.
         *
         *  @returns number of instances in this schematic instance.
         */
        inline uint32_t size() { return (inst_name.size()); }

        std::string lib_name, cell_name, view_name;
        NameUnit inst_name;
        Transform xform;
        ParamMap params;
        std::list<Name> in_terms, out_terms, io_terms;
        std::map<NameUnit, NameUnit> term_map;
    };

    struct CSchMaster {
        CSchMaster() = default;

        std::list<Name> in_terms, out_terms, io_terms;
        std::list<CSchInstance> inst_list;
    };

    // YAML stream out functions.

    YAML::Emitter &operator<<(YAML::Emitter &out, const CSchInstance &v);

    YAML::Emitter &operator<<(YAML::Emitter &out, const CSchMaster &v);

}

#endif //CBAG_SCHEMATIC_OBJECTS_H
