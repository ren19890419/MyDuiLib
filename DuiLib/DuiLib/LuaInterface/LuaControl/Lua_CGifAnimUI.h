#ifndef _Lua_CGifAnimUI_h
#define _Lua_CGifAnimUI_h
#pragma once


namespace DuiLib
{
	LUA_CLASS(CGifAnimUI)
	{
	public:
		
		LUA_METHOD_DECL(New)
		LUA_METHOD_DECL(GetClassName)
		LUA_METHOD_DECL(GetGifImage)
		LUA_METHOD_DECL(SetGifImage)

		LUA_CLASS_REG_DECL()
	};
}


#endif//_Lua_CGifAnimUI_h