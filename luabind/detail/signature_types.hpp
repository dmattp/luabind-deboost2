// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_SIGNATURE_TYPES_HPP
# define LUABIND_SIGNATURE_TYPES_HPP

#include <luabind/config.hpp>
#include <luabind/lua_include.hpp>
#include <luabind/typeid.hpp>
#include <luabind/pseudo_traits.hpp>
#include <luabind/detail/meta.hpp>
#include <luabind/detail/type_info.hpp>
#include <vector>

namespace luabind {
	namespace adl {

		class object;
		class argument;
		template <class Base>
		struct table;

	} // namespace adl

	using adl::object;
	using adl::argument;
	using adl::table;

	namespace detail {

		template <class T, class Enable = void>
		struct get_type_info
		{
			static void get(lua_State* L,TypeInfo &outTypeInfo)
			{
				outTypeInfo.typeInfo = &typeid(T);
				class_registry* r = class_registry::get_registry(L);
				outTypeInfo.crep = r->find_class(*outTypeInfo.typeInfo);
			}
		};

		template <class T>
		struct get_type_info<T*>
		{
			static void get(lua_State* L,TypeInfo &outTypeInfo)
			{
				get_type_info<T>::get(L,outTypeInfo);
				outTypeInfo.isPointer = true;
			}
		};

		template <class T>
		struct get_type_info<T&>
		{
			static void get(lua_State* L,TypeInfo &outTypeInfo)
			{
				get_type_info<T>::get(L,outTypeInfo);
				outTypeInfo.isReference = true;
			}
		};

		template <class T>
		struct get_type_info<T const>
		{
			static void get(lua_State* L,TypeInfo &outTypeInfo)
			{
				get_type_info<T>::get(L,outTypeInfo);
				outTypeInfo.isConst = true;
			}
		};

# define LUABIND_TYPE_TO_TYPEINFO(x,typeId) \
    template <> \
    struct get_type_info<x> \
    { \
        static void get(lua_State* L,TypeInfo &outTypeInfo) \
        { \
			outTypeInfo.type = typeId; \
        } \
    };

	LUABIND_TYPE_TO_TYPEINFO(char,TypeInfo::FundamentalType::Char)
	LUABIND_TYPE_TO_TYPEINFO(unsigned char,TypeInfo::FundamentalType::UChar)
	LUABIND_TYPE_TO_TYPEINFO(short,TypeInfo::FundamentalType::Short)
	LUABIND_TYPE_TO_TYPEINFO(unsigned short,TypeInfo::FundamentalType::UShort)
	LUABIND_TYPE_TO_TYPEINFO(int,TypeInfo::FundamentalType::Int)
	LUABIND_TYPE_TO_TYPEINFO(unsigned int,TypeInfo::FundamentalType::UInt)
	LUABIND_TYPE_TO_TYPEINFO(long,TypeInfo::FundamentalType::Long)
	LUABIND_TYPE_TO_TYPEINFO(unsigned long,TypeInfo::FundamentalType::ULong)
	LUABIND_TYPE_TO_TYPEINFO(void,TypeInfo::FundamentalType::Void)
	LUABIND_TYPE_TO_TYPEINFO(bool,TypeInfo::FundamentalType::Bool)
	LUABIND_TYPE_TO_TYPEINFO(std::string,TypeInfo::FundamentalType::String)
	LUABIND_TYPE_TO_TYPEINFO(lua_State,TypeInfo::FundamentalType::LuaState)
	LUABIND_TYPE_TO_TYPEINFO(luabind::object,TypeInfo::FundamentalType::LuabindObject)
	LUABIND_TYPE_TO_TYPEINFO(luabind::argument,TypeInfo::FundamentalType::LuabindArgument)

	// See https://stackoverflow.com/a/28796458/2482983
	template<typename Test, template<typename...> class Ref>
		struct is_specialization : std::false_type {};
	template<template<typename...> class Ref, typename... Args>
		struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

		template <class Base>
		struct get_type_info<table<Base> >
		{
			static void get(lua_State* L,TypeInfo &outTypeInfo)
			{
				outTypeInfo.type = TypeInfo::FundamentalType::Table;
			}
		};

		inline void get_type_aux(lua_State*, bool, meta::type_list< >,std::vector<TypeInfo> &outTypes)
		{}

		template <typename T> struct get_template_parameter_type_info_impl;
			template <template <typename...> typename C, typename... Args>
				struct get_template_parameter_type_info_impl<C<Args...>> {
					static auto call(lua_State* L,std::vector<TypeInfo> &outTypes);
					static constexpr size_t size() {return sizeof...(Args);}
				};

		template <class T, class... Ts>
		void get_template_parameter_type_info(lua_State *L,std::vector<TypeInfo> &outTypes) {
			outTypes.push_back({});
			auto &typeInfo = outTypes.back();
			get_type_info<T>::get(L,typeInfo);
			typeInfo.isPseudoType = true;

			if constexpr(pseudo_traits<T>::is_pseudo_type)
			{
				if constexpr(pseudo_traits<T>::is_variadic)
				{
					constexpr auto n = get_template_parameter_type_info_impl<T>::size();
					typeInfo.templateTypes.reserve(n);
					get_template_parameter_type_info_impl<T>::call(L,typeInfo.templateTypes);
				}
				else
				{
					TypeInfo templateType {};
					get_type_info<pseudo_traits<T>::value_type>::get(L,templateType);
					typeInfo.templateTypes.push_back(templateType);
				}
			}

			if constexpr (sizeof...(Ts) > 0)
				get_template_parameter_type_info<Ts...>(L,outTypes);
		}

		template <class Signature>
		void get_type_aux(lua_State* L, bool first, Signature,std::vector<TypeInfo> &outTypes)
		{
			TypeInfo typeInfo;
			using T = typename meta::front<Signature>::type;
			if constexpr(pointer_traits<T>::is_pointer)
			{
				get_type_info<pointer_traits<T>::value_type>::get(L,typeInfo);
				typeInfo.isSmartPtr = true;
			}
			else if constexpr(pseudo_traits<T>::is_pseudo_type)
				get_template_parameter_type_info_impl<T>::call(L,typeInfo.templateTypes);
			else
				get_type_info<T>::get(L,typeInfo);
			outTypes.push_back(std::move(typeInfo));
			get_type_aux(L, false, typename meta::pop_front<Signature>::type(),outTypes);
		}

		template <class Signature>
		void get_type(lua_State* L, char const* function, Signature,std::vector<TypeInfo> &outTypes)
		{
			using first = typename meta::front<Signature>::type;

			TypeInfo typeInfo;
			if constexpr(pointer_traits<first>::is_pointer)
			{
				get_type_info<pointer_traits<first>::value_type>::get(L,typeInfo);
				typeInfo.isSmartPtr = true;
			}
			else if constexpr(pseudo_traits<first>::is_pseudo_type)
				get_template_parameter_type_info_impl<first>::call(L,typeInfo.templateTypes);
			else
				get_type_info<first>::get(L,typeInfo);
			outTypes.push_back(std::move(typeInfo));

			get_type_aux(
				L
				, true
				, typename meta::pop_front<Signature>::type(),
				outTypes
			);
		}

	} // namespace detail

} // namespace luabind

template <template <typename...> typename C, typename... Args>
	auto luabind::detail::get_template_parameter_type_info_impl<C<Args...>>::call(lua_State* L,std::vector<TypeInfo> &outTypes) {
		return get_template_parameter_type_info<Args...>(L,outTypes);
	}

#endif // LUABIND_FORMAT_SIGNATURE_081014_HPP

