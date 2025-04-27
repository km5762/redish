#include "resp_codec.h"

#include <gtest/gtest.h>

class RespCodecTest : public ::testing::Test {
protected:
    RespCodec m_codec;
};

TEST_F(RespCodecTest, HappyPath) {
    const auto output = m_codec.decode("+OK\r\n");
    EXPECT_EQ("OK", std::get<RespCodec::SimpleString>(*output).value);
}

TEST_F(RespCodecTest, EmptyInvalidString) {
    const auto output = m_codec.decode("");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, NoCrlf) {
    const auto output = m_codec.decode("+");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, CrNoLf) {
    const auto output = m_codec.decode("+\r");
    EXPECT_EQ(std::nullopt, output);
}

TEST_F(RespCodecTest, EmptyValidString) {
    const auto output = m_codec.decode("+\r\n");
    EXPECT_EQ("", std::get<RespCodec::SimpleString>(*output).value);
}

TEST_F(RespCodecTest, TrailingInput) {
    const auto output = m_codec.decode("+\r\nTEST");
    EXPECT_EQ(std::nullopt, output);
}
