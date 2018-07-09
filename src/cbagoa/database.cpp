#include <iostream>

#include <fmt/format.h>
// include fmt/ostream.h to support formatting oaStrings, which defines operator <<
#include <fmt/ostream.h>

#include <cbag/spirit/parsers.h>
#include <cbag/spirit/name.h>
#include <cbag/spirit/name_unit.h>

#include <cbagoa/database.h>


namespace bsp = cbag::spirit;
namespace bsa = bsp::ast;

namespace cbagoa {
    bsa::name parse_name(const std::string &source) {
        return cbag::parse<bsa::name, bsp::parser::name_type>(source, bsp::name());
    }

    bsa::name_unit parse_name_unit(const std::string &source) {
        return cbag::parse<bsa::name_unit,
                bsp::parser::name_unit_type>(source, bsp::name_unit());
    }

    cbag::Orientation convert_orient(const oa::oaOrient &orient) {
        switch (orient) {
            case oa::oacR0:
                return cbag::Orientation::R0;
            case oa::oacR90:
                return cbag::Orientation::R90;
            case oa::oacR180:
                return cbag::Orientation::R180;
            case oa::oacR270:
                return cbag::Orientation::R270;
            case oa::oacMY:
                return cbag::Orientation::MY;
            case oa::oacMYR90:
                return cbag::Orientation::MYR90;
            case oa::oacMX:
                return cbag::Orientation::MX;
            case oa::oacMXR90:
                return cbag::Orientation::MXR90;
            default:
                throw std::invalid_argument("Unknown orientation code.");
        }
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

    oa::oaBoolean LibDefObserver::onLoadWarnings(oa::oaLibDefList *obj, const oa::oaString &msg,
                                                 oa::oaLibDefListWarningTypeEnum type) {
        throw std::runtime_error("OA Error: " + std::string(msg));
    }

#pragma clang diagnostic pop

    Library::~Library() {
        close();
    }

    void Library::open_lib(const std::string &lib_file, const std::string &library,
                           const std::string &lib_path, const std::string &tech_lib) {
        try {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "NotAssignable"
            oaDesignInit(oacAPIMajorRevNumber, oacAPIMinorRevNumber, // NOLINT
                         oacDataModelRevNumber); // NOLINT
#pragma clang diagnostic pop

            // open library definition
            oa::oaString lib_def_file(lib_file.c_str());
            oa::oaLibDefList::openLibs(lib_def_file);

            // open library
            lib_name = library;
            lib_name_oa = oa::oaScalarName(ns, library.c_str());
            lib_ptr = oa::oaLib::find(lib_name_oa);
            if (lib_ptr == nullptr) {
                // create new library
                oa::oaString oa_lib_path(lib_path.c_str());
                oa::oaScalarName oa_tech_lib(ns, tech_lib.c_str());
                lib_ptr = oa::oaLib::create(lib_name_oa, oa_lib_path);
                oa::oaTech::attach(lib_ptr, oa_tech_lib);

                // NOTE: I cannot get open access to modify the library file, so
                // we just do it by hand.
                std::ofstream outfile;
                outfile.open(lib_file, std::ios_base::app);
                outfile << "DEFINE " << library << " " << lib_path << std::endl;
                outfile.close();
            } else if (!lib_ptr->isValid()) {
                throw std::invalid_argument("Invalid library: " + library);
            }

            // open technology file
            tech_ptr = oa::oaTech::find(lib_ptr);
            if (tech_ptr == nullptr) {
                // opened tech not found, attempt to open
                if (!oa::oaTech::exists(lib_ptr)) {
                    throw std::runtime_error("Cannot find technology for library: " + library);
                } else {
                    tech_ptr = oa::oaTech::open(lib_ptr, 'r');
                    if (tech_ptr == nullptr) {
                        throw std::runtime_error("Cannot open technology for library: " + library);
                    }
                }
            }

            // get database unit
            dbu_per_uu = tech_ptr->getDBUPerUU(oa::oaViewType::get(oa::oacMaskLayout));

            is_open = true;
        } catch (oa::oaCompatibilityError &ex) {
            std::string msg_std(ex.getMsg());
            throw std::runtime_error("OA Compatibility Error: " + std::string(ex.getMsg()));
        } catch (oa::oaDMError &ex) {
            throw std::runtime_error("OA DM Error: " + std::string(ex.getMsg()));
        } catch (oa::oaError &ex) {
            throw std::runtime_error("OA Error: " + std::string(ex.getMsg()));
        } catch (oa::oaDesignError &ex) {
            throw std::runtime_error("OA Design Error: " + std::string(ex.getMsg()));
        }
    }

    std::pair<oa::oaDesign *, oa::oaBlock *>
    Library::open_design(const std::string &cell_name, const std::string &view_name,
                         oa::oaReservedViewTypeEnum view_type) {
        oa::oaScalarName cell_oa(ns, cell_name.c_str());
        oa::oaScalarName view_oa(ns, view_name.c_str());

        oa::oaDesign *dsn_ptr = oa::oaDesign::open(lib_name_oa, cell_oa, view_oa,
                                                   oa::oaViewType::get(view_type), 'r');
        if (dsn_ptr == nullptr) {
            throw std::invalid_argument(fmt::format("Cannot open cell: {}__{}({})",
                                                    lib_name, cell_name, view_name));
        }

        oa::oaBlock *blk_ptr = dsn_ptr->getTopBlock();
        if (blk_ptr == nullptr) {
            throw std::invalid_argument(fmt::format("Cannot open top block for cell: {}__{}({})",
                                                    lib_name, cell_name, view_name));
        }
        return {dsn_ptr, blk_ptr};
    }

    cbag::CSchMaster Library::parse_schematic(const std::string &cell_name,
                                              const std::string &view_name) {
        // get OA design and block pointers
        auto dsn_ptr_pair = open_design(cell_name, view_name, oa::oacSchematic);
        oa::oaDesign *dsn_ptr = dsn_ptr_pair.first;
        oa::oaBlock *blk_ptr = dsn_ptr_pair.second;

        // place holder classes
        oa::oaString tmp_str;
        // create schematic master
        cbag::CSchMaster ans;

        // get pins
        oa::oaIter<oa::oaPin> pin_iter(blk_ptr->getPins());
        oa::oaPin *pin_ptr;
        while ((pin_ptr = pin_iter.getNext()) != nullptr) {
            oa::oaTerm *term_ptr = pin_ptr->getTerm();
            oa::oaString tmp_;
            term_ptr->getName(ns_cdba, tmp_str);
            bool success;
            switch (term_ptr->getTermType()) {
                case oa::oacInputTermType :
                    success = ans.in_pins.insert(parse_name(std::string(tmp_str))).second;
                    break;
                case oa::oacOutputTermType :
                    success = ans.out_pins.insert(parse_name(std::string(tmp_str))).second;
                    break;
                case oa::oacInputOutputTermType :
                    success = ans.io_pins.insert(parse_name(std::string(tmp_str))).second;
                    break;
                default:
                    term_ptr->getName(ns_cdba, tmp_str);
                    throw std::invalid_argument(fmt::format("Pin {} has invalid terminal type: {}",
                                                            tmp_str, term_ptr->getTermType().getName()));
            }
            if (!success) {
                throw std::invalid_argument(fmt::format("Cannot add pin {}; it already exists.", tmp_str));
            }
        }

        // get instances
        oa::oaIter<oa::oaInst> inst_iter(blk_ptr->getInsts());
        oa::oaInst *inst_ptr;
        while ((inst_ptr = inst_iter.getNext()) != nullptr) {
            oa::oaString inst_lib_oa, inst_cell_oa, inst_name_oa;
            inst_ptr->getLibName(ns_cdba, inst_lib_oa);
            inst_ptr->getCellName(ns_cdba, inst_cell_oa);
            // NOTE: exclude pin instances
            if (inst_lib_oa != "basic" ||
                (inst_cell_oa != "ipin" && inst_cell_oa != "opin" && inst_cell_oa != "iopin")) {
                // get view
                oa::oaString inst_view_oa;
                inst_ptr->getViewName(ns_cdba, inst_view_oa);

                // get instance name
                inst_ptr->getName(ns_cdba, inst_name_oa);

                // get transform
                oa::oaTransform xform;
                inst_ptr->getTransform(xform);

                // create schematic instance
                bsa::name_unit inst_name = parse_name_unit(std::string(inst_name_oa));
                if (inst_name.mult > 1) {
                    throw std::invalid_argument(fmt::format("Invalid instance name: {}", inst_name_oa));
                }
                uint32_t inst_size = inst_name.size();

                auto inst_ret_val = ans.inst_map.emplace(std::move(inst_name),
                                                         cbag::CSchInstance(std::string(inst_lib_oa),
                                                                            std::string(inst_cell_oa),
                                                                            std::string(inst_view_oa),
                                                                            cbag::Transform(xform.xOffset(),
                                                                                            xform.yOffset(),
                                                                                            convert_orient(
                                                                                                    xform.orient()))));
                if (!inst_ret_val.second) {
                    throw std::invalid_argument(fmt::format("Instance {} already exists.", inst_name_oa));
                }

                // get instance pointer
                cbag::CSchInstance *sinst_ptr = &inst_ret_val.first->second;

                // get parameters
                if (inst_ptr->hasProp()) {
                    oa::oaIter<oa::oaProp> prop_iter(inst_ptr->getProps());
                    oa::oaProp *prop_ptr;
                    while ((prop_ptr = prop_iter.getNext()) != nullptr) {
                        add_param(sinst_ptr->params, prop_ptr);
                    }
                }

                // get instance connections
                oa::oaIter<oa::oaInstTerm> iterm_iter(inst_ptr->getInstTerms(oacInstTermIterAll));
                oa::oaInstTerm *iterm_ptr;
                oa::oaString term_name_oa, net_name_oa;
                while ((iterm_ptr = iterm_iter.getNext()) != nullptr) {
                    // get terminal and net names
                    iterm_ptr->getTerm()->getName(ns_cdba, term_name_oa);
                    iterm_ptr->getNet()->getName(ns_cdba, net_name_oa);
                    bsa::name term_name = parse_name(std::string(term_name_oa));
                    bsa::name net_name = parse_name(std::string(net_name_oa));

                    // populate connection map
                    auto tname_iter = term_name.begin();
                    auto tname_end = term_name.end();
                    auto nname_iter = net_name.begin();
                    auto nname_end = net_name.end();
                    if (inst_size == 1) {
                        // handle case where we have a scalar instance
                        for (; tname_iter != tname_end; ++tname_iter, ++nname_iter) {
                            if (nname_iter == nname_end) {
                                throw std::invalid_argument(
                                        fmt::format("Instance {} terminal {} net {} length mismatch.",
                                                    inst_name_oa, term_name_oa, net_name_oa));
                            }
                            bsa::name_bit term_name_bit = *tname_iter;

                            auto conn_ret = sinst_ptr->connections.emplace(*tname_iter, inst_size);
                            if (conn_ret.second) {
                                (conn_ret.first->second)[0] = *nname_iter;
                            } else {
                                throw std::invalid_argument(fmt::format("Instance {} has duplicate pin {}",
                                                                        inst_name_oa, term_name_bit.to_string()));
                            }
                        }
                        if (nname_iter != nname_end) {
                            throw std::invalid_argument(
                                    fmt::format("Instance {} terminal {} net {} length mismatch.",
                                                inst_name_oa, term_name_oa, net_name_oa));
                        }
                    } else {
                        // handle case where we have a vector instance
                        std::vector<std::map<bsa::name_bit, std::vector<bsa::name_bit> >::iterator> ptr_list;
                        for (; tname_iter != tname_end; ++tname_iter, ++nname_iter) {
                            if (nname_iter == nname_end) {
                                throw std::invalid_argument(
                                        fmt::format("Instance {} terminal {} net {} length mismatch.",
                                                    inst_name_oa, term_name_oa, net_name_oa));
                            }
                            bsa::name_bit term_name_bit = *tname_iter;

                            auto conn_ret = sinst_ptr->connections.emplace(*tname_iter, inst_size);
                            if (conn_ret.second) {
                                (conn_ret.first->second)[0] = *nname_iter;
                                ptr_list.push_back(conn_ret.first);
                            } else {
                                throw std::invalid_argument(fmt::format("Instance {} has duplicate pin {}",
                                                                        inst_name_oa, term_name_bit.to_string()));
                            }
                        }
                        for (uint32_t idx = 1; idx < inst_size; ++idx) {
                            for (auto ptr : ptr_list) {
                                if (nname_iter == nname_end) {
                                    throw std::invalid_argument(
                                            fmt::format("Instance {} terminal {} net {} length mismatch.",
                                                        inst_name_oa, term_name_oa, net_name_oa));
                                }
                                (ptr->second)[idx] = *nname_iter;
                                ++nname_iter;
                            }
                        }
                    }
                }
            }
        }

        // close design and return master
        dsn_ptr->close();
        return ans;
    }

    void Library::parse_symbol(const std::string &cell_name, const std::string &view_name) {
        // get OA design and block pointers
        auto dsn_ptr_pair = open_design(cell_name, view_name, oa::oacSchematicSymbol);
        oa::oaDesign *dsn_ptr = dsn_ptr_pair.first;
        oa::oaBlock *blk_ptr = dsn_ptr_pair.second;

        // place holder classes
        oa::oaString tmp_str;

        // get pins
        oa::oaIter<oa::oaPin> pin_iter(blk_ptr->getPins());
        oa::oaPin *pin_ptr;
        while ((pin_ptr = pin_iter.getNext()) != nullptr) {
            oa::oaTerm *term_ptr = pin_ptr->getTerm();
            oa::oaString tmp_;
            term_ptr->getName(ns_cdba, tmp_str);

            std::cout << "Pin " << tmp_str << ':' << std::endl;

            switch (term_ptr->getTermType()) {
                case oa::oacInputTermType :
                    std::cout << "  type: input" << std::endl;
                    break;
                case oa::oacOutputTermType :
                    std::cout << "  type: output" << std::endl;
                    break;
                case oa::oacInputOutputTermType :
                    std::cout << "  type: inout" << std::endl;
                    break;
                default:
                    term_ptr->getName(ns_cdba, tmp_str);
                    throw std::invalid_argument(fmt::format("Pin {} has invalid terminal type: {}",
                                                            tmp_str, term_ptr->getTermType().getName()));
            }

            std::cout << "  figures:" << std::endl;
            oa::oaIter<oa::oaPinFig> fig_iter(pin_ptr->getFigs());
            oa::oaPinFig *fig_ptr;
            while ((fig_ptr = fig_iter.getNext()) != nullptr) {
                oa::oaBox bbox;
                fig_ptr->getBBox(bbox);
                std::cout << "    type: " << fig_ptr->getType().getName() << std::endl;
                std::cout << fmt::format("    bbox: ({}, {}, {}, {})", bbox.left(),
                                         bbox.bottom(), bbox.right(), bbox.top()) << std::endl;
            }
        }

        // close design and return master
        dsn_ptr->close();
    }

    void Library::close() {
        if (is_open) {
            tech_ptr->close();
            lib_ptr->close();

            is_open = false;
        }

    }

    void add_param(cbag::ParamMap &params, oa::oaProp *prop_ptr) {
        // get parameter name
        oa::oaString tmp_str;
        prop_ptr->getName(tmp_str);
        std::string key(tmp_str);

        // get parameter value
        // NOTE: static_cast for down-casting is bad, but openaccess API sucks...
        // use NOLINT to suppress IDE warnings
        oa::oaType ptype = prop_ptr->getType();
        switch (ptype) {
            case oa::oacStringPropType : {
                prop_ptr->getValue(tmp_str);
                std::string vals(tmp_str);
                params.emplace(key, vals);
                break;
            }
            case oa::oacIntPropType : {
                params.emplace(key, static_cast<oa::oaIntProp *>(prop_ptr)->getValue()); // NOLINT
                break;
            }
            case oa::oacDoublePropType : {
                params.emplace(key,
                               static_cast<oa::oaDoubleProp *>(prop_ptr)->getValue()); // NOLINT
                break;
            }
            case oa::oacFloatPropType : {
                double vald = static_cast<oa::oaFloatProp *>(prop_ptr)->getValue(); // NOLINT
                params.emplace(key, vald);
                break;
            }
            default :
                throw std::invalid_argument(fmt::format("Unsupported OA property type: {}, see developer.",
                                                        ptype.getName()));
        }
    }

}