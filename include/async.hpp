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
        inline void series(
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
        inline void series(
            LastHandler const& last_handler,
            FirstHandler const& first_handler,
            RestHandlers const& ... rest_handlers
        ) {
            first_handler([&] (error_type error) -> void {
                if (error)
                    last_handler(error);
                else
                    series(last_handler, rest_handlers...);
            });
        }

    }

    template<
        typename ... Handlers
    >
    inline void series(
        Handlers const& ... handlers
    ) {
        std::tuple<Handlers const& ...> handler_tuple (handlers ...);
        auto& last_handler = std::get<sizeof...(Handlers)-1>(handler_tuple);
        detail::series(last_handler, handlers...);
    }




    namespace detail
    {

        template<
            typename LastHandler
        >
        inline void param_series(
            LastHandler const& last_handler,
            std::tuple<> const&,
            LastHandler const&
        ) {
            last_handler(nullptr);
        }

        template<
            typename LastHandler
        >
        inline void param_series_impl(
            LastHandler const& last_handler,
            std::tuple<> const&,
            std::index_sequence<>,
            LastHandler const&
        ) {
            last_handler(nullptr);
        }

        template<
            typename LastHandler,
            typename ... InArgs,
            typename FirstHandler,
            typename ... RestHandlers
        >
        inline void param_series(
            LastHandler const& last_handler,
            std::tuple<InArgs&&...> const& in_args,
            FirstHandler const& first_handler,
            RestHandlers const& ... rest_handlers
        );
        
        template<
            typename LastHandler,
            typename ... InArgs,
            std::size_t ... Is,
            typename FirstHandler,
            typename ... RestHandlers
        >
        inline void param_series_impl(
            LastHandler const& last_handler,
            std::tuple<InArgs&&...> const& in_args,
            std::index_sequence<Is...>,
            FirstHandler const& first_handler,
            RestHandlers const& ... rest_handlers
        ) {
            first_handler(std::get<Is>(in_args)..., [&] (error_type error) -> void {
                if (error)
                    last_handler(error);
                else
                    param_series(last_handler, std::tuple<>{}, rest_handlers...);
            });
        }

        template<
            typename LastHandler,
            typename ... InArgs,
            typename FirstHandler,
            typename ... RestHandlers
        >
        inline void param_series(
            LastHandler const& last_handler,
            std::tuple<InArgs&&...> const& in_args,
            FirstHandler const& first_handler,
            RestHandlers const& ... rest_handlers
        ) {
            param_series_impl(
                last_handler,
                in_args,
                std::index_sequence_for<InArgs...>{},
                rest_handlers...
            );
        }

    }

    template<
        typename ... Handlers
    >
    inline void param_series(
        Handlers const& ... handlers
    ) {
        std::tuple<Handlers const& ...> handler_tuple (handlers ...);
        auto& last_handler = std::get<sizeof...(Handlers)-1>(handler_tuple);
        detail::param_series(last_handler, std::tuple<>{}, handlers...);
    }




#if false
    namespace detail 
    {

        template<typename F>
        struct obj_series_entry
        {
            error_type error;
            F const& f;
            constexpr obj_series_entry(F const& f) : error(nullptr), f(f) {}
            template<typename F2>
            constexpr decltype(auto) operator& (obj_series_entry<F2> const& rhs) const {
                return *this;
            }
        };

    }

    template<typename ... Functions>
    inline void obj_series(
        Functions const& ... functions
    ) {
        (... & detail::obj_series_entry(functions));
    }
#endif


    namespace detail
    {

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
        inline void obj_series(
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
        inline void obj_series(
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
                    obj_series<N+1>(function_tuple, std::tuple<std::tuple_element_t<OutArgIs, out_tuple_t>...>{args...});
                }
            );
        }

        template<
            int N,
            typename ... Functions,
            typename ... InArgs
        >
        inline void obj_series(
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
                obj_series<N,traits>(
                    function_tuple, 
                    in_args, 
                    std::make_index_sequence<sizeof...(InArgs)>(),
                    std::make_index_sequence<traits::arity-1>()
                );
            }
        }

    }

    template<typename ... Functions>
    inline void obj_series(
        Functions ... functions
    ) {
        std::tuple<Functions...> function_tuple(functions...);
        detail::obj_series<0>(function_tuple, std::tuple<>{});
    }


}