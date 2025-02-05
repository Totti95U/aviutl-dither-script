#include <lua.hpp>

struct Pixel_RGBA {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

// Main function to call from Lua
int mult_color(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    double rm = lua_tonumber(L, 4);
    double gm = lua_tonumber(L, 5);
    double bm = lua_tonumber(L, 6);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            pixels[index].r *= rm;
            pixels[index].g *= gm;
            pixels[index].b *= bm;
        }
    }
    
    return 0;
}

// Make function list
static luaL_Reg functions[] = {
    { "mult_color", mult_color },
    { nullptr, nullptr }
};

// Register function and export
extern "C" {
    // `AUL_DLL_Sample` is a call name from Lua
    // the detail is `http://milkpot.sakura.ne.jp/lua/lua51_manual_ja.html#pdf-require`
    __declspec(dllexport) int luaopen_AUL_DLL_Sample(lua_State *L) {
        luaL_register(L, "AUL_DLL_Sample", functions);
        return 1;
    }
}
