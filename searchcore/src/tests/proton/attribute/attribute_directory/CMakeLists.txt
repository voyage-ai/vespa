# Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
vespa_add_executable(searchcore_attribute_directory_test_app TEST
    SOURCES
    attribute_directory_test.cpp
    DEPENDS
    searchcore_attribute
    GTest::gtest
)
vespa_add_test(NAME searchcore_attribute_directory_test_app COMMAND searchcore_attribute_directory_test_app)
