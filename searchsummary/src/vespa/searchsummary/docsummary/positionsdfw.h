// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "attributedfw.h"
#include "resultconfig.h"
#include <vespa/searchlib/common/geo_location_spec.h>

namespace search::docsummary {

class LocationAttrDFW : public AttrDFW
{
public:
    using GeoLoc = search::common::GeoLocation;

    explicit LocationAttrDFW(const std::string & attrName)
        : AttrDFW(attrName)
    {}

    struct AllLocations {
        std::vector<const GeoLoc *> matching;
        std::vector<const GeoLoc *> other;

        AllLocations();
        ~AllLocations();

        bool empty() const {
            return matching.empty() && other.empty();
        }
        const std::vector<const GeoLoc *> & best() const {
            return matching.empty() ? other : matching;
        }
    };
    AllLocations getAllLocations(GetDocsumsState& state) const;
};

class AbsDistanceDFW : public LocationAttrDFW
{
private:
    uint64_t findMinDistance(uint32_t docid, GetDocsumsState& state,
                             const std::vector<const GeoLoc *> &locations) const;
public:
    explicit AbsDistanceDFW(const std::string & attrName);

    bool isGenerated() const override { return true; }
    void insert_field(uint32_t docid, const IDocsumStoreDocument* doc, GetDocsumsState& state,
                      ElementIds selected_elements,
                      vespalib::slime::Inserter &target) const override;

    static std::unique_ptr<DocsumFieldWriter> create(const char *attribute_name, const IAttributeManager *index_man);

};

//--------------------------------------------------------------------------

class PositionsDFW : public AttrDFW
{
private:
    bool _useV8geoPositions;
public:
    using UP = std::unique_ptr<PositionsDFW>;
    PositionsDFW(const std::string & attrName, bool useV8geoPositions);
    bool isGenerated() const override { return true; }
    void insert_field(uint32_t docid, const IDocsumStoreDocument* doc, GetDocsumsState& state,
                      ElementIds selected_elements,
                      vespalib::slime::Inserter &target) const override;
    static UP create(const char *attribute_name, const IAttributeManager *index_man, bool useV8geoPositions);
};


}
