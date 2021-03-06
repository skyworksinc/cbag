/** \file read_oa.cpp
 *  \brief This file converts OpenAccess objects to CBAG data structure.
 *
 *  \author Eric Chang
 *  \date   2018/07/13
 */

#include <memory>
#include <utility>

#include <cbag/logging/spdlog.h>

#include <cbag/common/transformation_util.h>
#include <cbag/schematic/cellview.h>
#include <cbag/schematic/instance.h>

#include <cbag/oa/oa_read.h>
#include <cbag/oa/oa_util.h>

namespace cbagoa {

// enum conversion methods

cbag::path_style get_path_style(oa::oaPathStyleEnum oa_enum) {
    return static_cast<cbag::path_style>(oa_enum);
}

cbag::text_align get_text_align(oa::oaTextAlignEnum oa_enum) {
    return static_cast<cbag::text_align>(oa_enum);
}

cbag::orientation get_orientation(oa::oaOrientEnum oa_enum) {
    switch (oa_enum) {
    case oa::oacR0:
        return cbag::oR0;
    case oa::oacR90:
        return cbag::oR90;
    case oa::oacR180:
        return cbag::oR180;
    case oa::oacR270:
        return cbag::oR270;
    case oa::oacMY:
        return cbag::oMY;
    case oa::oacMYR90:
        return cbag::oMYR90;
    case oa::oacMX:
        return cbag::oMX;
    case oa::oacMXR90:
        return cbag::oMXR90;
    default:
        throw std::invalid_argument("Unknown OA orientation code.");
    }
}

cbag::font_t get_font(oa::oaFontEnum oa_enum) { return static_cast<cbag::font_t>(oa_enum); }

cbag::text_disp_format get_text_disp_format(oa::oaTextDisplayFormatEnum oa_enum) {
    return static_cast<cbag::text_disp_format>(oa_enum);
}

cbag::term_attr_type get_term_attr_type(oa::oaTermAttrTypeEnum oa_enum) {
    return static_cast<cbag::term_attr_type>(oa_enum);
}

cbag::sig_type get_sig_type(oa::oaSigTypeEnum oa_enum) {
    return static_cast<cbag::sig_type>(oa_enum);
}

cbag::term_type get_term_type(oa::oaTermTypeEnum oa_enum) {
    return static_cast<cbag::term_type>(oa_enum);
}

// Read method for properties

std::pair<std::string, cbag::value_t> read_prop(oa::oaProp *p) {
    oa::oaString tmp_str;
    p->getName(tmp_str);
    std::string key(tmp_str);
    // NOTE: static_cast for down-casting is bad, but openaccess API sucks...
    switch (p->getType()) {
    case oa::oacStringPropType: {
        p->getValue(tmp_str);
        return {std::move(key), std::string(tmp_str)};
    }
    case oa::oacIntPropType: {
        return {std::move(key),
                static_cast<int_fast32_t>(static_cast<oa::oaIntProp *>(p)->getValue())};
    }
    case oa::oacDoublePropType: {
        return {std::move(key),
                static_cast<double_t>(static_cast<oa::oaDoubleProp *>(p)->getValue())};
    }
    case oa::oacTimePropType: {
        return {std::move(key), cbag::time_struct{static_cast<oa::oaTimeProp *>(p)->getValue()}};
    }
    case oa::oacAppPropType: {
        oa::oaByteArray data;
        oa::oaString app_str;
        oa::oaAppProp *app_ptr = static_cast<oa::oaAppProp *>(p);
        app_ptr->getValue(data);
        app_ptr->getAppType(app_str);
        const unsigned char *data_ptr = data.getElements();
        return {std::move(key),
                cbag::binary_t{(const char *)app_str,
                               std::string(data_ptr, data_ptr + data.getNumElements())}};
    }
    case oa::oacBooleanPropType: {
        return {std::move(key), static_cast<bool>(static_cast<oa::oaBooleanProp *>(p)->getValue())};
    }
    default: {
        throw std::invalid_argument(
            fmt::format("Unsupported OA property {} with type: {}, see developer.", key,
                        (const char *)p->getType().getName()));
    }
    }
}

std::pair<std::string, cbag::value_t> read_app_def(oa::oaDesign *dsn, oa::oaAppDef *p) {
    oa::oaString tmp_str;
    p->getName(tmp_str);
    std::string key(tmp_str);
    // NOTE: static_cast for down-casting is bad, but openaccess API sucks...
    switch (p->getType()) {
    case oa::oacIntAppDefType: {
        return {std::move(key), static_cast<int_fast32_t>(
                                    (static_cast<oa::oaIntAppDef<oa::oaDesign> *>(p))->get(dsn))};
    }
    case oa::oacStringAppDefType: {
        (static_cast<oa::oaStringAppDef<oa::oaDesign> *>(p))->get(dsn, tmp_str);
        return {std::move(key), std::string(tmp_str)};
    }
    default: {
        throw std::invalid_argument(
            fmt::format("Unsupported OA AppDef {} with type: {}, see developer.", key,
                        (const char *)p->getType().getName()));
    }
    }
}

// Read methods for shapes

cbag::sch::rectangle read_rect(oa::oaRect *p, std::string &&net) {
    oa::oaBox box;
    p->getBBox(box);
    cbag::sch::rectangle ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), box.left(),
                             box.bottom(), box.right(), box.top());
    return ans;
}

cbag::sch::polygon read_poly(oa::oaPolygon *p, std::string &&net) {
    oa::oaPointArray arr;
    p->getPoints(arr);
    oa::oaUInt4 size = p->getNumPoints();
    cbag::sch::polygon ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), size);
    for (oa::oaUInt4 idx = 0; idx < size; ++idx) {
        ans.points.emplace_back(cbag::point{arr[idx].x(), arr[idx].y()});
    }
    return ans;
}

cbag::sch::arc read_arc(oa::oaArc *p, std::string &&net) {
    oa::oaBox box;
    p->getBBox(box);
    cbag::sch::arc ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), p->getStartAngle(),
                       p->getStopAngle(), box.left(), box.bottom(), box.right(), box.top());
    return ans;
}

cbag::sch::donut read_donut(oa::oaDonut *p, std::string &&net) {
    oa::oaPoint pt;
    p->getCenter(pt);
    cbag::sch::donut ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), p->getRadius(),
                         p->getHoleRadius(), pt.x(), pt.y());
    return ans;
}

cbag::sch::ellipse read_ellipse(oa::oaEllipse *p, std::string &&net) {
    oa::oaBox box;
    p->getBBox(box);
    cbag::sch::ellipse ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), box.left(),
                           box.bottom(), box.right(), box.top());
    return ans;
}

cbag::sch::line read_line(oa::oaLine *p, std::string &&net) {
    oa::oaPointArray arr;
    p->getPoints(arr);
    oa::oaUInt4 size = p->getNumPoints();
    cbag::sch::line ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), size);
    for (oa::oaUInt4 idx = 0; idx < size; ++idx) {
        ans.points.emplace_back(cbag::point{arr[idx].x(), arr[idx].y()});
    }
    return ans;
}

cbag::sch::path read_path(oa::oaPath *p, std::string &&net) {
    oa::oaPointArray arr;
    p->getPoints(arr);
    oa::oaUInt4 size = p->getNumPoints();
    cbag::sch::path ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), p->getWidth(), size,
                        get_path_style(p->getStyle()), p->getBeginExt(), p->getEndExt());
    for (oa::oaUInt4 idx = 0; idx < size; ++idx) {
        ans.points.emplace_back(cbag::point{arr[idx].x(), arr[idx].y()});
    }
    return ans;
}

cbag::sch::text_t read_text(oa::oaText *p, std::string &&net) {
    oa::oaString text;
    p->getText(text);
    oa::oaPoint pt;
    p->getOrigin(pt);
    bool overbar = (p->hasOverbar() != 0);
    bool visible = (p->isVisible() != 0);
    bool drafting = (p->isDrafting() != 0);
    cbag::sch::text_t ans(p->getLayerNum(), p->getPurposeNum(), std::move(net), std::string(text),
                          get_text_align(p->getAlignment()), get_orientation(p->getOrient()),
                          get_font(p->getFont()), p->getHeight(), overbar, visible, drafting,
                          pt.x(), pt.y());
    return ans;
}

cbag::sch::eval_text read_eval_text(oa::oaEvalText *p, std::string &&net) {
    oa::oaString text, eval;
    p->getText(text);
    p->getEvaluatorName(eval);
    oa::oaPoint pt;
    p->getOrigin(pt);
    bool overbar = (p->hasOverbar() != 0);
    bool visible = (p->isVisible() != 0);
    bool drafting = (p->isDrafting() != 0);
    cbag::sch::eval_text ans(
        p->getLayerNum(), p->getPurposeNum(), std::move(net), std::string(text),
        get_text_align(p->getAlignment()), get_orientation(p->getOrient()), get_font(p->getFont()),
        p->getHeight(), overbar, visible, drafting, std::string(eval), pt.x(), pt.y());
    return ans;
}

/** Returns true if the given shape should be included.
 *
 *  The rules are:
 *  1. if a shape has a pin, don't include it (we already added it to the pins).
 *  2. if a shape is an attribute display of a terminal, don't include it (we
 * already added it).
 *  3. otherwise, include it.
 */
bool include_shape(oa::oaShape *p) {
    if (!p->hasPin()) {
        if (p->getType() == oa::oacAttrDisplayType) {
            auto disp = static_cast<oa::oaAttrDisplay *>(p);
            if (disp->getObject()->isDesign()) {
                auto obj = static_cast<oa::oaDesignObject *>(disp->getObject());
                if (obj->isBlockObject()) {
                    return !((static_cast<oa::oaBlockObject *>(obj))->isTerm());
                } else {
                    return true;
                }
            } else {
                return true;
            }
        } else {
            return true;
        }
    } else {
        return false;
    }
}

cbag::sch::shape_t read_shape(const oa::oaCdbaNS &ns, spdlog::logger &logger, oa::oaShape *p) {
    std::string net;
    if (p->hasNet()) {
        oa::oaString net_name;
        p->getNet()->getName(ns, net_name);
        net = std::string(net_name);
        logger.info("Shape associated with net: {}", net);
    } else {
        logger.info("Shape has no net");
    }

    // NOTE: static_cast for down-casting is bad, but openaccess API sucks...
    switch (p->getType()) {
    case oa::oacRectType:
        return read_rect(static_cast<oa::oaRect *>(p), std::move(net));
    case oa::oacPolygonType:
        return read_poly(static_cast<oa::oaPolygon *>(p), std::move(net));
    case oa::oacArcType:
        return read_arc(static_cast<oa::oaArc *>(p), std::move(net));
    case oa::oacDonutType:
        return read_donut(static_cast<oa::oaDonut *>(p), std::move(net));
    case oa::oacEllipseType:
        return read_ellipse(static_cast<oa::oaEllipse *>(p), std::move(net));
    case oa::oacLineType:
        return read_line(static_cast<oa::oaLine *>(p), std::move(net));
    case oa::oacPathType:
        return read_path(static_cast<oa::oaPath *>(p), std::move(net));
    case oa::oacTextType:
        return read_text(static_cast<oa::oaText *>(p), std::move(net));
    case oa::oacEvalTextType:
        return read_eval_text(static_cast<oa::oaEvalText *>(p), std::move(net));
    default: {
        throw std::invalid_argument(fmt::format("Unsupported OA shape type: {}, see developer.",
                                                (const char *)p->getType().getName()));
    }
    }
}

// Read method for references
cbag::transformation get_xform(const oa::oaTransform &xform) {
    return cbag::make_xform(xform.xOffset(), xform.yOffset(), get_orientation(xform.orient()));
}

cbag::sch::instance read_instance(const oa::oaCdbaNS &ns, spdlog::logger &logger, oa::oaInst *p,
                                  const std::unordered_set<std::string> &primitive_libs) {
    // read cellview name
    oa::oaString inst_lib_oa, inst_cell_oa, inst_view_oa;
    p->getLibName(ns, inst_lib_oa);
    p->getCellName(ns, inst_cell_oa);
    p->getViewName(ns, inst_view_oa);

    // read transform and bounding box
    oa::oaTransform xform;
    p->getTransform(xform);
    oa::oaBox bbox;
    p->getBBox(bbox);

    // create instance object
    std::string lib_name(inst_lib_oa);
    cbag::sch::instance inst(lib_name, std::string(inst_cell_oa), std::string(inst_view_oa),
                             get_xform(xform), bbox.left(), bbox.bottom(), bbox.right(),
                             bbox.top());
    inst.is_primitive = primitive_libs.find(lib_name) != primitive_libs.end();
    // read instance parameters
    if (p->hasProp()) {
        oa::oaIter<oa::oaProp> prop_iter(p->getProps());
        oa::oaProp *prop_ptr;
        while ((prop_ptr = prop_iter.getNext()) != nullptr) {
            inst.params.insert(read_prop(prop_ptr));
        }
    }

    // read instance connections
    logger.info("Reading connections");
    oa::oaIter<oa::oaInstTerm> iterm_iter(p->getInstTerms(oacInstTermIterNotImplicit));
    oa::oaInstTerm *iterm_ptr;
    oa::oaString term_name_oa, net_name_oa;
    while ((iterm_ptr = iterm_iter.getNext()) != nullptr) {
        // get terminal and net names
        iterm_ptr->getTermName(ns, term_name_oa);
        iterm_ptr->getNet()->getName(ns, net_name_oa);
        logger.info("Terminal {} connected to net {}", (const char *)term_name_oa,
                    (const char *)net_name_oa);
        inst.connections.emplace(std::string(term_name_oa), std::string(net_name_oa));
    }

    return inst;
}

std::pair<std::string, std::unique_ptr<cbag::sch::instance>>
read_instance_pair(const oa::oaCdbaNS &ns, spdlog::logger &logger, oa::oaInst *p,
                   const std::unordered_set<std::string> &primitive_libs) {
    oa::oaString inst_name_oa;
    p->getName(ns, inst_name_oa);
    logger.info("Reading instance {}", (const char *)inst_name_oa);
    return {std::string(inst_name_oa),
            std::make_unique<cbag::sch::instance>(read_instance(ns, logger, p, primitive_libs))};
}

// Read method for pin figures

cbag::sch::pin_figure read_pin_figure(const oa::oaCdbaNS &ns, spdlog::logger &logger, oa::oaTerm *t,
                                      oa::oaPinFig *p,
                                      const std::unordered_set<std::string> &primitive_libs) {
    cbag::sig_type stype = get_sig_type(t->getNet()->getSigType());
    cbag::term_type ttype = get_term_type(t->getTermType());
    if (p->isInst()) {
        cbag::sch::instance inst =
            read_instance(ns, logger, static_cast<oa::oaInst *>(p), primitive_libs);

        oa::oaTextDisplayIter disp_iter(oa::oaTextDisplay::getTextDisplays(t));
        auto *disp_ptr = static_cast<oa::oaAttrDisplay *>(disp_iter.getNext());
        if (disp_ptr == nullptr) {
            throw std::invalid_argument(fmt::format("Terminal has no attr display."));
        }
        if (disp_iter.getNext() != nullptr) {
            throw std::invalid_argument(fmt::format("Terminal has more than one attr display."));
        }

        bool overbar = (disp_ptr->hasOverbar() != 0);
        bool visible = (disp_ptr->isVisible() != 0);
        bool drafting = (disp_ptr->isDrafting() != 0);
        std::string net;
        if (disp_ptr->hasNet()) {
            oa::oaString net_name;
            disp_ptr->getNet()->getName(ns, net_name);
            net = std::string(net_name);
        }
        oa::oaPoint pt;
        disp_ptr->getOrigin(pt);
        cbag::sch::term_attr attr(
            get_term_attr_type(
                oa::oaTermAttrType(disp_ptr->getAttribute().getRawValue()).getValue()),
            disp_ptr->getLayerNum(), disp_ptr->getPurposeNum(), std::move(net),
            get_text_align(disp_ptr->getAlignment()), get_orientation(disp_ptr->getOrient()),
            get_font(disp_ptr->getFont()), disp_ptr->getHeight(),
            get_text_disp_format(disp_ptr->getFormat()), overbar, visible, drafting, pt.x(),
            pt.y());

        return {cbag::sch::pin_object(std::move(inst), std::move(attr)), stype, ttype};
    } else if (p->getType() == oa::oacRectType) {
        oa::oaRect *r = static_cast<oa::oaRect *>(p);
        std::string net;
        if (r->hasNet()) {
            oa::oaString tmp;
            r->getNet()->getName(ns, tmp);
            net = std::string(tmp);
        }
        return {read_rect(r, std::move(net)), stype, ttype};
    } else {
        throw std::invalid_argument(
            fmt::format("Unsupported OA pin figure type: {}, see developer.",
                        (const char *)p->getType().getName()));
    }
}

// Read method for terminals

std::pair<std::string, cbag::sch::pin_figure>
read_terminal_single(const oa::oaCdbaNS &ns, spdlog::logger &logger, oa::oaTerm *term,
                     const std::unordered_set<std::string> &primitive_libs) {
    // parse terminal name
    oa::oaString term_name_oa;
    term->getName(ns, term_name_oa);

    // get pin
    oa::oaIter<oa::oaPin> pin_iter(term->getPins());
    oa::oaPin *pin_ptr = pin_iter.getNext();
    if (pin_ptr == nullptr) {
        throw std::invalid_argument(
            fmt::format("Terminal {} has no pins.", (const char *)term_name_oa));
    }
    if (pin_iter.getNext() != nullptr) {
        throw std::invalid_argument(
            fmt::format("Terminal {} has more than one pin.", (const char *)term_name_oa));
    }

    // get pin figure
    oa::oaIter<oa::oaPinFig> fig_iter(pin_ptr->getFigs());
    oa::oaPinFig *fig_ptr = fig_iter.getNext();
    if (fig_ptr == nullptr) {
        throw std::invalid_argument(
            fmt::format("Terminal {} has no figures.", (const char *)term_name_oa));
    }
    if (fig_iter.getNext() != nullptr) {
        throw std::invalid_argument(
            fmt::format("Terminal {} has more than one figures.", (const char *)term_name_oa));
    }

    return {std::string(term_name_oa), read_pin_figure(ns, logger, term, fig_ptr, primitive_libs)};
};

// Read method for schematic/symbol cell view

cbag::sch::cellview read_sch_cellview(const oa::oaNativeNS &ns_native, const oa::oaCdbaNS &ns,
                                      spdlog::logger &logger, const std::string &lib_name,
                                      const std::string &cell_name, const std::string &view_name,
                                      const std::unordered_set<std::string> &primitive_libs) {
    oa::oaDesign *p =
        open_design(ns_native, logger, lib_name, cell_name, view_name, 'r', oa::oacSchematic);

    logger.info("Reading cellview {}__{}({})", lib_name, cell_name, view_name);
    oa::oaBlock *block = p->getTopBlock();
    oa::oaBox bbox;
    block->getBBox(bbox);

    cbag::sch::cellview ans(lib_name, cell_name, view_name, bbox.left(), bbox.bottom(),
                            bbox.right(), bbox.top());

    // read terminals
    logger.info("Reading terminals");
    oa::oaIter<oa::oaTerm> term_iter(block->getTerms());
    oa::oaTerm *term_ptr;
    oa::oaString tmp;
    while ((term_ptr = term_iter.getNext()) != nullptr) {
        term_ptr->getName(ns, tmp);
        ans.terminals.insert(read_terminal_single(ns, logger, term_ptr, primitive_libs));
    }

    // read shapes
    logger.info("Reading shapes");
    oa::oaIter<oa::oaShape> shape_iter(block->getShapes());
    oa::oaShape *shape_ptr;
    while ((shape_ptr = shape_iter.getNext()) != nullptr) {
        logger.info("shape type: {}", (const char *)shape_ptr->getType().getName());
        // skip shapes associated with pins.  We got those already.
        if (include_shape(shape_ptr)) {
            ans.shapes.push_back(read_shape(ns, logger, shape_ptr));
        } else {
            logger.info("Skipping this shape");
        }
    }

    // read instances
    logger.info("Reading instances");
    oa::oaIter<oa::oaInst> inst_iter(block->getInsts());
    oa::oaInst *inst_ptr;
    while ((inst_ptr = inst_iter.getNext()) != nullptr) {
        // skip instances associated with pins.  We got those already.
        if (!inst_ptr->hasPin()) {
            ans.instances.insert(read_instance_pair(ns, logger, inst_ptr, primitive_libs));
        }
    }

    // read properties
    logger.info("Reading properties");
    oa::oaIter<oa::oaProp> prop_iter(p->getProps());
    oa::oaProp *prop_ptr;
    while ((prop_ptr = prop_iter.getNext()) != nullptr) {
        ans.props.insert(read_prop(prop_ptr));
    }
    logger.info("properties end");

    logger.info("Reading AppDefs");
    oa::oaIter<oa::oaAppDef> appdef_iter(p->getAppDefs());
    oa::oaAppDef *appdef_ptr;
    while ((appdef_ptr = appdef_iter.getNext()) != nullptr) {
        ans.app_defs.insert(read_app_def(p, appdef_ptr));
    }
    logger.info("AppDefs end");

    /*
    logger->info("Reading design groups");
    oa::oaIter<oa::oaGroup> grp_iter(p->getGroups(oacGroupIterBlockDomain | oacGroupIterModDomain |
                                                  oacGroupIterNoDomain | oacGroupIterOccDomain));
    oa::oaGroup *grp_ptr;
    while ((grp_ptr = grp_iter.getNext()) != nullptr) {
        print_group(grp_ptr);
    }
    logger->info("Groups end");
    */

    /*
    logger->info("Reading cell DM data");
    oa::oaScalarName lib_name;
    oa::oaScalarName cell_name;
    p->getLibName(lib_name);
    p->getCellName(cell_name);
    oa::oaCellDMData *data = oa::oaCellDMData::open(lib_name, cell_name, 'r');
    print_dm_data(data);

    logger->info("Reading cellview DM data");
    oa::oaScalarName view_name;
    p->getViewName(view_name);
    if (oa::oaCellViewDMData::exists(lib_name, cell_name, view_name)) {
        oa::oaCellViewDMData *cv_data =
            oa::oaCellViewDMData::open(lib_name, cell_name, view_name, 'r');
        print_dm_data(cv_data);
    }

    logger->info("Reading time stamps");
    for (unsigned int idx = 0; idx < 81; ++idx) {
        try {
            oa::oaDesignDataType data_type(static_cast<oa::oaDesignDataTypeEnum>(idx));
            logger->info("{} timestamp = {}", (const char *)data_type.getName(),
                         p->getTimeStamp(data_type));
        } catch (...) {
            logger->info("error on idx = {}", idx);
        }
    }
    */

    logger.info("Finish reading schematic/symbol cellview");

    p->close();
    return ans;
}

void print_prop(spdlog::logger &logger, oa::oaObject *obj) {
    if (obj->hasProp()) {
        oa::oaIter<oa::oaProp> prop_iter(obj->getProps());
        oa::oaProp *p;
        logger.info("Reading properties");
        while ((p = prop_iter.getNext()) != nullptr) {
            oa::oaString name;
            oa::oaString val;
            p->getName(name);
            p->getValue(val);
            logger.info("Property name = {}, value = {}, type = {}", (const char *)name,
                        (const char *)val, (const char *)p->getType().getName());
            if (val == "oaHierProp") {
                logger.info("Hierarchical properties:");
                print_prop(logger, p);
            } else if (p->getType().getName() == "AppProp") {
                static_cast<oa::oaAppProp *>(p)->getAppType(val);
                logger.info("AppProp type: {}", (const char *)val);
            }
        }
        logger.info("properties end");
    } else {
        logger.info("No properties");
    }
}

void print_app_def(spdlog::logger &logger, oa::oaDesign *dsn, oa::oaAppDef *p) {
    oa::oaString name;
    p->getName(name);
    // NOTE: static_cast for down-casting is bad, but openaccess API sucks...
    switch (p->getType()) {
    case oa::oacIntAppDefType: {
        logger.info("AppDef name: {}, AppDef value: {}", (const char *)name,
                    (static_cast<oa::oaIntAppDef<oa::oaDesign> *>(p))->get(dsn));
        break;
    }
    case oa::oacStringAppDefType: {
        oa::oaString val;
        (static_cast<oa::oaStringAppDef<oa::oaDesign> *>(p))->get(dsn, val);
        logger.info("AppDef name: {}, AppDef value: {}", (const char *)name, (const char *)val);
        break;
    }
    default: {
        throw std::invalid_argument(
            fmt::format("Unsupported OA AppDef {} with type: {}, see developer.",
                        (const char *)name, (const char *)p->getType().getName()));
    }
    }
    print_prop(logger, p);
}

void print_group(spdlog::logger &logger, oa::oaGroup *p) {
    oa::oaString grp_str;
    p->getName(grp_str);
    logger.info("group name: {}, domain: {}", (const char *)grp_str,
                (const char *)p->getGroupDomain().getName());
    logger.info("group has prop: {}, has appdef: {}", p->hasProp(), p->hasAppDef());
    p->getDef()->getName(grp_str);
    logger.info("group def name: {}", (const char *)grp_str);
    oa::oaIter<oa::oaGroupMember> mem_iter(p->getMembers());
    oa::oaGroupMember *mem_ptr;
    while ((mem_ptr = mem_iter.getNext()) != nullptr) {
        logger.info("group object type: {}",
                    (const char *)mem_ptr->getObject()->getType().getName());
    }
}

void print_dm_data(spdlog::logger &logger, oa::oaDMData *data) {
    logger.info("Has app def: {}", data->hasAppDef());
    print_prop(logger, data);
    logger.info("Reading groups");
    oa::oaIter<oa::oaGroup> grp_iter(data->getGroups());
    oa::oaGroup *grp_ptr;
    while ((grp_ptr = grp_iter.getNext()) != nullptr) {
        print_group(logger, grp_ptr);
    }
    logger.info("Groups end");

    logger.info("Reading AppObjects");
    oa::oaIter<oa::oaAppObjectDef> odef_iter(data->getAppObjectDefs());
    oa::oaAppObjectDef *odef_ptr;
    while ((odef_ptr = odef_iter.getNext()) != nullptr) {
        logger.info("has object def");
    }
    logger.info("AppObjects end");

    logger.info("Reading time stamps");
    for (unsigned int idx = 0; idx < 15; ++idx) {
        try {
            oa::oaDMDataType data_type(static_cast<oa::oaDMDataTypeEnum>(idx));
            logger.info("{} timestamp = {}", (const char *)data_type.getName(),
                        data->getTimeStamp(data_type));
        } catch (...) {
            logger.info("error on idx = {}", idx);
        }
    }
}

} // namespace cbagoa
