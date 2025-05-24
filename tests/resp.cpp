//
// Created by d4wgr on 5/21/2025.
//

#include "resp.h"

#include <gtest/gtest.h>


using namespace resp;

namespace {
    template<typename T>
    void test_save_load(const T &original) {
        std::stringstream ss;
        save(original, ss);
        ss.seekg(0);
        const auto result = load(ss);
        const T &loaded = std::get<T>(result);
        EXPECT_EQ(original, loaded);
    }
}

TEST(Resp, SaveLoadSimpleString) {
    test_save_load(SimpleString{"value"});
}

TEST(Resp, SaveLoadSimpleError) {
    test_save_load(SimpleError{"prefix", "value"});
}

TEST(Resp, SaveLoadInteger) {
    test_save_load(Integer{1});
}

TEST(Resp, SaveLoadNullBulkString) {
    test_save_load(BulkString{std::nullopt});
}

TEST(Resp, SaveLoadBulkString) {
    test_save_load(BulkString{"value"});
}

TEST(Resp, SaveLoadArray) {
    const Array array{
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
    test_save_load(array);
}
