# Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
vespa_add_executable(searchlib_direct_posting_store_test_app TEST
    SOURCES
    direct_posting_store_test.cpp
    DEPENDS
    vespa_searchlib
    searchlib_test
    GTest::gtest
)
vespa_add_test(NAME searchlib_direct_posting_store_test_app COMMAND searchlib_direct_posting_store_test_app)
