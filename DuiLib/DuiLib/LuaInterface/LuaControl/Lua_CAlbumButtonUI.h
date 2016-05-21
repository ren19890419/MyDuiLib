#ifndef _Lua_CAlbumButtonUI_h
#define _Lua_CAlbumButtonUI_h
#pragma once


namespace DuiLib
{
	LUA_CLASS(CAlbumButtonUI)
	{
	public:
		
		LUA_METHOD_DECL(New)
		LUA_METHOD_DECL(GetClassName)
		LUA_METHOD_DECL(SetHotBkImage)
		LUA_METHOD_DECL(GetHotBkImage)
		LUA_METHOD_DECL(SetAlbumImage)

		LUA_CLASS_REG_DECL()
	};
}


#endif//_Lua_CAlbumButtonUI_h