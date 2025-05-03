#include "resp_codec.h"

#include <gtest/gtest.h>

class RespCodecTest : public ::testing::Test {
protected:
    RespCodec m_codec;
};

TEST_F(RespCodecTest, SimpleStringHappyPath) {
    const auto output = m_codec.decode("+OK\r\n");
    EXPECT_EQ("OK", std::get<RespCodec::SimpleString>(*output).value);
}

TEST_F(RespCodecTest, SimpleStringEmptyInvalidString) {
    const auto output = m_codec.decode("");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, SimpleStringNoCrlf) {
    const auto output = m_codec.decode("+");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, SimpleStringCrNoLf) {
    const auto output = m_codec.decode("+\r");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, SimpleStringEmptyValidString) {
    const auto output = m_codec.decode("+\r\n");
    EXPECT_EQ("", std::get<RespCodec::SimpleString>(*output).value);
}

TEST_F(RespCodecTest, SimpleStringTrailingInput) {
    const auto output = m_codec.decode("+\r\nTRAILING");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, SimpleStringUnexpectedPrefix) {
    const auto output = m_codec.decode("UNEXPECTED+ERR\r\n");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, SimpleStringMultipleMessagesNotAllowed) {
    const auto output = m_codec.decode("+OK\r\n+MORE\r\n");
    EXPECT_EQ(std::nullopt, output);
}


TEST_F(RespCodecTest, SimpleErrorEmpty) {
    const auto output = m_codec.decode("-\r\n");
    const auto [prefix, value] = std::get<RespCodec::SimpleError>(*output);
    EXPECT_EQ("", prefix);
    EXPECT_EQ("", value);
}

TEST_F(RespCodecTest, SimpleErrorEmptyPrefix) {
    const auto output = m_codec.decode("- value\r\n");
    const auto [prefix, value] = std::get<RespCodec::SimpleError>(*output);
    EXPECT_EQ("", prefix);
    EXPECT_EQ("value", value);
}

TEST_F(RespCodecTest, SimpleErrorEmptyValue) {
    const auto output = m_codec.decode("-PREFIX\r\n");
    const auto [prefix, value] = std::get<RespCodec::SimpleError>(*output);
    EXPECT_EQ("PREFIX", prefix);
    EXPECT_EQ("", value);
}

TEST_F(RespCodecTest, SimpleErrorHappyPath) {
    const auto output = m_codec.decode("-PREFIX value\r\n");
    const auto [prefix, value] = std::get<RespCodec::SimpleError>(*output);
    EXPECT_EQ("PREFIX", prefix);
    EXPECT_EQ("value", value);
}

TEST_F(RespCodecTest, SimpleErrorEmptyValueWithSpaceAfterPrefix) {
    const auto output = m_codec.decode("-PREFIX \r\n");
    const auto [prefix, value] = std::get<RespCodec::SimpleError>(*output);
    EXPECT_EQ("PREFIX", prefix);
    EXPECT_EQ("", value);
}

TEST_F(RespCodecTest, SimpleErrorNoCrlf) {
    const auto output = m_codec.decode("-PREFIX");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, SimpleErrorCrNoLf) {
    const auto output = m_codec.decode("-PREFIX\r");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, IntegerNoSign) {
    const auto output = m_codec.decode(":0\r\n");
    EXPECT_EQ(0, std::get<RespCodec::Integer>(*output).value);
}

TEST_F(RespCodecTest, IntegerWithPlus) {
    const auto output = m_codec.decode(":+1\r\n");
    EXPECT_EQ(1, std::get<RespCodec::Integer>(*output).value);
}

TEST_F(RespCodecTest, IntegerWithMinus) {
    const auto output = m_codec.decode(":-1\r\n");
    EXPECT_EQ(-1, std::get<RespCodec::Integer>(*output).value);
}

TEST_F(RespCodecTest, IntegerEmptyWithPlus) {
    const auto output = m_codec.decode(":+\r\n");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, IntegerEmptyWithMinus) {
    const auto output = m_codec.decode(":-\r\n");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, IntegerEmpty) {
    const auto output = m_codec.decode(":\r\n");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, IntegerNonNumeric) {
    const auto output = m_codec.decode(":+123ABC\r\n");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, BulkStringHappyPath) {
    const auto output = m_codec.decode("$5\r\nhello\r\n");
    EXPECT_EQ("hello", std::get<RespCodec::BulkString>(*output).value.value());
}

TEST_F(RespCodecTest, BulkStringNull) {
    const auto output = m_codec.decode("$-1\r\n");
    EXPECT_EQ(std::nullopt, std::get<RespCodec::BulkString>(*output).value);
}

TEST_F(RespCodecTest, BulkStringEmpty) {
    const auto output = m_codec.decode("$0\r\n\r\n");
    EXPECT_EQ("", std::get<RespCodec::BulkString>(*output).value);
}

TEST_F(RespCodecTest, BulkStringNoCrlf) {
    const auto output = m_codec.decode("$5\r\nhello");
    EXPECT_EQ(std::nullopt, output);
}
