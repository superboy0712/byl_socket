//
// Created by yulong on 3/7/17.
//
#include <gtest/gtest.h>
#include "../src/bylSocket.hpp"
using namespace std;
using namespace bylSocket;
TEST(cantest, first) {
    EXPECT_ANY_THROW(assert_n_throw(false));
    EXPECT_ANY_THROW(for (int fail_cnt = 0;;) {
        try {
            throw 3;
        } catch (const std::exception &e) {
            if (++fail_cnt > 2)
                throw;
        }
    });
    EXPECT_ANY_THROW(tryForMax(3, []() {
        throw "hello";
    }));

    EXPECT_ANY_THROW(tryForMaxInterval(3, 1, []() {
        throw "lalal";
    }));

    EXPECT_NO_THROW(tryfor_interval_not_throw("not throw", 1, 3, 0.001, []() {
        throw "lalal";
    }));
}
//