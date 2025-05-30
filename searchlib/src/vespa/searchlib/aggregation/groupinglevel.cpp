// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "groupinglevel.h"
#include "grouping.h"
#include <vespa/searchlib/expression/resultvector.h>
#include <vespa/searchlib/expression/current_index_setup.h>

#include <vespa/log/log.h>
LOG_SETUP(".searchlib.aggregation.groupinglevel");

namespace search::aggregation {

using expression::ResultNodeVector;
using vespalib::Serializer;
using vespalib::Deserializer;

IMPLEMENT_IDENTIFIABLE_NS2(search, aggregation, GroupingLevel, vespalib::Identifiable);

GroupingLevel::GroupingLevel() noexcept
    : _maxGroups(-1),
      _precision(-1),
      _isOrdered(false),
      _frozen(false),
      _currentIndex(),
      _classify(),
      _collect(),
      _grouper(nullptr)
{ }

GroupingLevel::~GroupingLevel() = default;

GroupingLevel::GroupingLevel(const GroupingLevel &) = default;
GroupingLevel::GroupingLevel(GroupingLevel&&) noexcept = default;
GroupingLevel & GroupingLevel::operator =(const GroupingLevel &) = default;
GroupingLevel& GroupingLevel::operator=(GroupingLevel&&) noexcept = default;

Serializer &
GroupingLevel::onSerialize(Serializer & os) const
{
    return os << _maxGroups << _precision << _classify << _collect;
}

Deserializer &
GroupingLevel::onDeserialize(Deserializer & is)
{
    return is >> _maxGroups >> _precision >> _classify >> _collect;
}

Serializer &
GroupingLevel::serializeVariant(Serializer & os, bool allowV2) const {
    if (allowV2 && _filter.get()) {
        uint32_t cid = CID_search_aggregation_GroupingLevelV2;
        return os << cid << _maxGroups << _precision << _classify << _filter << _collect;
    } else {
        uint32_t cid = CID_search_aggregation_GroupingLevel;
        return onSerialize(os << cid);
    }
}

Deserializer &
GroupingLevel::deserializeVariant(Deserializer & is, bool allowV2) {
    uint32_t cid;
    is.get(cid);
    if (allowV2 && (cid == CID_search_aggregation_GroupingLevelV2)) {
        return is >> _maxGroups >> _precision >> _classify >> _filter >> _collect;
    } else {
        // assume old (V1):
        return onDeserialize(is);
    }
}

void
GroupingLevel::visitMembers(vespalib::ObjectVisitor &visitor) const
{
    visit(visitor, "maxGroups", _maxGroups);
    visit(visitor, "precision", _precision);
    visit(visitor, "classify",  _classify);
    if (_filter.get()) {
        visit(visitor, "filter",  *_filter);
    }
    visit(visitor, "collect",   _collect);
}

void
GroupingLevel::selectMembers(const vespalib::ObjectPredicate & predicate, vespalib::ObjectOperation & operation)
{
    _classify.select(predicate, operation);
    if (_filter.get()) {
        _filter->select(predicate, operation);
    }
    _collect.select(predicate, operation);
}

GroupingLevel::Grouper::Grouper(const Grouping * grouping, uint32_t level) noexcept
    : _grouping(grouping),
      _level(level),
      _frozen(_level < _grouping->getFirstLevel()),
      _hasNext(_level < _grouping->getLevels().size()),
      _doNext(_level < _grouping->getLastLevel())
{
}

bool
GroupingLevel::Grouper::hasNext(size_t level) const noexcept
{
    return level < _grouping->getLevels().size();
}

template<typename Doc>
void
GroupingLevel::SingleValueGrouper::groupDoc(Group & g, const ResultNode & result, const Doc & doc, HitRank rank) const
{
    if (_filter.allow(doc, rank)) {
        Group * next = g.groupSingle(result, rank, _grouping->getLevels()[_level]);
        if ((next != nullptr) && doNext()) { // do next level ?
            next->aggregate(*_grouping, _level + 1, doc, rank);
        }
    }
}

template<typename Doc>
void
GroupingLevel::MultiValueGrouper::groupDoc(Group & g, const ResultNode & result, const Doc & doc, HitRank rank) const
{
    const ResultNodeVector & rv(static_cast<const ResultNodeVector &>(result));
    for (size_t i(0), m(rv.size()); i < m; i++) {
        const ResultNode & sr(rv.get(i));
        _currentIndex.set(i);
        SingleValueGrouper::groupDoc(g, sr, doc, rank);
    }
}

void
GroupingLevel::wire_current_index(CurrentIndexSetup &setup, const vespalib::ObjectPredicate &resolve_pred, vespalib::ObjectOperation &resolve_op)
{
    CurrentIndexSetup::Usage usage;
    {
        CurrentIndexSetup::Usage::Bind capture_guard(setup, usage);
        _classify.select(resolve_pred, resolve_op);
    }
    if (usage.has_single_unbound_name()) {
        setup.bind(usage.get_unbound_name(), _currentIndex);
    }
    if (_filter.get()) {
        _filter->select(resolve_pred, resolve_op);
    }
    _collect.select(resolve_pred, resolve_op);
}

void
GroupingLevel::prepare(const Grouping * grouping, uint32_t level, bool isOrdered_)
{
    _isOrdered = isOrdered_;
    _frozen = level < grouping->getFirstLevel();
    if (_classify.getResult()->inherits(ResultNodeVector::classId)) {
        _grouper.reset(new MultiValueGrouper(_currentIndex, getActiveFilter(), grouping, level));
    } else {
        _grouper.reset(new SingleValueGrouper(getActiveFilter(), grouping, level));
    }
}

// template<> void GroupingLevel::MultiValueGrouper::groupDoc(Group & g, const ResultNode::CP & result, const document::Document & doc, HitRank rank, bool isOrdered) const;
// template<> void GroupingLevel::MultiValueGrouper::groupDoc(Group & g, const ResultNode::CP & result, DocId doc, HitRank rank, bool isOrdered) const;

}

// this function was added by ../../forcelink.sh
void forcelink_file_searchlib_aggregation_groupinglevel() {}
