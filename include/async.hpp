#pragma once

#include <functional>
#include <tuple>
#include <utility>

namespace async
{

    using error_type = std::exception_ptr;

    template<typename ... Args>
    using callback = std::function<void(error_type, Args...)>;

    namespace detail
    {
        
        template<
            typename LastHandler
        >
        inline void simple_series(
            LastHandler const& last_handler,
            LastHandler const&
        ) {
            last_handler(nullptr);
        }
        
        template<
            typename LastHandler,
            typename FirstHandler,
            typename ... RestHandlers
        >
        inline void simple_series(
            LastHandler const& last_handler,
            FirstHandler const& first_handler,
            RestHandlers const& ... rest_handlers
        ) {
            first_handler([&] (error_type error) -> void {
                if (error)
                    last_handler(error);
                else
                simple_series(last_handler, rest_handlers...);
            });
        }

        template<typename T>
        struct function_traits 
        : public function_traits<decltype(&T::operator())>
        {};

        template<typename F, typename ... Args>
        struct function_traits<void(F::*)(Args...) const>
        {
            static constexpr size_t arity = sizeof...(Args);
            typedef std::tuple<Args...> argument_tuple;
        };

        template<
            int N,
            typename ... Functions,
            typename ... InArgs
        >
        inline void series(
            std::tuple<Functions...> const& function_tuple,
            std::tuple<InArgs...> in_args
        );

        template<
            int N,
            typename NextFunctionTraits,
            typename ... Functions,
            typename ... InArgs,
            size_t ... InArgIs,
            size_t ... OutArgIs
        >
        inline void series(
            std::tuple<Functions...> const& function_tuple,
            std::tuple<InArgs...> in_args,
            std::index_sequence<InArgIs...>,
            std::index_sequence<OutArgIs...>
        ) {
            using out_tuple_t = typename NextFunctionTraits::argument_tuple;
            auto function = std::get<N>(function_tuple);
            function(
                std::get<InArgIs>(in_args)..., 
                [&](error_type error, std::tuple_element_t<OutArgIs, out_tuple_t>... args) {
                    if (error)
                        std::get<sizeof...(Functions)-1>(function_tuple)(error);
                    else
                        series<N+1>(function_tuple, std::tuple<std::tuple_element_t<OutArgIs, out_tuple_t>...>{args...});
                }
            );
        }

        template<
            int N,
            typename ... Functions,
            typename ... InArgs
        >
        inline void series(
            std::tuple<Functions...> const& function_tuple,
            std::tuple<InArgs...> in_args
        ) {
            if constexpr (N == sizeof...(Functions)-1) {
                auto function = std::get<N>(function_tuple);
                using fun_arg_tuple_t = typename function_traits<decltype(function)>::argument_tuple;
                static_assert(std::is_same_v<decltype(in_args), std::tuple<>>);
                static_assert(std::is_same_v<fun_arg_tuple_t, std::tuple<error_type>>);
                function(nullptr);
            }
            else {
                auto next_function = std::get<N+1>(function_tuple);
                using traits = function_traits<decltype(next_function)>;
                series<N,traits>(
                    function_tuple, 
                    in_args, 
                    std::make_index_sequence<sizeof...(InArgs)>(),
                    std::make_index_sequence<traits::arity-1>()
                );
            }
        }

    }

    template<
        typename ... Handlers
    >
    inline void simple_series(
        Handlers const& ... handlers
    ) {
        std::tuple<Handlers const& ...> handler_tuple (handlers ...);
        auto& last_handler = std::get<sizeof...(Handlers)-1>(handler_tuple);
        detail::simple_series(last_handler, handlers...);
    }

    template<typename ... Functions>
    inline void series(
        Functions ... functions
    ) {
        std::tuple<Functions...> function_tuple(functions...);
        detail::series<0>(function_tuple, std::tuple<>{});
    }

}
