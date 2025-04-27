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

