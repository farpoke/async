
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
namespace asio = boost::asio;

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/async.hpp"



class expected_exception : public std::exception {
    std::string msg;
public:
    expected_exception() : msg("") {}
    expected_exception(std::string_view msg) : msg(msg) {}
    virtual const char* what() const noexcept override { return msg.c_str(); }
};



template<int ThreadCount>
class AsioFixture
{
public:
    std::atomic_bool stop;
    asio::io_service ios;
    std::vector<std::thread> threads;

    AsioFixture() {
        INFO("Starting AsioFixture threads...");
        stop = false;
        for (int i = 0; i < ThreadCount; i++)
            threads.emplace_back([this](){
                while (!stop)
                    ios.run();
            });
    }

    ~AsioFixture() {
        INFO("Stopping AsioFixture threads...");
        stop = true;
        ios.stop();
        for (auto& t : threads)
            t.join();
    }
};



TEST_CASE("Non-concurrent async::simple_series", "[simple_series]") {

    bool first_called = false, second_called = false, 
        third_called = false, last_called = false;
    async::error_type error = nullptr;

    SECTION("No error") {

        async::simple_series(
            [&] (auto next) {
                first_called = true;
                next(nullptr);
            },
            [&] (auto next) {
                second_called = true;
                next(nullptr);
            },
            [&] (auto next) {
                third_called = true;
                next(nullptr);
            },
            [&] (auto err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == true);
        CHECK(last_called == true);
        CHECK(error == nullptr);

    }
    
    SECTION("With error") {

        async::simple_series(
            [&] (auto next) {
                first_called = true;
                next(nullptr);
            },
            [&] (auto next) {
                second_called = true;
                next(std::make_exception_ptr(expected_exception()));
            },
            [&] (auto next) {
                third_called = true;
                next(nullptr);
            },
            [&] (auto err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == false);
        CHECK(last_called == true);
        CHECK(error != nullptr);
        CHECK_THROWS_AS(std::rethrow_exception(error), expected_exception);

    }
    
    SECTION("With exceptions") {

        async::simple_series(
            [&] (auto next) {
                first_called = true;
                next(nullptr);
            },
            [&] (auto next) {
                second_called = true;
                throw expected_exception("async::simple_series");
                next(nullptr);
            },
            [&] (auto next) {
                third_called = true;
                next(nullptr);
            },
            [&] (auto err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == false);
        CHECK(last_called == true);
        CHECK(error != nullptr);
        CHECK_THROWS_AS(std::rethrow_exception(error), expected_exception);

    }

}



TEST_CASE("Non-concurrent async::series", "[series]") {

    bool first_called = false, second_called = false, 
        third_called = false, last_called = false;
    async::error_type error = nullptr;

    SECTION("No error") {

        async::series(
            [&] (async::callback<> next) {
                first_called = true;
                next(nullptr);
            },
            [&] (async::callback<> next) {
                second_called = true;
                next(nullptr);
            },
            [&] (async::callback<> next) {
                third_called = true;
                next(nullptr);
            },
            [&] (async::error_type err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == true);
        CHECK(last_called == true);
        CHECK(error == nullptr);

    }
    
    SECTION("With error") {

        async::series(
            [&] (async::callback<> next) {
                first_called = true;
                next(nullptr);
            },
            [&] (async::callback<> next) {
                second_called = true;
                next(std::make_exception_ptr(expected_exception()));
            },
            [&] (async::callback<> next) {
                third_called = true;
                next(nullptr);
            },
            [&] (async::error_type err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == false);
        CHECK(last_called == true);
        CHECK(error != nullptr);
        CHECK_THROWS_AS(std::rethrow_exception(error), expected_exception);

    }
    
    SECTION("With exception") {

        async::series(
            [&] (async::callback<> next) {
                first_called = true;
                next(nullptr);
            },
            [&] (async::callback<> next) {
                second_called = true;
                throw expected_exception("async::series");
                next(nullptr);
            },
            [&] (async::callback<> next) {
                third_called = true;
                next(nullptr);
            },
            [&] (async::error_type err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == false);
        CHECK(last_called == true);
        CHECK(error != nullptr);
        CHECK_THROWS_AS(std::rethrow_exception(error), expected_exception);

    }

    SECTION("With parameters") {

        int a=0, b=0, c=0;

        async::series(
            [&] (async::callback<int> next) {
                first_called = true;
                next(nullptr, 1);
            },
            [&] (int x, async::callback<int,int> next) {
                second_called = true;
                a = x;
                next(nullptr, 2, 3);
            },
            [&] (int x1, int x2, async::callback<> next) {
                third_called = true;
                b = x1;
                c = x2;
                next(nullptr);
            },
            [&] (async::error_type err) {
                last_called = true;
                error = err;
            }
        );

        CHECK(first_called == true);
        CHECK(second_called == true);
        CHECK(third_called == true);
        CHECK(last_called == true);
        CHECK(error == nullptr);

        CHECK(a == 1);
        CHECK(b == 2);
        CHECK(c == 3);

    }

}



TEST_CASE_METHOD(AsioFixture<2>, "Concurrent async::simple_series", "[simple_series]") {

    std::promise<void> promise;
    auto future = promise.get_future();
    bool first_called = false, second_called = false, third_called = false;

    async::simple_series(
        [&] (auto next) {
            first_called = true;
            asio::steady_timer timer(ios);
            timer.expires_from_now(std::chrono::milliseconds(10));
            timer.async_wait([&](auto ec){
                if (ec)
                    next(std::make_exception_ptr(boost::system::system_error(ec)));
                else
                    next(nullptr);
            });
        },
        [&] (auto next) {
            second_called = true;
            asio::steady_timer timer(ios);
            timer.expires_from_now(std::chrono::milliseconds(10));
            timer.async_wait([&](auto ec){
                if (ec)
                    next(std::make_exception_ptr(boost::system::system_error(ec)));
                else
                    next(nullptr);
            });
        },
        [&] (auto next) {
            third_called = true;
            asio::steady_timer timer(ios);
            timer.expires_from_now(std::chrono::milliseconds(10));
            timer.async_wait([&](auto ec){
                if (ec)
                    next(std::make_exception_ptr(boost::system::system_error(ec)));
                else
                    next(nullptr);
            });
        },
        [&] (auto ec) {
            promise.set_value();
        }
    );

    future.get();

    CHECK(first_called);
    CHECK(second_called);
    CHECK(third_called);

}
