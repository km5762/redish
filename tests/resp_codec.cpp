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
