#include "resp.h"

#include <gtest/gtest.h>
#include <sstream>

TEST(RespCodecTest, SimpleStringHappyPath) {
    std::istringstream input("+OK\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("OK", std::get<resp::SimpleString>(*output).value);
}

TEST(RespCodecTest, SimpleStringEmptyInvalidString) {
    std::istringstream input("");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, SimpleStringNoCrlf) {
    std::istringstream input("+");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, SimpleStringCrNoLf) {
    std::istringstream input("+\r");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, SimpleStringEmptyValidString) {
    std::istringstream input("+\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("", std::get<resp::SimpleString>(*output).value);
}

TEST(RespCodecTest, SimpleStringTrailingInput) {
    std::istringstream input("+\r\nTRAILING");
    const auto output = resp::decode(input);
    EXPECT_EQ("", std::get<resp::SimpleString>(*output).value);
}

TEST(RespCodecTest, SimpleStringUnexpectedPrefix) {
    std::istringstream input("UNEXPECTED+ERR\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, SimpleStringMultipleMessagesNotAllowed) {
    std::istringstream input("+OK\r\n+MORE\r\n");
    const auto a = resp::decode(input);
    const auto b = resp::decode(input);
    EXPECT_EQ("OK", std::get<resp::SimpleString>(*a).value);
    EXPECT_EQ("MORE", std::get<resp::SimpleString>(*b).value);
}

TEST(RespCodecTest, SimpleErrorEmpty) {
    std::istringstream input("-\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("", value);
}

TEST(RespCodecTest, SimpleErrorEmptyPrefix) {
    std::istringstream input("- value\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ(" value", value);
}

TEST(RespCodecTest, SimpleErrorEmptyValue) {
    std::istringstream input("-PREFIX\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("PREFIX", value);
}

TEST(RespCodecTest, SimpleErrorHappyPath) {
    std::istringstream input("-PREFIX value\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("PREFIX value", value);
}

TEST(RespCodecTest, SimpleErrorEmptyValueWithSpaceAfterPrefix) {
    std::istringstream input("-PREFIX \r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("PREFIX ", value);
}

TEST(RespCodecTest, SimpleErrorNoCrlf) {
    std::istringstream input("-PREFIX");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, SimpleErrorCrNoLf) {
    std::istringstream input("-PREFIX\r");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, IntegerNoSign) {
    std::istringstream input(":0\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(0, std::get<resp::Integer>(*output).value);
}

TEST(RespCodecTest, IntegerWithPlus) {
    std::istringstream input(":+1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(1, std::get<resp::Integer>(*output).value);
}

TEST(RespCodecTest, IntegerWithMinus) {
    std::istringstream input(":-1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(-1, std::get<resp::Integer>(*output).value);
}

TEST(RespCodecTest, IntegerEmptyWithPlus) {
    std::istringstream input(":+\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, IntegerEmptyWithMinus) {
    std::istringstream input(":-\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, IntegerEmpty) {
    std::istringstream input(":\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, BulkStringHappyPath) {
    std::istringstream input("$5\r\nhello\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("hello", std::get<resp::BulkString>(*output).value.value());
}

TEST(RespCodecTest, BulkStringNull) {
    std::istringstream input("$-1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, std::get<resp::BulkString>(*output).value);
}

TEST(RespCodecTest, BulkStringEmpty) {
    std::istringstream input("$0\r\n\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("", std::get<resp::BulkString>(*output).value);
}

TEST(RespCodecTest, BulkStringNoCrlf) {
    std::istringstream input("$5\r\nhello");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(RespCodecTest, ArrayEmpty) {
    std::istringstream input("*0\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::optional{std::vector<resp::Message>{}},
              std::get<resp::Array>(*output).value);
}

TEST(RespCodecTest, ArrayNull) {
    std::istringstream input("*-1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, std::get<resp::Array>(*output).value);
}

TEST(RespCodecTest, ArraySimpleStrings) {
    std::istringstream input("*2\r\n+OK\r\n+PONG\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Message>{
            resp::SimpleString{"OK"},
            resp::SimpleString{"PONG"}
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(RespCodecTest, ArraySimpleErrors) {
    std::istringstream input("*2\r\n-ERR one\r\n-ERR two\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Message>{
            resp::SimpleError{"ERR one"},
            resp::SimpleError{"ERR two"},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(RespCodecTest, ArrayIntegers) {
    std::istringstream input("*3\r\n:1\r\n:42\r\n:-5\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Message>{
            resp::Integer{1},
            resp::Integer{42},
            resp::Integer{-5},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(RespCodecTest, ArrayBulkStrings) {
    std::istringstream input("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Message>{
            resp::BulkString{"foo"},
            resp::BulkString{"bar"},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(RespCodecTest, ArrayNestedArrays) {
    std::istringstream input("*2\r\n*2\r\n+Hello\r\n+World\r\n*1\r\n:100\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Message>{
            resp::Array{
                std::vector<resp::Message>{
                    resp::SimpleString{"Hello"},
                    resp::SimpleString{"World"},
                }
            },
            resp::Array{
                std::vector<resp::Message>{
                    resp::Integer{100},
                }
            },
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(RespCodecTest, ArrayMixedVariants) {
    std::istringstream input("*4\r\n+Hi\r\n:123\r\n$5\r\nhello\r\n-Oops\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Message>{
            resp::SimpleString{"Hi"},
            resp::Integer{123},
            resp::BulkString{"hello"},
            resp::SimpleError{"Oops"},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(RespCodecTest, ArrayInvalidVariant) {
    std::istringstream input("*4\r\n+Hi\r\n:123\r\n");
    const auto output = resp::decode(input);

    EXPECT_EQ(
        std::nullopt, output
    );
}
