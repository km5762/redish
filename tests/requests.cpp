//
// Created by d4wgr on 5/8/2025.
//

#include "requests.h"

#include <gtest/gtest.h>

#include "dictionary.h"


using namespace resp;

class RequestsTest : public ::testing::Test {
protected:
    Dictionary m_dictionary{};
    std::stringstream m_stream{};

    std::optional<Message> handle(const Message &request) {
        requests::handle(request, m_stream, m_dictionary);
        return resp::decode(m_stream);
    }

    static resp::Array make_array(std::initializer_list<resp::Message> elements) {
        return resp::Array{std::optional<std::vector<resp::Message> >{elements}};
    }
};

TEST_F(RequestsTest, Ping) {
    auto request = make_array({BulkString{"PING"}});
    const auto response = handle(request);
    EXPECT_EQ(SimpleString{"PONG"}, *std::get_if<SimpleString>(&*response));
}

TEST_F(RequestsTest, PingArg) {
    auto request = make_array({BulkString{"PING"}, BulkString{"arg"}});
    const auto response = handle(request);
    EXPECT_EQ(BulkString{"arg"}, *std::get_if<BulkString>(&*response));
}
