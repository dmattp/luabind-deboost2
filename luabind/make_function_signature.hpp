// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_MAKE_FUNCTION_SIGNATURE_HPP
#define LUABIND_MAKE_FUNCTION_SIGNATURE_HPP

#include <luabind/make_function.hpp>
#include <luabind/detail/signature_types.hpp>

template <class F, class Signature, class InjectorList>
	void luabind::detail::function_object_impl<F,Signature,InjectorList>::get_signature_info(lua_State* L, char const* function, std::vector<TypeInfo> &outTypes) const
	{
		detail::get_type(L, function, Signature(), outTypes);
	}

#endif // LUABIND_MAKE_FUNCTION_081014_HPP
