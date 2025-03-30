#include <lua.hpp>
#include <random>
#include <unistd.h>
#include <math.h>
#include <algorithm>
#include <iostream>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(x) (min(max(x, 0), 255))
#define mixed(x, y) clamp(x * mix + y * (1.0f - mix))

using namespace std;

/*==================== Pixel structure ====================*/
struct Pixel_RGB {
    unsigned char r, g, b;
};

Pixel_RGB* read_txr(const char* path, int* w, int* h) {
    // open texture file
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("Failed to open texture file");
        return nullptr;
    }

    // read texture size
    unsigned short int tmp_w, tmp_h;
    fread(&tmp_w, sizeof(unsigned short int), 1, file);
    fread(&tmp_h, sizeof(unsigned short int), 1, file);
    *w = tmp_w;
    *h = tmp_h;
    // read texture data
    Pixel_RGB *texture = (Pixel_RGB*)malloc(sizeof(Pixel_RGB) * *w * *h);
    if (!texture) {
        perror("Failed to allocate memory for texture");
        fclose(file);
        return nullptr;
    }
    fread(texture, sizeof(Pixel_RGB), *w * *h, file);
    fclose(file);
    return texture;
}
struct Pixel_RGBA {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
public:
    operator Pixel_RGB() {
        Pixel_RGB pixel;
        pixel.r = r;
        pixel.g = g;
        pixel.b = b;
        return pixel;
    }
};

struct LinearRGB {
    float r;
    float g;
    float b;
};

// float toLinear(float c) {
//     if (c <= 0.04045f) {
//         return c / 12.92f;
//     } else {
//         return pow((c + 0.055f) / 1.055f, 2.4f);
//     }
// }
// we use lock-up table to speed up the conversion
const float toLinear[256] = {0.0f, 0.0009836768f, 0.0011481879f, 0.0013277208f, 0.0015226438f, 0.0017333125f, 0.0019600706f, 0.0022032515f, 0.0024631785f, 0.0027401655f, 0.0030345186f, 0.0033465358f, 0.0036765074f, 0.004024717f, 0.004391442f, 0.0047769533f, 0.0051815165f, 0.0056053917f, 0.006048833f, 0.0065120906f, 0.00699541f, 0.007499032f, 0.008023193f, 0.008568126f, 0.009134059f, 0.009721218f, 0.010329823f, 0.010960094f, 0.011612245f, 0.012286488f, 0.0129830325f, 0.013702083f, 0.014443844f, 0.015208514f, 0.015996294f, 0.016807375f, 0.017641954f, 0.01850022f, 0.019382361f, 0.020288562f, 0.02121901f, 0.022173885f, 0.023153367f, 0.024157632f, 0.02518686f, 0.026241222f, 0.027320892f, 0.02842604f, 0.029556835f, 0.030713445f, 0.031896032f, 0.033104766f, 0.034339808f, 0.035601314f, 0.03688945f, 0.038204372f, 0.039546236f, 0.0409152f, 0.04231141f, 0.04373503f, 0.045186203f, 0.046665087f, 0.048171826f, 0.049706567f, 0.051269457f, 0.052860647f, 0.054480277f, 0.05612849f, 0.05780543f, 0.059511237f, 0.061246052f, 0.063010015f, 0.064803265f, 0.06662594f, 0.06847817f, 0.070360094f, 0.07227185f, 0.07421357f, 0.07618538f, 0.07818742f, 0.08021982f, 0.08228271f, 0.08437621f, 0.08650046f, 0.08865558f, 0.09084171f, 0.093058966f, 0.09530747f, 0.09758735f, 0.099898726f, 0.10224173f, 0.104616486f, 0.107023105f, 0.10946171f, 0.11193243f, 0.114435375f, 0.116970666f, 0.11953843f, 0.122138776f, 0.12477182f, 0.12743768f, 0.13013647f, 0.13286832f, 0.13563333f, 0.13843161f, 0.14126329f, 0.14412847f, 0.14702727f, 0.14995979f, 0.15292615f, 0.15592647f, 0.15896083f, 0.16202937f, 0.1651322f, 0.1682694f, 0.17144111f, 0.1746474f, 0.17788842f, 0.18116425f, 0.18447499f, 0.18782078f, 0.19120169f, 0.19461784f, 0.19806932f, 0.20155625f, 0.20507874f, 0.20863687f, 0.21223076f, 0.2158605f, 0.2195262f, 0.22322796f, 0.22696587f, 0.23074006f, 0.23455058f, 0.23839757f, 0.24228112f, 0.24620132f, 0.25015828f, 0.2541521f, 0.25818285f, 0.26225066f, 0.2663556f, 0.2704978f, 0.2746773f, 0.27889428f, 0.28314874f, 0.28744084f, 0.29177064f, 0.29613826f, 0.30054379f, 0.3049873f, 0.30946892f, 0.31398872f, 0.31854677f, 0.3231432f, 0.3277781f, 0.33245152f, 0.33716363f, 0.34191442f, 0.34670407f, 0.3515326f, 0.35640013f, 0.3613068f, 0.3662526f, 0.3712377f, 0.37626213f, 0.38132602f, 0.38642943f, 0.39157248f, 0.39675522f, 0.40197778f, 0.4072402f, 0.4125426f, 0.41788507f, 0.42326766f, 0.4286905f, 0.43415365f, 0.43965718f, 0.4452012f, 0.4507858f, 0.45641103f, 0.462077f, 0.4677838f, 0.47353148f, 0.47932017f, 0.48514995f, 0.49102086f, 0.49693298f, 0.5028865f, 0.50888133f, 0.5149177f, 0.52099556f, 0.5271151f, 0.5332764f, 0.5394795f, 0.54572445f, 0.55201143f, 0.5583404f, 0.5647115f, 0.57112485f, 0.57758045f, 0.58407843f, 0.59061885f, 0.59720176f, 0.60382736f, 0.61049557f, 0.6172066f, 0.6239604f, 0.63075715f, 0.63759685f, 0.6444797f, 0.65140563f, 0.65837485f, 0.6653873f, 0.67244315f, 0.6795425f, 0.6866853f, 0.69387174f, 0.7011019f, 0.70837575f, 0.7156935f, 0.7230551f, 0.73046076f, 0.7379104f, 0.7454042f, 0.7529422f, 0.7605245f, 0.76815116f, 0.7758222f, 0.7835378f, 0.7912979f, 0.7991027f, 0.80695224f, 0.8148466f, 0.82278574f, 0.8307699f, 0.838799f, 0.8468732f, 0.8549926f, 0.8631572f, 0.8713671f, 0.8796224f, 0.8879231f, 0.8962694f, 0.9046612f, 0.91309863f, 0.92158186f, 0.9301109f, 0.9386857f, 0.9473065f, 0.9559733f, 0.9646863f, 0.9734453f, 0.9822506f, 0.9911021f, 1.0f};

LinearRGB rgb2linearRGB(Pixel_RGB pixel) {
    LinearRGB result;
    // result.r = toLinear[pixel.r];
    // result.g = toLinear[pixel.g];
    // result.b = toLinear[pixel.b];
    result.r = pixel.r / 255.0f;
    result.g = pixel.g / 255.0f;
    result.b = pixel.b / 255.0f;
    return result;
}

unsigned char toStandard(float c) {
    // binary search for the inverse of toLinear
    int low = 0;
    int high = 255;
    while (low < high) {
        int mid = (low + high) / 2;
        if (toLinear[mid] < c) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    if (low == 0) {
        return 0;
    } else if (low == 255) {
        return 255;
    } else {
        float diff1 = fabsf(toLinear[low] - c);
        float diff2 = fabsf(toLinear[low - 1] - c);
        if (diff1 < diff2) {
            return low;
        } else {
            return low - 1;
        }
    }
}

Pixel_RGB linearRGB2rgb(LinearRGB pixel) {
    Pixel_RGB result;
    // result.r = toStandard(pixel.r);
    // result.g = toStandard(pixel.g);
    // result.b = toStandard(pixel.b);
    result.r = pixel.r * 0xff;
    result.g = pixel.g * 0xff;
    result.b = pixel.b * 0xff;
    return result;
}

struct Oklab {
    float l;
    float a;
    float b;
};

Oklab linearRGB2oklab(LinearRGB pixel) {
    Oklab result;
    float l = 0.4122214708f * pixel.r + 0.5363325363f * pixel.g + 0.0514459929f * pixel.b;
    float m = 0.2119034982f * pixel.r + 0.6806995451f * pixel.g + 0.1073969566f * pixel.b;
    float s = 0.0883024619f * pixel.r + 0.2817188376f * pixel.g + 0.6299787005f * pixel.b;

    float l_ = std::cbrt(l);
    float m_ = std::cbrt(m);
    float s_ = std::cbrt(s);
    // float l_ = l;
    // float m_ = m;
    // float s_ = s;

    result.l = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    result.a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    result.b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;

    return result;
}

LinearRGB oklab2linearRGB(Oklab pixel) {
    LinearRGB result;
    float l_ = pixel.l + 0.3963377774f * pixel.a + 0.2158037573f * pixel.b;
    float m_ = pixel.l - 0.1055613458f * pixel.a - 0.0638541728f * pixel.b;
    float s_ = pixel.l - 0.0894841775f * pixel.a - 1.2914855480f * pixel.b;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    result.r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    result.g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    result.b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

    return result;
}

Oklab rgb2oklab(Pixel_RGB pixel) {
    LinearRGB linear_pixel = rgb2linearRGB(pixel);
    return linearRGB2oklab(linear_pixel);
}

Pixel_RGB oklab2rgb(Oklab pixel) {
    LinearRGB linear_pixel = oklab2linearRGB(pixel);
    return linearRGB2rgb(linear_pixel);
}

Pixel_RGBA oklab2rgba(Oklab pixel, unsigned char alpha = 255) {
    Pixel_RGBA result;
    Pixel_RGB rgb_pixel = oklab2rgb(pixel);
    result.r = rgb_pixel.r;
    result.g = rgb_pixel.g;
    result.b = rgb_pixel.b;
    result.a = alpha;
    return result;
}

Oklab* rgba_to_oklab(Pixel_RGBA* pixels, int s) {
    Oklab* oklab_pixels = (Oklab*)malloc(sizeof(Oklab) * s);

    for (int i = 0; i < s; i++) {
        oklab_pixels[i] = rgb2oklab(pixels[i]);
    }

    return oklab_pixels;
}

Oklab* rgb_to_oklab(Pixel_RGB* pixels, int s) {
    Oklab* oklab_pixels = (Oklab*)malloc(sizeof(Oklab) * s);
    for (int i = 0; i < s; i++) {
        oklab_pixels[i] = rgb2oklab(pixels[i]);
    }
    return oklab_pixels;
}

Pixel_RGB* to_rgb(Oklab* pixels, int s) {
    Pixel_RGB* rgb_pixels = (Pixel_RGB*)malloc(sizeof(Pixel_RGB) * s);
    for (int i = 0; i < s; i++) {
        rgb_pixels[i] = oklab2rgb(pixels[i]);
    }
    return rgb_pixels;
}

/*==================== Random number generator ====================*/

std::uniform_real_distribution<float> U_RAND(-0.5f, 0.5f);
std::uniform_int_distribution<int> channel_RAND(0, 2);
std::uniform_int_distribution<int> comb_RAND(0, 5);
std::uniform_int_distribution<int> I_RAND(0, 2048 * 2048 - 1);

int USE_CHANNEL = 3;
int USE_COMB = 0;

/*==================== Error diffusion NOISY dithering ====================*/

// Noise selecting function
void noises(std::default_random_engine& mt, float* channel, int noise_type, Pixel_RGB* texture, int* t_index) {
    float tmp = U_RAND(mt);

    if (noise_type == 1){
        // Uniformly on [-0.5, 0.5)
        // White noise (all channel use same value)
        channel[0] = tmp;
        channel[1] = tmp;
        channel[2] = tmp;
        return;
    } else if (noise_type == 2) {
        // Uniformly on [-0.5, 0.5)
        // White noise (all channel use different value)
        channel[0] = U_RAND(mt);
        channel[1] = U_RAND(mt);
        channel[2] = U_RAND(mt);
        return;
    } else if (noise_type == 3 || noise_type == 5) {
        // use texture (all channel use same value)
        float threshold = 0.0f;
        if (USE_CHANNEL == 0) {
            threshold = texture[t_index[0]].r;
        } else if (USE_CHANNEL == 1) {
            threshold = texture[t_index[0]].g;
        } else {
            threshold = texture[t_index[0]].b;
        }
        channel[0] = threshold / 255.0f - 0.5f;
        channel[1] = threshold / 255.0f - 0.5f;
        channel[2] = threshold / 255.0f - 0.5f;
        return;
    } else if (noise_type == 4 || noise_type == 6) {
        // use texture (all channel use different value)
        float thresholds[3];
        if (USE_COMB == 0) {
            thresholds[0] = texture[t_index[0]].r;
            thresholds[1] = texture[t_index[1]].g;
            thresholds[2] = texture[t_index[2]].b;
        } else if (USE_COMB == 1) {
            thresholds[0] = texture[t_index[0]].r;
            thresholds[1] = texture[t_index[1]].b;
            thresholds[2] = texture[t_index[2]].g;
        } else if (USE_COMB == 2) {
            thresholds[0] = texture[t_index[0]].g;
            thresholds[1] = texture[t_index[1]].r;
            thresholds[2] = texture[t_index[2]].b;
        } else if (USE_COMB == 3) {
            thresholds[0] = texture[t_index[0]].g;
            thresholds[1] = texture[t_index[1]].b;
            thresholds[2] = texture[t_index[2]].r;
        } else if (USE_COMB == 4) {
            thresholds[0] = texture[t_index[0]].b;
            thresholds[1] = texture[t_index[1]].r;
            thresholds[2] = texture[t_index[2]].g;
        } else {
            thresholds[0] = texture[t_index[0]].b;
            thresholds[1] = texture[t_index[1]].g;
            thresholds[2] = texture[t_index[2]].r;
        }
        channel[0] = thresholds[0] / 255.0f - 0.5f;
        channel[1] = thresholds[1] / 255.0f - 0.5f;
        channel[2] = thresholds[2] / 255.0f - 0.5f;
        return;
    } else {
        // constant
        channel[0] = 0.0f;
        channel[1] = 0.0f;
        channel[2] = 0.0f;
        return;
    }
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
    case 4:
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
    case 9:
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
    // Stucki
    case 8:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 8.0f / 42.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 4.0f / 42.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 2.0f / 42.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 4.0f / 42.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 8.0f / 42.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 4.0f / 42.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 2.0f / 42.0f);
        // x - 2, y + 2
        diffuse_at(errors, error, w, h, x - 2, y + 2, 1.0f / 42.0f);
        // x - 1, y + 2
        diffuse_at(errors, error, w, h, x - 1, y + 2, 2.0f / 42.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 4.0f / 42.0f);
        // x + 1, y + 2
        diffuse_at(errors, error, w, h, x + 1, y + 2, 2.0f / 42.0f);
        // x + 2, y + 2
        diffuse_at(errors, error, w, h, x + 2, y + 2, 1.0f / 42.0f);
        break;
    // Atkinson
    case 3:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 1.0f / 8.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 1.0f / 8.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 1.0f / 8.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 1.0f / 8.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 1.0f / 8.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 1.0f / 8.0f);
        break;
    // Burkes
    case 6:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 8.0f / 32.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 4.0f / 32.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 2.0f / 32.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 4.0f / 32.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 8.0f / 32.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 4.0f / 32.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 2.0f / 32.0f);
        break;
    // Sierra
    case 7:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 5.0f / 32.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 3.0f / 32.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 2.0f / 32.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 4.0f / 32.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 5.0f / 32.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 4.0f / 32.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 2.0f / 32.0f);
        // x - 1, y + 2
        diffuse_at(errors, error, w, h, x - 1, y + 2, 2.0f / 32.0f);
        // x, y + 2
        diffuse_at(errors, error, w, h, x, y + 2, 3.0f / 32.0f);
        // x + 1, y + 2
        diffuse_at(errors, error, w, h, x + 1, y + 2, 2.0f / 32.0f);
        break;
    // Two-row Sierra
    case 5:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 4.0f / 16.0f);
        // x + 2, y
        diffuse_at(errors, error, w, h, x + 2, y, 3.0f / 16.0f);
        // x - 2, y + 1
        diffuse_at(errors, error, w, h, x - 2, y + 1, 1.0f / 16.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 2.0f / 16.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 3.0f / 16.0f);
        // x + 1, y + 1
        diffuse_at(errors, error, w, h, x + 1, y + 1, 2.0f / 16.0f);
        // x + 2, y + 1
        diffuse_at(errors, error, w, h, x + 2, y + 1, 1.0f / 16.0f);
        break;
    // Sierra Lite
    case 1:
        // x + 1, y
        diffuse_at(errors, error, w, h, x + 1, y, 2.0f / 4.0f);
        // x - 1, y + 1
        diffuse_at(errors, error, w, h, x - 1, y + 1, 1.0f / 4.0f);
        // x, y + 1
        diffuse_at(errors, error, w, h, x, y + 1, 1.0f / 4.0f);
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

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    int noise_type = static_cast<int>(lua_tointeger(L, 7));
    int diffusion_type = static_cast<int>(lua_tointeger(L, 8));

    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 9));
    int t_w, t_h;
    Pixel_RGB *texture = nullptr;
    if (noise_type >= 3) {
        texture = read_txr(texture_path, &t_w, &t_h);
        if (!texture || t_w * t_h == 0) {
            perror("Failed to open texture file");
            return 0;
        }
    }

    float* errors;
    errors = (float*)calloc(sizeof(float),  3 * w * h);

    std::default_random_engine mt(seed);
    USE_CHANNEL = channel_RAND(mt);
    USE_COMB = comb_RAND(mt);
    int t_offset[3] = { 0, 0, 0 };
    if (noise_type >= 3) {
        t_offset[0] = I_RAND(mt) % (t_w * t_h);
        t_offset[1] = I_RAND(mt) % (t_w * t_h);
        t_offset[2] = I_RAND(mt) % (t_w * t_h);
    }
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            int t_index[3] = { 0, 0, 0 };
            if (noise_type >= 3) {
                t_index[0] = (x % t_w + t_w * (y % t_h) + t_offset[0]) % (t_w * t_h);
                t_index[1] = (x % t_w + t_w * (y % t_h) + t_offset[1]) % (t_w * t_h);
                t_index[2] = (x % t_w + t_w * (y % t_h) + t_offset[2]) % (t_w * t_h);
            }
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 255.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 255.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 255.0f + errors[2 + 3 * index];
            float r_int = std::floor(r * c_num);
            float g_int = std::floor(g * c_num);
            float b_int = std::floor(b * c_num);

            float noise[3] = { 0.0f, 0.0f, 0.0f };
            noises(mt, noise, noise_type, texture, t_index);

            // channel r
            if (fmodf(r * c_num, 1.0f) < noise_amp * noise[0] + 0.5f) {
                pixels[index].r = clamp(0xff * r_int / c_num);
            } else {
                pixels[index].r = clamp(0xff * (r_int + 1.0f) / c_num);
            }
            error[0] = r - pixels[index].r / 255.0f;

            // channel g
            if (fmodf(g * c_num, 1.0f) < noise_amp * noise[1] + 0.5f) {
                pixels[index].g = clamp(0xff * g_int / c_num);
            } else {
                pixels[index].g = clamp(0xff * (g_int + 1.0f) / c_num);
            }
            error[1] = g - pixels[index].g / 255.0f;

            // channel b (rgb)
            if (fmodf(b * c_num, 1.0f) < noise_amp * noise[2] + 0.5f) {
                pixels[index].b = 0xff * b_int / c_num;
            } else {
                pixels[index].b = 0xff * (b_int + 1.0f) / c_num;
            }
            error[2] = b - pixels[index].b / 255.0f;

            // error diffusion
            diffuse_error(errors, error, x, y, w, h, diffusion_type);
        }
    }

    free(errors);
    free(texture);
    return 0;
}

// Custom texture dithering (uniformly distributed color palette version)
int custom_texture_diffusion_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int diffusion_type = static_cast<int>(lua_tointeger(L, 6));

    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 7));
    int t_w, t_h;
    Pixel_RGB *texture = nullptr;
    texture = read_txr(texture_path, &t_w, &t_h);
    if (!texture || t_w * t_h == 0) {
        perror("Failed to open texture file");
        return 0;
    }

    float t_x = static_cast<float>(lua_tonumber(L, 8));
    float t_y = static_cast<float>(lua_tonumber(L, 9));
    float t_scale = static_cast<float>(lua_tonumber(L, 10));
    float center_x = static_cast<float>(lua_tonumber(L, 11));
    float center_y = static_cast<float>(lua_tonumber(L, 12));
    int t_loop = static_cast<int>(lua_tointeger(L, 13));

    if (t_scale == 0.0f) {
        perror("Invalid scale");
        return 0;
    }

    int center_disp = static_cast<int>(lua_tointeger(L, 14));

    float* errors = (float*)calloc(sizeof(float), 3 * w * h);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float scaled_x = (x - t_x - w/2.0f) / t_scale - center_x + t_w/2.0f;
            float scaled_y = (y - t_y - h/2.0f) / t_scale - center_y + t_h/2.0f;
            int t_index = 0;
            bool use_texture = false;
            if (t_loop == 1) {
                use_texture = true;
            } else if (0 <= scaled_x && scaled_x < t_w && 0 <= scaled_y && scaled_y < t_h) {
                use_texture = true;
            }
            
            if (use_texture) {
                int tmp_y = (static_cast<int>(scaled_y) % t_h + t_h) % t_h;
                t_index = (static_cast<int>(scaled_x) % t_w + t_w) % t_w + tmp_y * t_w;
            }

            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 255.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 255.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 255.0f + errors[2 + 3 * index];
            float r_int = std::floor(r * c_num);
            float g_int = std::floor(g * c_num);
            float b_int = std::floor(b * c_num);

            // テクスチャ範囲外の場合は四捨五入ののち誤差伝播
            if (!use_texture) {
                pixels[index].r = fmodf(r * c_num, 1.0f) < 0.5f ? clamp(0xff * r_int / c_num) : clamp(0xff * (r_int + 1) / c_num);
                error[0] = r - pixels[index].r / 255.0f;

                pixels[index].g = fmodf(g * c_num, 1.0f) < 0.5f ? clamp(0xff * g_int / c_num) : clamp(0xff * (g_int + 1) / c_num);
                error[1] = g - pixels[index].g / 255.0f;

                pixels[index].b = fmodf(b * c_num, 1.0f) < 0.5f ? clamp(0xff * b_int / c_num) : 0xff * (b_int + 1) / c_num;
                error[2] = b - pixels[index].b / 255.0f;

                diffuse_error(errors, error, x, y, w, h, diffusion_type);
                continue;
            }

            // テクスチャ範囲内の場合はテクスチャを使ってディザリング
            // channel r
            if (fmodf(r * c_num, 1.0f) < noise_amp * texture[t_index].r / 255.0f) {
                pixels[index].r = clamp(0xff * r_int / c_num);
            } else {
                pixels[index].r = clamp(0xff * (r_int + 1) / c_num);
            }
            error[0] = r - pixels[index].r / 255.0f;

            // channel g
            if (fmodf(g * c_num, 1.0f) < noise_amp * texture[t_index].g / 255.0f) {
                pixels[index].g = clamp(0xff * g_int / c_num);
            } else {
                pixels[index].g = clamp(0xff * (g_int + 1) / c_num);
            }
            error[1] = g - pixels[index].g / 255.0f;

            // channel b
            if (fmodf(b * c_num, 1.0f) < noise_amp * texture[t_index].b / 255.0f) {
                pixels[index].b = clamp(0xff * b_int / c_num);
            } else {
                pixels[index].b = clamp(0xff * (b_int + 1) / c_num);
            }
            error[2] = b - pixels[index].b / 255.0f;

            // error diffusion
            diffuse_error(errors, error, x, y, w, h, diffusion_type);
        }
    }

    if (center_disp == 1) {
        const int radius = 2;
        for (int y = -radius; y <= radius; y++){
            for (int x = -radius; x <= radius; x++){
                // 円の外はスキップ
                if (x*x + y*y > radius*radius){
                    continue;
                }

                float moved_x = x + w/2 + t_x;
                float moved_y = y + h/2 + t_y;
                if (moved_x < 0 || w < moved_x || moved_y < 0 || h < moved_y){
                    continue;
                }
                int index = static_cast<int>(moved_x) + w * static_cast<int>(moved_y);
                pixels[index].r = 0;
                pixels[index].g = 0xff;
                pixels[index].b = 0xff;
            }
        }
    }

    free(errors);
    free(texture);
    return 1;
}

/*------------------- A palette dithering -------------------*/

float rgb_luma(Pixel_RGB color) {
    // CCIR 601 luminosity
    return (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
}

float rgb_luma(LinearRGB color) {
    // CCIR 601 luminosity
    return 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
}

// Pixel_RGB findClosestColor(const Pixel_RGB* palette, int palette_size, const Pixel_RGB& color){
//     Pixel_RGB closest_color = {0, 0, 0};
Oklab findClosestColor(const Oklab* palette, int palette_size, const Oklab& color) {
    Oklab closest_color = {0, 0, 0};
    float min_dist = 1e4;

    // TODO: use kd-tree
    for (int i = 0; i < palette_size; ++i) {
        // weighted luma rgb-distance
        // float luma1 = rgb_luma(palette[i]);
        // float luma2 = rgb_luma(color);
        // float luma_diff = luma1 - luma2;
        // float r_diff = (palette[i].r - color.r) / 255.0f;
        // float g_diff = (palette[i].g - color.g) / 255.0f;
        // float b_diff = (palette[i].b - color.b) / 255.0f;
        // float dist = (r_diff * r_diff * 0.299 + g_diff * g_diff * 0.587 + b_diff * b_diff * 0.114) * 0.75f + luma_diff * luma_diff;
        // CIE76 color distance
        float dist = 
            (palette[i].l - color.l) * (palette[i].l - color.l) +
            (palette[i].a - color.a) * (palette[i].a - color.a) +
            (palette[i].b - color.b) * (palette[i].b - color.b);
        if (dist < min_dist) {
            min_dist = dist;
            closest_color = palette[i];
        }
    }

    return closest_color;
}

LinearRGB findClosestColor(const LinearRGB *palette, int palette_size, const LinearRGB &color){
    LinearRGB closest_color = {0, 0, 0};
    float min_dist = 1e4;

    for (int i = 0; i < palette_size; ++i) {
        // weighted luma rgb-distance
        float luma1 = rgb_luma(palette[i]);
        float luma2 = rgb_luma(color);
        float luma_diff = luma1 - luma2;
        float r_diff = (palette[i].r - color.r) / 255.0f;
        float g_diff = (palette[i].g - color.g) / 255.0f;
        float b_diff = (palette[i].b - color.b) / 255.0f;
        float dist = (r_diff * r_diff * 0.299 + g_diff * g_diff * 0.587 + b_diff * b_diff * 0.114) * 0.75f + luma_diff * luma_diff;
        if (dist < min_dist) {
            min_dist = dist;
            closest_color = palette[i];
        }
    }

    return closest_color;
}

// arbitrary color palette version
int oklab_noisy_error_diffusion_custom_palette_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));
    // LinearRGB *linear_pixels = (LinearRGB*)malloc(sizeof(LinearRGB) * w * h);
    // Oklab *oklab_pixels = rgba_to_oklab(pixels, w * h);

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    
    // array containing palette data is at index 5
    int palette_size = static_cast<int>(lua_tointeger(L, 6));
    Oklab *palette = (Oklab*)malloc(sizeof(Oklab) * palette_size);
    for (int i = 0; i < palette_size; i++) {
        lua_rawgeti(L, 5, i + 1);
        int val = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        Pixel_RGB tmp;
        tmp.r = (val >> 16) & 0xff;
        tmp.g = (val >> 8) & 0xff;
        tmp.b = val & 0xff;
        palette[i] = rgb2oklab(tmp);
        // debug: display palette
        // for (int x = 0; x < 16; x++) {
        //     for (int y = 0; y < 16; y++) {
        //         int index = x + i * 16 + w * y;
        //         pixels[index].r = tmp.r;
        //         pixels[index].g = tmp.g;
        //         pixels[index].b = tmp.b;
        //     }
        // }
    }
    int candSize = static_cast<int>(lua_tointeger(L, 7));

    int seed = static_cast<int>(lua_tointeger(L, 8));
    int noise_type = static_cast<int>(lua_tointeger(L, 9));
    int diffusion_type = static_cast<int>(lua_tointeger(L, 10));

    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 11));
    int t_w, t_h;
    Pixel_RGB *texture = nullptr;
    if (noise_type >= 3) {
        texture = read_txr(texture_path, &t_w, &t_h);
        if (!texture || t_w * t_h == 0) {
            perror("Failed to open texture file");
            return 0;
        }
    }

    float* errors;
    errors = (float*)calloc(sizeof(float),  3 * w * h);

    std::default_random_engine mt(seed);
    USE_CHANNEL = channel_RAND(mt);
    USE_COMB = comb_RAND(mt);
    int t_offset[3] = { 0, 0, 0 };
    if (noise_type >= 3) {
        t_offset[0] = I_RAND(mt) % (t_w * t_h);
        t_offset[1] = I_RAND(mt) % (t_w * t_h);
        t_offset[2] = I_RAND(mt) % (t_w * t_h);
    }
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            int t_index[3] = { 0, 0, 0 };
            if (noise_type >= 3) {
                t_index[0] = (x % t_w + t_w * (y % t_h) + t_offset[0]) % (t_w * t_h);
                t_index[1] = (x % t_w + t_w * (y % t_h) + t_offset[1]) % (t_w * t_h);
                t_index[2] = (x % t_w + t_w * (y % t_h) + t_offset[2]) % (t_w * t_h);
            }
            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 255.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 255.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 255.0f + errors[2 + 3 * index];
            r = min(max(r, 0.0f), 1.0f);
            g = min(max(g, 0.0f), 1.0f);
            b = min(max(b, 0.0f), 1.0f);
            // float r = toLinear[pixels[index].r] + errors[0 + 3 * index];
            // float g = toLinear[pixels[index].g] + errors[1 + 3 * index];
            // float b = toLinear[pixels[index].b] + errors[2 + 3 * index];
            Oklab oklab_pixel = linearRGB2oklab({r, g, b});

            float noise[3] = { 0.0f, 0.0f, 0.0f };
            noises(mt, noise, noise_type, texture, t_index);

            // Make candidate color list
            float candError[3] = { 0.0f, 0.0f, 0.0f };
            Oklab candList[candSize];
            for (int i = 0; i < candSize; i++) {
                Oklab attempt = oklab_pixel;
                attempt.l += candError[0] * noise_amp;
                attempt.a += candError[1] * noise_amp;
                attempt.b += candError[2] * noise_amp;
                candList[i] = findClosestColor(palette, palette_size, attempt);
                candError[0] += oklab_pixel.l - candList[i].l;
                candError[1] += oklab_pixel.a - candList[i].a;
                candError[2] += oklab_pixel.b - candList[i].b;
            }

            // sort by luminance
            std::sort(candList, candList + candSize, [](const Oklab& a, const Oklab& b) { return a.l < b.l; });
            int candIndexes[3] = { 0, 0, 0 };

            Pixel_RGB useColors[3];
            for (int i = 0; i < 3; i++) {
                candIndexes[i] = static_cast<int>((noise[i] + 0.5f) * (candSize));
                if (candIndexes[i] >= candSize) {
                    candIndexes[i] = candSize - 1;
                } else if (candIndexes[i] < 0) {
                    candIndexes[i] = 0;
                }
                useColors[i] = oklab2rgb(candList[candIndexes[i]]);
            }
            error[0] = (pixels[index].r - useColors[0].r) / 255.0f;
            error[1] = (pixels[index].g - useColors[1].g) / 255.0f;
            error[2] = (pixels[index].b - useColors[2].b) / 255.0f;
            pixels[index].r = useColors[0].r;
            pixels[index].g = useColors[1].g;
            pixels[index].b = useColors[2].b;

            // error diffusion
            diffuse_error(errors, error, x, y, w, h, diffusion_type);
        }
    }

    free(errors);
    free(texture);
    free(palette);
    // free(linear_pixels);
    // free(oklab_pixels);
    return 0;
}

// Custom texture dithering (arbitrary color palette version)
int oklab_custom_texture_palette_dither(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));
    // LinearRGB *linear_pixels = (LinearRGB*)malloc(sizeof(LinearRGB) * w * h);
    // Oklab *oklab_pixels = rgba_to_oklab(pixels, w * h);

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    // array containing palette data is at index 5
    int palette_size = static_cast<int>(lua_tointeger(L, 6));
    Oklab *palette = (Oklab*)malloc(sizeof(Oklab) * palette_size);
    for (int i = 0; i < palette_size; i++) {
        lua_rawgeti(L, 5, i + 1);
        int val = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        Pixel_RGB tmp;
        tmp.r = (val >> 16) & 0xff;
        tmp.g = (val >> 8) & 0xff;
        tmp.b = val & 0xff;
        palette[i] = rgb2oklab(tmp);
    }
    int candSize = static_cast<int>(lua_tointeger(L, 7));
    int do_mixColor = static_cast<int>(lua_tointeger(L, 8));

    int diffusion_type = static_cast<int>(lua_tointeger(L, 9));

    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 10));
    int t_w, t_h;
    Pixel_RGB *texture = nullptr;
    texture = read_txr(texture_path, &t_w, &t_h);
    if (!texture || t_w * t_h == 0) {
        perror("Failed to open texture file");
        return 0;
    }

    float t_x = static_cast<float>(lua_tonumber(L, 11));
    float t_y = static_cast<float>(lua_tonumber(L, 12));
    float t_scale = static_cast<float>(lua_tonumber(L, 13));
    float center_x = static_cast<float>(lua_tonumber(L, 14));
    float center_y = static_cast<float>(lua_tonumber(L, 15));
    int t_loop = static_cast<int>(lua_tointeger(L, 16));

    if (t_scale == 0.0f) {
        perror("Invalid scale");
        return 0;
    }

    int center_disp = static_cast<int>(lua_tointeger(L, 17));
    int seed = static_cast<int>(lua_tointeger(L, 18));
    std::default_random_engine mt(seed);

    float* errors = (float*)calloc(sizeof(float), 3 * w * h);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            float scaled_x = (x - t_x - w/2.0f) / t_scale - center_x + t_w/2.0f;
            float scaled_y = (y - t_y - h/2.0f) / t_scale - center_y + t_h/2.0f;
            int t_index = 0;
            bool use_texture = false;
            if (t_loop == 1) {
                use_texture = true;
            } else if (0 <= scaled_x && scaled_x < t_w && 0 <= scaled_y && scaled_y < t_h) {
                use_texture = true;
            }
            
            if (use_texture) {
                int tmp_y = (static_cast<int>(scaled_y) % t_h + t_h) % t_h;
                t_index = static_cast<int>(scaled_x) % t_w + tmp_y * t_w;
            }

            float error[3] = { 0.0f, 0.0f, 0.0f };
            float r = pixels[index].r / 255.0f + errors[0 + 3 * index];
            float g = pixels[index].g / 255.0f + errors[1 + 3 * index];
            float b = pixels[index].b / 255.0f + errors[2 + 3 * index];
            r = min(max(r, 0.0f), 1.0f);
            g = min(max(g, 0.0f), 1.0f);
            b = min(max(b, 0.0f), 1.0f);
            Oklab oklab_pixel = linearRGB2oklab({r, g, b});

            // テクスチャ範囲外の場合はもっとも近い色を使い誤差伝播
            if (!use_texture) {
                Oklab cand = findClosestColor(palette, palette_size, oklab_pixel);

                Pixel_RGB useColor = oklab2rgb(cand);
                error[0] = (pixels[index].r - useColor.r) / 255.0f;
                error[1] = (pixels[index].g - useColor.g) / 255.0f;
                error[2] = (pixels[index].b - useColor.b) / 255.0f;
                pixels[index].r = useColor.r;
                pixels[index].g = useColor.g;
                pixels[index].b = useColor.b;

                diffuse_error(errors, error, x, y, w, h, diffusion_type);
                continue;
            }

            // テクスチャ範囲内の場合はテクスチャを使ってディザリング
            // Make candidate color list
            float candError[3] = { 0.0f, 0.0f, 0.0f };
            Oklab candList[candSize];
            for (int i = 0; i < candSize; i++) {
                Oklab attempt = oklab_pixel;
                attempt.l += candError[0] * noise_amp;
                attempt.a += candError[1] * noise_amp;
                attempt.b += candError[2] * noise_amp;
                candList[i] = findClosestColor(palette, palette_size, attempt);
                candError[0] += oklab_pixel.l - candList[i].l;
                candError[1] += oklab_pixel.a - candList[i].a;
                candError[2] += oklab_pixel.b - candList[i].b;
            }
            // sort by luminance
            std::sort(candList, candList + candSize, [](const Oklab& a, const Oklab& b) { return a.l < b.l; });
            int candIndexes[3] = { texture[t_index].r, texture[t_index].r, texture[t_index].r };
            if (do_mixColor == 0) {
                int use_channel = channel_RAND(mt);
                if (use_channel == 1) {
                    candIndexes[0] = texture[t_index].g;
                    candIndexes[1] = texture[t_index].g;
                    candIndexes[2] = texture[t_index].g;
                } else if (use_channel == 2) {
                    candIndexes[0] = texture[t_index].b;
                    candIndexes[1] = texture[t_index].b;
                    candIndexes[2] = texture[t_index].b;
                }
            }
            if (do_mixColor == 1) {
                int use_comb = comb_RAND(mt);
                if (use_comb == 1) {
                    candIndexes[0] = texture[t_index].r;
                    candIndexes[1] = texture[t_index].g;
                    candIndexes[2] = texture[t_index].b;
                } else if (use_comb == 2) {
                    // candIndexes[0] = texture[t_index].r;
                    candIndexes[1] = texture[t_index].b;
                    candIndexes[2] = texture[t_index].g;
                } else if (use_comb == 3) {
                    candIndexes[0] = texture[t_index].g;
                    // candIndexes[1] = texture[t_index].r;
                    candIndexes[2] = texture[t_index].b;
                } else if (use_comb == 4) {
                    candIndexes[0] = texture[t_index].g;
                    candIndexes[1] = texture[t_index].b;
                    // candIndexes[2] = texture[t_index].r;
                } else if (use_comb == 5) {
                    candIndexes[0] = texture[t_index].b;
                    // candIndexes[1] = texture[t_index].r;
                    candIndexes[2] = texture[t_index].g;
                } else if (use_comb == 6) {
                    candIndexes[0] = texture[t_index].b;
                    candIndexes[1] = texture[t_index].g;
                    // candIndexes[2] = texture[t_index].r;
                }
            }

            Pixel_RGB useColors[3];
            for (int i = 0; i < 3; i++) {
                candIndexes[i] = static_cast<int>((candIndexes[i] / 255.0f) * (candSize));
                if (candIndexes[i] >= candSize) {
                    candIndexes[i] = candSize - 1;
                } else if (candIndexes[i] < 0) {
                    candIndexes[i] = 0;
                }
                useColors[i] = oklab2rgb(candList[candIndexes[i]]);
            }
            error[0] = (pixels[index].r - useColors[0].r) / 255.0f;
            error[1] = (pixels[index].g - useColors[1].g) / 255.0f;
            error[2] = (pixels[index].b - useColors[2].b) / 255.0f;
            pixels[index].r = useColors[0].r;
            pixels[index].g = useColors[1].g;
            pixels[index].b = useColors[2].b;

            // error diffusion
            diffuse_error(errors, error, x, y, w, h, diffusion_type);
        }
    }

    if (center_disp == 1) {
        const int radius = 2;
        for (int y = -radius; y <= radius; y++){
            for (int x = -radius; x <= radius; x++){
                // 円の外はスキップ
                if (x*x + y*y > radius*radius){
                    continue;
                }

                float moved_x = x + w/2 + t_x;
                float moved_y = y + h/2 + t_y;
                if (moved_x < 0 || w < moved_x || moved_y < 0 || h < moved_y){
                    continue;
                }
                int index = static_cast<int>(moved_x) + w * static_cast<int>(moved_y);
                pixels[index].r = 0;
                pixels[index].g = 0xff;
                pixels[index].b = 0xff;
            }
        }
    }

    free(errors);
    free(texture);
    free(palette);
    // free(linear_pixels);
    // free(oklab_pixels);
    return 1;
}

int texture_debug(lua_State *L) {
    Pixel_RGBA *pixels = reinterpret_cast<Pixel_RGBA*>(lua_touserdata(L, 1));
    int w = static_cast<int>(lua_tointeger(L, 2));
    int h = static_cast<int>(lua_tointeger(L, 3));

    float noise_amp = static_cast<float>(lua_tonumber(L, 4));
    int c_num = static_cast<int>(lua_tointeger(L, 5)) - 1;

    int seed = static_cast<int>(lua_tointeger(L, 6));
    int noise_type = static_cast<int>(lua_tointeger(L, 7));
    int diffusion_type = static_cast<int>(lua_tointeger(L, 8));

    // open texture file
    char texture_path[512];
    sprintf(texture_path, "%s", lua_tostring(L, 9));
    int t_w, t_h;
    Pixel_RGB *texture = read_txr(texture_path, &t_w, &t_h);
    if (!texture) {
        return 1;
    }

    float* errors;
    errors = (float*)malloc(sizeof(float) * 3 * w * h);
    for (int i = 0; i < 3 * w * h; i++) {
        errors[i] = 0.0f;
    }

    std::default_random_engine mt(seed);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int index = x + w * y;
            int t_index = x % t_w + t_w * (y % t_h);
            if (x < t_w && y < t_h) {
                pixels[index].r = texture[t_index].r;
                pixels[index].g = texture[t_index].g;
                pixels[index].b = texture[t_index].b;
            }
        }
    }

    free(errors);
    free(texture);
    return 0;
}

// Make function list
static luaL_Reg functions[] = {
    { "uniform_palette_dither", noisy_error_diffusion_dither },
    { "uniform_palette_custom_dither", custom_texture_diffusion_dither },
    { "palette_dither", oklab_noisy_error_diffusion_custom_palette_dither },
    { "custom_palette_dither", oklab_custom_texture_palette_dither },
    { "texture_debug", texture_debug },
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
