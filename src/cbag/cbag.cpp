/** \file cbag.cpp
 *  \brief This implements the top level header file for cbag library.
 *
 *  \author Eric Chang
 *  \date   2018/07/18
 */

#include <fstream>

#include <yaml-cpp/yaml.h>
#include <easylogging++.h>

#include <cbag/spirit/parsers.h>
#include <cbag/spirit/name_unit.h>
#include <cbag/database/yaml_cellviews.h>

INITIALIZE_EASYLOGGINGPP // NOLINT


namespace cbag {
    void init_logging() {}

    void to_file(const SchCellView &cv, const char *fname) {
        std::ofstream outfile(fname, std::ios_base::out);
        YAML::Node node(cv);
        YAML::Emitter emitter;
        emitter << node;
        outfile << emitter.c_str() << std::endl;
        outfile.close();
    }

    spirit::ast::name_unit parse_cdba_name_unit(const std::string &source) {
        return parse<spirit::ast::name_unit,
                spirit::parser::name_unit_type>(source.c_str(), source.size(), spirit::name_unit());
    }
}