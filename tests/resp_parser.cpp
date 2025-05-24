#include "resp_parser.h"

#include <gtest/gtest.h>
#include <sstream>
#include <iterator>

using namespace resp;

static std::vector<Value> parse_all(const std::string &s, size_t buf_limit = 8192) {
    Parser p{buf_limit};
    // feed in one chunk
    p.feed({s.data(), s.size()});
    return p.take_values();
}

TEST(RespParser, SimpleStringHappyPath) {
    auto values = parse_all("+OK\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("OK", std::get<SimpleString>(values[0]).value);
}

TEST(RespParser, SimpleStringEmptyInvalidString) {
    auto values = parse_all("");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, SimpleStringNoCrlf) {
    auto values = parse_all("+");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, SimpleStringCrNoLf) {
    auto values = parse_all("+\r");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, SimpleStringEmptyValidString) {
    auto values = parse_all("+\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("", std::get<SimpleString>(values[0]).value);
}

TEST(RespParser, SimpleStringTrailingInput) {
    // trailing data isn't parsed until next feed
    auto values = parse_all("+\r\nTRAILING");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("", std::get<SimpleString>(values[0]).value);
}

TEST(RespParser, SimpleStringUnexpectedPrefix) {
    auto values = parse_all("UNEXPECTED+ERR\r\n");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, SimpleStringMultipleMessages) {
    Parser p;
    p.feed({"+OK\r\n+MORE\r\n", 12});
    auto values = p.take_values();
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ("OK", std::get<SimpleString>(values[0]).value);
    EXPECT_EQ("MORE", std::get<SimpleString>(values[1]).value);
}

TEST(RespParser, SimpleErrorEmpty) {
    auto values = parse_all("-\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("", std::get<SimpleError>(values[0]).value);
}

TEST(RespParser, SimpleErrorEmptyPrefix) {
    auto values = parse_all("- value\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("value", std::get<SimpleError>(values[0]).value);
}

TEST(RespParser, SimpleErrorEmptyValue) {
    auto values = parse_all("-PREFIX\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("PREFIX", std::get<SimpleError>(values[0]).prefix);
    EXPECT_EQ("", std::get<SimpleError>(values[0]).value);
}

TEST(RespParser, SimpleErrorHappyPath) {
    auto values = parse_all("-PREFIX value\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("PREFIX", std::get<SimpleError>(values[0]).prefix);
    EXPECT_EQ("value", std::get<SimpleError>(values[0]).value);
}

TEST(RespParser, SimpleErrorEmptyValueWithSpaceAfterPrefix) {
    auto values = parse_all("-PREFIX \r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("PREFIX", std::get<SimpleError>(values[0]).prefix);
    EXPECT_EQ("", std::get<SimpleError>(values[0]).value);
}

TEST(RespParser, SimpleErrorNoCrlf) {
    auto values = parse_all("-PREFIX");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, SimpleErrorCrNoLf) {
    auto values = parse_all("-PREFIX\r");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, IntegerNoSign) {
    auto values = parse_all(":0\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(0, std::get<Integer>(values[0]).value);
}

TEST(RespParser, IntegerWithPlus) {
    auto values = parse_all(":+1\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(1, std::get<Integer>(values[0]).value);
}

TEST(RespParser, IntegerWithMinus) {
    auto values = parse_all(":-1\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(-1, std::get<Integer>(values[0]).value);
}

TEST(RespParser, IntegerEmptyWithPlus) {
    auto values = parse_all(":+\r\n");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, IntegerEmptyWithMinus) {
    auto values = parse_all(":-\r\n");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, IntegerEmpty) {
    auto values = parse_all(":\r\n");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, BulkStringHappyPath) {
    auto values = parse_all("$5\r\nhello\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("hello", *std::get<BulkString>(values[0]).value);
}

TEST(RespParser, BulkStringNull) {
    auto values = parse_all("$-1\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(std::nullopt, std::get<BulkString>(values[0]).value);
}

TEST(RespParser, BulkStringEmpty) {
    auto values = parse_all("$0\r\n\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ("", *std::get<BulkString>(values[0]).value);
}

TEST(RespParser, BulkStringNoCrlf) {
    auto values = parse_all("$5\r\nhello");
    EXPECT_TRUE(values.empty());
}

TEST(RespParser, ArrayEmpty) {
    auto values = parse_all("*0\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_TRUE(std::get<Array>(values[0]).value->empty());
}

TEST(RespParser, ArrayNull) {
    auto values = parse_all("*-1\r\n");
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(std::nullopt, std::get<Array>(values[0]).value);
}

TEST(RespParser, ArraySimpleStrings) {
    auto values = parse_all("*2\r\n+OK\r\n+PONG\r\n");
    ASSERT_EQ(values.size(), 1u);
    Array expected{
        std::vector<Value>{
            SimpleString{"OK"},
            SimpleString{"PONG"}
        }
    };
    EXPECT_EQ(expected, std::get<Array>(values[0]));
}

TEST(RespParser, ArraySimpleErrors) {
    auto values = parse_all("*2\r\n-ERR one\r\n-ERR two\r\n");
    ASSERT_EQ(values.size(), 1u);
    Array expected{
        std::vector<Value>{
            SimpleError{"ERR", "one"},
            SimpleError{"ERR", "two"}
        }
    };
    EXPECT_EQ(expected, std::get<Array>(values[0]));
}

TEST(RespParser, ArrayIntegers) {
    auto values = parse_all("*3\r\n:1\r\n:42\r\n:-5\r\n");
    ASSERT_EQ(values.size(), 1u);
    Array expected{
        std::vector<Value>{
            Integer{1},
            Integer{42},
            Integer{-5}
        }
    };
    EXPECT_EQ(expected, std::get<Array>(values[0]));
}

TEST(RespParser, ArrayBulkStrings) {
    auto values = parse_all("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    ASSERT_EQ(values.size(), 1u);
    Array expected{
        std::vector<Value>{
            BulkString{"foo"},
            BulkString{"bar"}
        }
    };
    EXPECT_EQ(expected, std::get<Array>(values[0]));
}

TEST(RespParser, ArrayNestedArrays) {
    auto values = parse_all("*2\r\n*2\r\n+Hello\r\n+World\r\n*1\r\n:100\r\n");
    ASSERT_EQ(values.size(), 1u);
    Array expected{
        std::vector<Value>{
            Array{
                std::vector<Value>{
                    SimpleString{"Hello"},
                    SimpleString{"World"}
                }
            },
            Array{
                std::vector<Value>{
                    Integer{100}
                }
            }
        }
    };
    EXPECT_EQ(expected, std::get<Array>(values[0]));
}

TEST(RespParser, ArrayMixedVariants) {
    auto values = parse_all("*4\r\n+Hi\r\n:123\r\n$5\r\nhello\r\n-Oops\r\n");
    ASSERT_EQ(values.size(), 1u);
    Array expected{
        std::vector<Value>{
            SimpleString{"Hi"},
            Integer{123},
            BulkString{"hello"},
            SimpleError{"Oops"}
        }
    };
    EXPECT_EQ(expected, std::get<Array>(values[0]));
}

TEST(RespParser, ArrayInvalidVariant) {
    auto values = parse_all("*4\r\n+Hi\r\n:123\r\n");
    EXPECT_TRUE(values.empty());
}
