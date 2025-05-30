// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "attribute_combiner_dfw.h"
#include "attribute_tokens_dfw.h"
#include "copy_dfw.h"
#include "docsum_field_writer_commands.h"
#include "docsum_field_writer_factory.h"
#include "document_id_dfw.h"
#include "empty_dfw.h"
#include "geoposdfw.h"
#include "idocsumenvironment.h"
#include "dynamicteaserdfw.h"
#include "positionsdfw.h"
#include "rankfeaturesdfw.h"
#include "summaryfeaturesdfw.h"
#include "tokens_dfw.h"
#include <vespa/searchlib/common/matching_elements_fields.h>
#include <vespa/vespalib/util/exceptions.h>
#include <vespa/vespalib/util/issue.h>

using vespalib::IllegalArgumentException;
using vespalib::Issue;

namespace search::docsummary {

DocsumFieldWriterFactory::DocsumFieldWriterFactory(bool use_v8_geo_positions, const IDocsumEnvironment& env, const IQueryTermFilterFactory& query_term_filter_factory)
    : _use_v8_geo_positions(use_v8_geo_positions),
      _env(env),
      _query_term_filter_factory(query_term_filter_factory)
{
}

DocsumFieldWriterFactory::~DocsumFieldWriterFactory() = default;

bool
DocsumFieldWriterFactory::has_attribute_manager() const noexcept
{
    return getEnvironment().getAttributeManager() != nullptr;
}

namespace {

void
throw_if_nullptr(const std::unique_ptr<DocsumFieldWriter>& writer,
                 const std::string& command)
{
    if ( ! writer) {
        throw IllegalArgumentException("Failed to create docsum field writer for command '" + command + "'.");
    }
}

}

void
DocsumFieldWriterFactory::throw_missing_source(const std::string& command)
{
    throw IllegalArgumentException("Missing source for command '" + command + "'.");
}

std::unique_ptr<DocsumFieldWriter>
DocsumFieldWriterFactory::create_docsum_field_writer(const std::string& field_name,
                                                     const std::string& command,
                                                     const std::string& source)
{
    std::unique_ptr<DocsumFieldWriter> fieldWriter;
    if (command == command::dynamic_teaser) {
        if ( ! source.empty() ) {
            fieldWriter = std::make_unique<DynamicTeaserDFW>(getEnvironment().getJuniper(), field_name.c_str(), source, _query_term_filter_factory);
        } else {
            throw_missing_source(command);
        }
    } else if (command == command::summary_features) {
        fieldWriter = std::make_unique<SummaryFeaturesDFW>();
    } else if (command == command::rank_features) {
        fieldWriter = std::make_unique<RankFeaturesDFW>();
    } else if (command == command::empty) {
        fieldWriter = std::make_unique<EmptyDFW>();
    } else if (command == command::copy) {
        if ( ! source.empty() ) {
            fieldWriter = std::make_unique<CopyDFW>(source);
        } else {
            throw_missing_source(command);
        }
    } else if (command == command::tokens) {
        if (!source.empty()) {
            fieldWriter = std::make_unique<TokensDFW>(source);
        } else {
            throw_missing_source(command);
        }
    } else if (command == command::attribute_tokens) {
        if (!source.empty()) {
            if (has_attribute_manager()) {
                auto ctx = getEnvironment().getAttributeManager()->createContext();
                const auto* attr = ctx->getAttribute(source);
                if (attr == nullptr) {
                    Issue::report("No valid attribute vector found: field='%s', command='%s', source='%s'",
                                  field_name.c_str(), command.c_str(), source.c_str());
                } else {
                    fieldWriter = std::make_unique<AttributeTokensDFW>(source);
                }
            }
        } else {
            throw_missing_source(command);
        }
    } else if (command == command::abs_distance) {
        if (has_attribute_manager()) {
            fieldWriter = AbsDistanceDFW::create(source.c_str(), getEnvironment().getAttributeManager());
            throw_if_nullptr(fieldWriter, command);
        }
    } else if (command == command::positions) {
        if (has_attribute_manager()) {
            fieldWriter = PositionsDFW::create(source.c_str(), getEnvironment().getAttributeManager(), _use_v8_geo_positions);
            throw_if_nullptr(fieldWriter, command);
        }
    } else if (command == command::geo_position) {
        if (has_attribute_manager()) {
            fieldWriter = GeoPositionDFW::create(source.c_str(), getEnvironment().getAttributeManager(), _use_v8_geo_positions);
            throw_if_nullptr(fieldWriter, command);
        }
    } else if (command == command::attribute) {
        if (has_attribute_manager()) {
            fieldWriter = AttributeDFWFactory::create(*getEnvironment().getAttributeManager(), source);
            // Missing attribute vector is allowed, so throw_if_nullptr() is NOT used.
        }
    } else if (command == command::attribute_combiner) {
        if (has_attribute_manager()) {
            auto attr_ctx = getEnvironment().getAttributeManager()->createContext();
            const std::string& source_field = source.empty() ? field_name : source;
            fieldWriter = AttributeCombinerDFW::create(source_field, *attr_ctx);
            throw_if_nullptr(fieldWriter, command);
        }
    } else if (command == command::matched_attribute_elements_filter) {
        const std::string& source_field = source.empty() ? field_name : source;
        if (has_attribute_manager()) {
            auto attr_ctx = getEnvironment().getAttributeManager()->createContext();
            if (attr_ctx->getAttribute(source_field) != nullptr) {
                fieldWriter = AttributeDFWFactory::create(*getEnvironment().getAttributeManager(), source_field);
            } else {
                fieldWriter = AttributeCombinerDFW::create(source_field, *attr_ctx);
            }
            throw_if_nullptr(fieldWriter, command);
        }
    } else if (command == command::matched_elements_filter) {
        const std::string& source_field = source.empty() ? field_name : source;
        fieldWriter = std::make_unique<CopyDFW>(source_field);
    } else if (command == command::documentid) {
        fieldWriter = std::make_unique<DocumentIdDFW>();
    } else {
        throw IllegalArgumentException("Unknown command '" + command + "'.");
    }
    return fieldWriter;
}

}
