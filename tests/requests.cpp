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

    std::optional<Value> handle(const Value &request) {
        requests::handle(request, m_stream, m_dictionary);
        return resp::decode(m_stream);
    }

    static resp::Array make_array(std::initializer_list<resp::Value> elements) {
        return resp::Array{std::optional<std::vector<resp::Value> >{elements}};
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

TEST_F(RequestsTest, GetNonExistent) {
    auto request = make_array({BulkString{"GET"}, BulkString{"key"}});
    const auto response = handle(request);
    EXPECT_EQ(BulkString{std::nullopt}, *std::get_if<BulkString>(&*response));
}

TEST_F(RequestsTest, GetBulkString) {
    m_dictionary.set("key", BulkString{"value"});
    auto request = make_array({BulkString{"GET"}, BulkString{"key"}});
    const auto response = handle(request);
    EXPECT_EQ(BulkString{"value"}, *std::get_if<BulkString>(&*response));
}

TEST_F(RequestsTest, GetNonBulkString) {
    m_dictionary.set("key", Integer{1});
    auto request = make_array({BulkString{"GET"}, BulkString{"key"}});
    const auto response = handle(request);
    EXPECT_EQ(SimpleError{"ERR non bulk-string value"}, *std::get_if<SimpleError>(&*response));
}

TEST_F(RequestsTest, SetNonExistent) {
    auto request = make_array({BulkString{"SET"}, BulkString{"key"}, BulkString{"value"}});
    const auto response = handle(request);
    EXPECT_EQ(SimpleString{"OK"}, *std::get_if<SimpleString>(&*response));
}

TEST_F(RequestsTest, SetUpdate) {
    m_dictionary.set("key", BulkString{"value"});
    auto request = make_array({BulkString{"SET"}, BulkString{"key"}, BulkString{"value1"}});
    const auto response = handle(request);
    EXPECT_EQ(SimpleString{"OK"}, *std::get_if<SimpleString>(&*response));
}
