
#ifndef CBAG_NETLIST_CORE_H
#define CBAG_NETLIST_CORE_H

#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

#include <cbag/logging/logging.h>

#include <fmt/core.h>

#include <cbag/netlist/netlist_map_t.h>
#include <cbag/schematic/cellview.h>
#include <cbag/schematic/cellview_info.h>
#include <cbag/schematic/term_t.h>
#include <cbag/spirit/namespace_info.h>

namespace cbag {

// forward declaration
struct time_struct;
struct binary_t;

namespace netlist {

// netlister base class

namespace traits {

template <typename T> struct nstream {
    using type = T;

    static void close(type &stream) {}

    static void write_header(type &stream, const std::vector<std::string> &inc_list, bool shell) {}

    static void write_end(type &stream) {}

    static void write_cv_header(type &stream, const std::string &name,
                                const sch::cellview_info &info) {}

    static void write_cv_end(type &stream, const std::string &name) {}

    static void write_instance(type &stream, const std::string &name, const sch::instance &inst,
                               const sch::cellview_info &info) {}
};

} // namespace traits

template <typename Stream, typename traits::nstream<Stream>::type * = nullptr>
void add_cellview(Stream &stream, const std::string &name, const sch::cellview &cv,
                  const sch::cellview_info &info, const netlist_map_t &cell_map, bool shell) {
    traits::nstream<Stream>::write_cv_header(stream, name, info);
    if (!shell) {
        for (auto const &p : cv.instances) {
            const sch::instance &inst = *(p.second);

            // get instance master's information object
            auto libmap_iter = cell_map.find(inst.lib_name);
            if (libmap_iter == cell_map.end()) {
                throw std::invalid_argument(
                    fmt::format("Cannot find library {} in netlist map for cell {}.", inst.lib_name,
                                inst.cell_name));
            }
            auto cellmap_iter = libmap_iter->second.find(inst.cell_name);
            if (cellmap_iter == libmap_iter->second.end()) {
                throw std::invalid_argument(fmt::format("Cannot find cell {}__{} in netlist map.",
                                                        inst.lib_name, inst.cell_name));
            }

            // Only write instance if the name is not empty
            if (!cellmap_iter->second.cell_name.empty()) {
                traits::nstream<Stream>::write_instance(stream, p.first, inst,
                                                        cellmap_iter->second);
            }
        }
    }
    traits::nstream<Stream>::write_cv_end(stream, name);
}

struct line_format {
    size_t ncol;
    std::string cnt_str;
    bool break_before;
    int tab_size;
};

class lstream {
  private:
    std::vector<std::string> tokens;
    const line_format *fmt_info;

  public:
    class back_inserter {
      private:
        lstream *stream_ = nullptr;

      public:
        explicit back_inserter(lstream *stream);
        back_inserter &operator*();
        back_inserter &operator=(std::string name);
    };

    explicit lstream(const line_format *fmt_info);

    bool empty() const;

    back_inserter get_back_inserter();

    lstream &append_last(const char *seq);

    lstream &append_last(const std::string &seq);

    std::ofstream &append_to(std::ofstream &stream, bool newline = true) const;

    friend lstream &operator<<(lstream &builder, const std::string &token);

    friend lstream &operator<<(lstream &builder, std::string &&token);

    friend lstream &operator<<(lstream &builder, const std::vector<std::string> &tokens);

    friend std::ofstream &operator<<(std::ofstream &stream, const lstream &b);
};

class nstream_file {
  public:
    std::ofstream out_file;
    spirit::namespace_info ns;
    line_format line_fmt;

    explicit nstream_file(const std::string &fname, spirit::namespace_type ns_type,
                          line_format line_fmt);

    lstream make_lstream() const;

    void close();
};

template <class OutIter> class write_param_visitor {
  private:
    OutIter &iter_;
    const std::string &key_;

  public:
    write_param_visitor(OutIter &iter, const std::string &key) : iter_(iter), key_(key) {}

    void operator()(const std::string &v) const { *iter_ = fmt::format("{}={}", key_, v); }
    void operator()(const int32_t &v) const {
        auto logger = cbag::get_cbag_logger();
        logger->warn("integer parameter, do nothing.");
    }
    void operator()(const double &v) const {
        auto logger = cbag::get_cbag_logger();
        logger->warn("integer parameter, do nothing.");
    }
    void operator()(const bool &v) const {
        auto logger = cbag::get_cbag_logger();
        logger->warn("integer parameter, do nothing.");
    }
    void operator()(const time_struct &v) const {
        auto logger = cbag::get_cbag_logger();
        logger->warn("integer parameter, do nothing.");
    }
    void operator()(const binary_t &v) const {
        auto logger = cbag::get_cbag_logger();
        logger->warn("integer parameter, do nothing.");
    }
};

} // namespace netlist
} // namespace cbag

#endif // CBAG_NETLIST_NETLIST_H