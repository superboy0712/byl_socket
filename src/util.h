//
// Created by yulong on 3/29/17.
//

#ifndef BYLSOCKET_UTIL_H
#define BYLSOCKET_UTIL_H

#include <exception>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <assert.h>

namespace bylSocket {

#define err_report_and_throw(s) \
    do { fprintf(stderr, "%s: %s (%s) in %s at line %d\n", __func__, s, \
            strerror(errno), __FILE__, __LINE__);\
         throw std::logic_error(strerror(errno));\
    } while(0)

#define err_report(s) \
    do { fprintf(stderr, "%s: %s (%s) in %s at line %d\n", __func__, s, \
            strerror(errno), __FILE__, __LINE__);\
    } while(0)

#define assert_n_throw(expr) \
    do { if(!(expr)) err_report_and_throw(#expr); } while(0)


/**
 * try at most n times for a timeout blocking call
 *  (like timeout blocking socket)
 * @param n times
 * @param f
 * @param args
 */
template<typename Functor, typename ... Args>
void tryForMax(int n, Functor f, Args ... args) {
    assert_n_throw(n > 0);
    for (int fail_count = 0;;) {
        try {
            f(args...);
            break;
        }
        catch (std::exception &e) {
            if (++fail_count >= n)
                throw;
        }
    }
}
/**
 * try maximum n times with specified interval in secs
 * for a non-blocking call
 * @param n try times
 * @param sec seconds in float
 * @param f
 * @param args
 */
template<typename Functor, typename ... Args>
void tryForMaxInterval(int n, float sec, Functor f, Args ... args) {
    assert_n_throw(n > 0 && sec > 0);
    for (int fail_count = 0;;) {
        try {
            f(args...);
            break;
        }
        catch (std::exception &e) {
            if (++fail_count >= n)
                throw;
            std::this_thread::sleep_for(
                    std::chrono::milliseconds((int) (1000 * sec))
            );
        }
    }
}


template<typename Functor, typename ... Args>
void tryfor_interval_not_throw(const char *str,
                               int lvl,
                               int n,
                               float sec,
                               Functor f,
                               Args ... args) {
    assert_n_throw(n > 0 && sec > 0);
    for (int fail_count = 0;;) {
        try {
            fprintf(stderr,
                    "%*c%s:%d-st try\n",
                    lvl * 4,
                    ' ',
                    str,
                    fail_count + 1);
            f(args...);
            break;
        }
        catch (...) {
            fprintf(stderr,
                    "%*c %s:%d-st try failed\n",
                    lvl * 4,
                    ' ',
                    str,
                    fail_count + 1);
            if (++fail_count >= n)
                break;
            std::this_thread::sleep_for(
                    std::chrono::milliseconds((int) (1000 * sec))
            );
        }
    }
}

/**
 * usually used for outtermost level
 * @param str
 * @param lvl
 * @param n
 * @param sec
 * @param f
 * @param args
 */
template<typename Functor, typename ... Args>
void tryforever_interval_not_throw(const char *str,
                                   int lvl,
                                   float sec,
                                   Functor f,
                                   Args ... args) {
    assert_n_throw(sec > 0);
    for (int fail_count = 0;; ++fail_count) {
        try {
            fprintf(stderr,
                    "%*c%s:%d-st try\n",
                    lvl * 4,
                    ' ',
                    str,
                    fail_count + 1);
            f(args...);
            break;
        }
        catch (std::exception &e) {
            fprintf(stderr,
                    "%*c %s:%d-st try failed\n",
                    lvl * 4,
                    ' ',
                    str,
                    fail_count + 1);
            err_report(e.what());
            std::this_thread::sleep_for(
                    std::chrono::milliseconds((int) (1000 * sec))
            );
        }
    }
}

}
#endif //BYLSOCKET_UTIL_H
