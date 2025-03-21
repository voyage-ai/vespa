// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <memory>
#include <vespa/vespalib/testkit/test_kit.h>

#include <string>
#include <vespa/juniper/query_item.h>
#include <vespa/juniper/queryhandle.h>
#include <vespa/juniper/queryvisitor.h>

using namespace juniper;

struct MyQueryItem : public QueryItem {
    MyQueryItem() : QueryItem() {}
    ~MyQueryItem() override = default;

    std::string_view get_index() const override { return {}; }
    int              get_weight() const override { return 0; }
    ItemCreator      get_creator() const override { return ItemCreator::CREA_ORIG; }
};

class MyQuery : public juniper::IQuery {
private:
    std::string _term;

public:
    explicit MyQuery(const std::string& term) : _term(term) {}

    bool Traverse(IQueryVisitor* v) const override {
        MyQueryItem item;
        v->visitKeyword(&item, _term, false, false);
        return true;
    }
    bool UsefulIndex(const QueryItem*) const override { return true; }
};

struct Fixture {
    MyQuery       query;
    QueryHandle   handle;
    QueryVisitor  visitor;
    explicit Fixture(const std::string& term)
      : query(term),
        handle(query, ""),
        visitor(query, &handle) {}
};

TEST_F("require that terms are picked up by the query visitor", Fixture("my_term")) {
    auto query = std::unique_ptr<QueryExpr>(f.visitor.GetQuery());
    ASSERT_TRUE(query != nullptr);
    QueryNode* node = query->AsNode();
    ASSERT_TRUE(node != nullptr);
    EXPECT_EQUAL(1, node->_arity);
    QueryTerm* term = node->_children[0]->AsTerm();
    ASSERT_TRUE(term != nullptr);
    EXPECT_EQUAL("my_term", std::string(term->term()));
}

TEST_F("require that empty terms are ignored by the query visitor", Fixture("")) {
    QueryExpr* query = f.visitor.GetQuery();
    ASSERT_TRUE(query == nullptr);
}

TEST_MAIN() {
    TEST_RUN_ALL();
}
