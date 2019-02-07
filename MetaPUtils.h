#pragma once

#include <type_traits>
#include <tuple>

namespace AsmBuilder::MetaPUtils
{
	// Source: https://stackoverflow.com/a/22968432
	template<typename... Args>
	struct pack { };

	// Source: https://stackoverflow.com/a/7943765
	template <typename T>
	struct function_traits
		: public function_traits<decltype(&T::operator())>
	{};
	// For generic types, directly use the result of the signature of its 'operator()'

	template <typename ClassType, typename ReturnType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) const>
		// we specialize for pointers to member function
	{
		// Result property
		using result_type = ReturnType;

		// Arg properties
		static constexpr std::size_t number_of_args = sizeof...(Args);

		// All args
		static constexpr pack<Args...> args = {};
		
		// Accessor for a specific arg
		template <size_t i>
		struct arg
		{
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
			// the i-th argument is equivalent to the i-th tuple element of a tuple
			// composed of those arguments.
		};
	};
}



