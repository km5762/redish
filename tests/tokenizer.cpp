//
// Created by d4wgr on 5/20/2025.
//

#include "tokenizer.h"
#include "utils.h"

#include <gtest/gtest.h>

using namespace resp;

TEST(Tokenizer, EmptyArray) {
    const Array tokens = make_array();
    const Tokenizer tokenizer{tokens};

    const auto it = tokenizer.begin();
    const auto end = tokenizer.end();

    EXPECT_EQ(it, end);
}

TEST(Tokenizer, SingleValidString) {
    const Array tokens = make_array("hello");
    const Tokenizer tokenizer{tokens};

    auto it = tokenizer.begin();
    auto end = tokenizer.end();

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, "hello");

    ++it;
    EXPECT_EQ(it, end);
}


TEST(Tokenizer, SingleNullString) {
    const Array tokens = make_array(std::nullopt);
    const Tokenizer tokenizer{tokens};

    auto it = tokenizer.begin();
    auto end = tokenizer.end();

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, std::nullopt);

    ++it;
    EXPECT_EQ(it, end);
}


TEST(Tokenizer, MultipleMixedStrings) {
    const Array tokens = make_array("foo", std::nullopt, "bar");
    Tokenizer tokenizer(tokens);

    auto it = tokenizer.begin();
    auto end = tokenizer.end();

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, "foo");

    ++it;
    ASSERT_NE(it, end);
    EXPECT_EQ(*it, std::nullopt);

    ++it;
    ASSERT_NE(it, end);
    EXPECT_EQ(*it, "bar");

    ++it;
    EXPECT_EQ(it, end);
}


TEST(Tokenizer, IteratorEquality) {
    BulkString s1{std::make_optional<std::string>("value")};
    const Array tokens = make_array("value");
    Tokenizer tokenizer(tokens);

    auto begin = tokenizer.begin();
    auto end = tokenizer.end();

    auto copy = begin;
    EXPECT_EQ(copy, begin);
    ++copy;
    EXPECT_NE(copy, begin);
    EXPECT_EQ(copy, end);
}
