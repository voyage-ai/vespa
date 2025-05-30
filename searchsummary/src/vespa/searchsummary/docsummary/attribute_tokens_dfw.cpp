// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "attribute_tokens_dfw.h"
#include "docsumstate.h"
#include "empty_docsum_field_writer_state.h"
#include <vespa/searchcommon/attribute/iattributevector.h>
#include <vespa/searchcommon/attribute/i_multi_value_attribute.h>
#include <vespa/vespalib/data/slime/slime.h>
#include <vespa/vespalib/text/lowercase.h>
#include <vespa/vespalib/text/utf8.h>
#include <vespa/vespalib/util/stash.h>

using search::attribute::IAttributeVector;
using search::attribute::BasicType;
using search::attribute::IMultiValueAttribute;
using search::attribute::IMultiValueReadView;
using vespalib::LowerCase;
using vespalib::Utf8Reader;
using vespalib::Utf8Writer;
using vespalib::slime::ArrayInserter;
using vespalib::slime::Cursor;
using vespalib::slime::Inserter;

namespace search::docsummary {

namespace {

const IMultiValueReadView<const char*>*
make_read_view(const IAttributeVector& attribute, vespalib::Stash& stash)
{
    auto multi_value_attribute = attribute.as_multi_value_attribute();
    if (multi_value_attribute != nullptr) {
        return multi_value_attribute->make_read_view(IMultiValueAttribute::MultiValueTag<const char*>(), stash);
    }
    return nullptr;
}

void
insert_value(std::string_view value, Inserter& inserter, std::string& scratch, bool lowercase)
{
    Cursor& arr = inserter.insertArray(1);
    ArrayInserter ai(arr);
    if (lowercase) {
        scratch.clear();
        Utf8Reader r(value);
        Utf8Writer w(scratch);
        while (r.hasMore()) {
            w.putChar(LowerCase::convert(r.getChar()));
        }
        ai.insertString(scratch);
    } else {
        ai.insertString(value);
    }
}

}

class MultiAttributeTokensDFWState : public DocsumFieldWriterState
{
    const IMultiValueReadView<const char*>* _read_view;
    std::string                        _lowercase_scratch;
    bool                                    _lowercase;
public:
    MultiAttributeTokensDFWState(const IAttributeVector& attr, vespalib::Stash& stash);
    ~MultiAttributeTokensDFWState() override;
    void insertField(uint32_t docid, ElementIds selected_elements, Inserter& target) override;
};

MultiAttributeTokensDFWState::MultiAttributeTokensDFWState(const IAttributeVector& attr, vespalib::Stash& stash)
    : DocsumFieldWriterState(),
      _read_view(make_read_view(attr, stash)),
      _lowercase_scratch(),
      _lowercase(attr.has_uncased_matching())
{
}

MultiAttributeTokensDFWState::~MultiAttributeTokensDFWState() = default;

void
MultiAttributeTokensDFWState::insertField(uint32_t docid, ElementIds selected_elements, Inserter& target)
{
    (void) selected_elements;
    if (!_read_view) {
        return;
    }
    auto elements = _read_view->get_values(docid);
    if (elements.empty()) {
        return;
    }
    Cursor &arr = target.insertArray(elements.size());
    ArrayInserter ai(arr);
    for (const auto & element : elements) {
        insert_value(element, ai, _lowercase_scratch, _lowercase);
    }
}

class SingleAttributeTokensDFWState : public DocsumFieldWriterState
{
    const IAttributeVector& _attr;
    std::string        _lowercase_scratch;
    bool                    _lowercase;
public:
    SingleAttributeTokensDFWState(const IAttributeVector& attr);
    ~SingleAttributeTokensDFWState() override;
    void insertField(uint32_t docid, ElementIds selected_elements, Inserter& target) override;
};

SingleAttributeTokensDFWState::SingleAttributeTokensDFWState(const IAttributeVector& attr)
    : DocsumFieldWriterState(),
      _attr(attr),
      _lowercase_scratch(),
      _lowercase(attr.has_uncased_matching())
{
}

SingleAttributeTokensDFWState::~SingleAttributeTokensDFWState() = default;

void
SingleAttributeTokensDFWState::insertField(uint32_t docid, ElementIds, Inserter& target)
{
    auto s = _attr.get_raw(docid);
    insert_value(std::string_view(s.data(), s.size()), target, _lowercase_scratch, _lowercase);
}

DocsumFieldWriterState*
make_field_writer_state(const IAttributeVector& attr, vespalib::Stash& stash)
{
    auto type = attr.getBasicType();
    switch (type) {
    case BasicType::Type::STRING:
        if (attr.hasMultiValue()) {
            return &stash.create<MultiAttributeTokensDFWState>(attr, stash);
        } else {
            return &stash.create<SingleAttributeTokensDFWState>(attr);
        }
    default:
        ;
    }
    return &stash.create<EmptyDocsumFieldWriterState>();
}

AttributeTokensDFW::AttributeTokensDFW(const std::string& input_field_name)
    : DocsumFieldWriter(),
      _input_field_name(input_field_name)
{
}

AttributeTokensDFW::~AttributeTokensDFW() = default;

const std::string&
AttributeTokensDFW::getAttributeName() const
{
    return _input_field_name;
}

bool
AttributeTokensDFW::isGenerated() const
{
    return true;
}

bool
AttributeTokensDFW::setFieldWriterStateIndex(uint32_t fieldWriterStateIndex)
{
    _state_index = fieldWriterStateIndex;
    return true;
}

void
AttributeTokensDFW::insert_field(uint32_t docid, const IDocsumStoreDocument*, GetDocsumsState& state,
                                 ElementIds selected_elements,
                                 vespalib::slime::Inserter& target) const
{
    auto& field_writer_state = state._fieldWriterStates[_state_index];
    if (!field_writer_state) {
        const auto attr = state.getAttribute(getIndex());
        if (attr != nullptr) {
            field_writer_state = make_field_writer_state(*attr, state.get_stash());
        } else {
            field_writer_state = &state.get_stash().create<EmptyDocsumFieldWriterState>();
        }
    }
    field_writer_state->insertField(docid, selected_elements, target);
}

}
