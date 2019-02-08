#pragma once

#include <functional>
#include "MetaPUtils.h"

#include "fixed_size_function.h"

// Source: http://videocortex.io/2016/lambdas-callbacks/
// TODO: Create LambdaWrapper class so operator= can be used.

namespace AsmBuilder::LambdaPayloadInjector
{
	enum class CallingConvention {
		StdCall,
		CDeclCall,
		ThisCall,
		FastCall
	};

	template<CallingConvention CallConv, typename Ret, typename... Args>
	struct CallingConvType;

	template<typename Ret, typename... Args>
	struct CallingConvType<CallingConvention::StdCall, Ret, Args...> {
		using Type = Ret(__stdcall*)(Args...);
	};

	template<typename Ret, typename... Args>
	struct CallingConvType<CallingConvention::CDeclCall, Ret, Args...> {
		using Type = Ret(__cdecl*)(Args...);
	};

	template<typename Ret, typename... Args>
	struct CallingConvType<CallingConvention::ThisCall, Ret, Args...> {
		using Type = Ret(__thiscall*)(Args...);
	};

	template<typename Ret, typename... Args>
	struct CallingConvType<CallingConvention::FastCall, Ret, Args...> {
		using Type = Ret(__fastcall*)(Args...);
	};


	namespace detail
	{
		template <typename LambdaTag, typename StdFuncTag>
		struct payload_injector
		{
			static StdFuncTag val;
		};
		
		template <typename LambdaTag, typename StdFuncTag>
		StdFuncTag payload_injector<LambdaTag, StdFuncTag>::val; // static variable definition for linker
	

		// CallingConvention Helper
		template<CallingConvention Convention>
		struct CallConventionDispatchHelper;

		template<>
		struct CallConventionDispatchHelper<CallingConvention::ThisCall> 
		{
			template<typename PayloadInjectorType, typename Ret, typename... Args>
			static Ret __thiscall Dispatch(Args... args) {
				return PayloadInjectorType::val(args...);
			}
		};

		template<>
		struct CallConventionDispatchHelper<CallingConvention::CDeclCall>
		{
			template<typename PayloadInjectorType, typename Ret, typename... Args>
			static Ret __cdecl Dispatch(Args... args) {
				return PayloadInjectorType::val(args...);
			}
		};

		template<>
		struct CallConventionDispatchHelper<CallingConvention::StdCall>
		{
			template<typename PayloadInjectorType, typename Ret, typename... Args>
			static Ret __stdcall Dispatch(Args... args) {
				return PayloadInjectorType::val(args...);
			}
		};

		template<>
		struct CallConventionDispatchHelper<CallingConvention::FastCall>
		{
			template<typename PayloadInjectorType, typename Ret, typename... Args>
			static Ret __fastcall Dispatch(Args... args) {
				return PayloadInjectorType::val(args...);
			}
		};


		template<CallingConvention Convention, typename Ret,  typename... Args, typename LambdaFunc>
		auto CreateFuncPtrFromLambdaHelper(LambdaFunc func, AsmBuilder::MetaPUtils::pack<Args...>)
		{
			// we add sizeof(void*) * 3 because of the vtable used in fixed_size_function
			using StdFunctionType = fixed_size_function<Ret(Args...), sizeof(LambdaFunc) + sizeof(void*) * 3, construct_type::move>;
			using LambdaType = std::decay_t<LambdaFunc>;
			using PayloadInjectorType = payload_injector<LambdaType, StdFunctionType>;
			PayloadInjectorType::val = StdFunctionType(func);

			return &CallConventionDispatchHelper<Convention>::template Dispatch<PayloadInjectorType, Ret, Args...>;
		}
	}
	
	
	

	template<CallingConvention Convention = CallingConvention::StdCall, typename LambdaFunc>
	auto CreateFuncPtrFromLambda(LambdaFunc func)
	{
		using lambda_traits_t = AsmBuilder::MetaPUtils::function_traits<LambdaFunc>;
		
		return detail::CreateFuncPtrFromLambdaHelper<Convention, lambda_traits_t::result_type>(func, lambda_traits_t::args);
	}
}



