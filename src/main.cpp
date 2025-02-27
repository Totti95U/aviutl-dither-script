#include <lua.hpp>
#include <random>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(x) (min(max(x, 0), 255))
#define mixed(x, y) clamp(x * mix + y * (1.0f - mix))

using namespace std;

struct Pixel_RGBA {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

std::uniform_real_distribution<float> U_RAND(0.0f, 1.0f);

/*==================== Error diffusion NOISY dithering ====================*/

// Noise selecting function
void noises(std::default_random_engine& mt, float* channel, int noise_type) {
    float tmp = U_RAND(mt);
    switch (noise_type) {
        // Uniformly on [0, 1)
        // All channels are same
        case 1:
            channel[0] = tmp;
            channel[1] = tmp;
            channel[2] = tmp;
            break;
        // Uniformly on [0.5, 1)
        // All channels are distinct
        case 2:
            channel[0] = U_RAND(mt);
            channel[1] = U_RAND(mt);
            channel[2] = U_RAND(mt);
            break;
        // TODO: implement using blue noise
        case 3:
            return;
        // TODO: implement using bayer matrix
        case 4:
            return;
        // constant
        default:
            channel[0] = 0.5f;
            channel[1] = 0.5f;
            channel[2] = 0.5f;
            break;
    }

    return;
}

void diffuse_at(float* errors, float* error, int w, int h, int x, int y, float ratio) {
    if (x < 0 || x >= w || y < 0 || y >= h) {
        return;
    } else {
        errors[0 + 3 * (x + w * y)] += error[0] * ratio;
        errors[1 + 3 * (x + w * y)] += error[1] * ratio;
        errors[2 + 3 * (x + w * y)] += error[2] * ratio;
        return;
    }
    return;
}

void diffuse_error(float* errors, float* error, int x, int y, int w, int h, int diffusion_type){
    switch (diffusion_type) {
    // Floyd-Steinberg
    case 1:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 7.0f / 16.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 3.0f / 16.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 5.0f / 16.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 1.0f / 16.0f);
        break;
    // False Floyd-Steinberg
    case 2:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 3.0f / 8.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 3.0f / 8.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 2.0f / 8.0f);
        break;
    // Jarvis, Judice, and Ninke
    case 3:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 7.0f / 48.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 5.0f / 48.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 3.0f / 48.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 5.0f / 48.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 7.0f / 48.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 5.0f / 48.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 3.0f / 48.0f);
        // x - 2, y + 2
        diffuse_at(errors, error, w, h, x - 2, y + 2, 1.0f / 48.0f);
        // x - 1, y + 2
        diffuse_at(errors, error, w, h, x - 1, y + 2, 3.0f / 48.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 5.0f / 48.0f);
        // x + 1, y + 2
        diffuse_at(errors, error, w, h, x + 1, y + 2, 3.0f / 48.0f);
        // x + 2, y + 2
        diffuse_at(errors, error, w, h, x + 2, y + 2, 1.0f / 48.0f);
        break;
    // no diffusion
    default:
        break;
    }

    return;
}

/*------------------- Uniformly fixed palette dithering -------------------*/
// Uniformly distributed color palette version
int noisy_error_diffusion_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float mix = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    int noise_type = static_cast<int>(lua_tointeger(L, 7));
    int diffusion_type = static_cast<int>(lua_tointeger(L, 8));

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }

    std::default_random_engine mt(seed);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 225.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 225.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 225.0f + errors[2 + 3 * index];

            float noise[3] = { 0.5f, 0.5f, 0.5f };
            noises(mt, noise, noise_type);

            // channel r
            int new_r = 0;
            int r_num = std::floor(r * c_num);
            if (r * c_num - r_num < noise[0]) {
                new_r = r_num * 0xff / c_num;
            } else {
                new_r = (r_num + 1) * 0xff / c_num;
            }
            error[0] = r - new_r / 225.0f;

            // channel g
            int new_g = 0;
            int g_num = std::floor(g * c_num);
            if (g * c_num - g_num < noise[1]) {
                new_g = g_num * 0xff / c_num;
            } else {
                new_g = (g_num + 1) * 0xff / c_num;
            }
            error[1] = g - new_g / 225.0f;

            // channel b
            int new_b = 0;
            int b_num = std::floor(b * c_num);
            if (b * c_num - b_num < noise[2]) {
                new_b = b_num * 0xff / c_num;
            } else {
                new_b = (b_num + 1) * 0xff / c_num;
            }
            error[2] = b - new_b / 225.0f;

            pixels[index].r = mixed(new_r, pixels[index].r);
            pixels[index].g = mixed(new_g, pixels[index].g);
            pixels[index].b = mixed(new_b, pixels[index].b);

            // error diffusion
            diffuse_error(errors, error, x, y, w, h, diffusion_type);
        }
    }

    free(errors);
    return 0;
}

// Make function list
static luaL_Reg functions[] = {
    { "uniform_palette_dither", noisy_error_diffusion_dither },
    { nullptr, nullptr }
};

// Register function and export
extern "C" {
    // `AUL_DLL_Sample` is a call name from Lua
    // the detail is `http://milkpot.sakura.ne.jp/lua/lua51_manual_ja.html#pdf-require`
    __declspec(dllexport) int luaopen_DITHERINGS(lua_State *L) {
        luaL_register(L, "DITHERINGS", functions);
        return 1;
    }
}
