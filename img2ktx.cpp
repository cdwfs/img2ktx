/* img2ktx
 *
 * To build in Linux:
 *   cc -L. -lispc_texcomp -o img2ktx img2ktx.cpp
 *
 * To build with Visual Studio 2013:
 *   "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
 *   cl -W4 -MT -nologo -EHsc -wd4996 /Feimg2ktx.exe img2ktx.cpp /link -incremental:no -opt:ref ispc_texcomp.lib
 * Debug-mode:
 *   cl -W4 -Od -Z7 -FC -MTd -nologo -EHsc -wd4996 /Feimg2ktx.exe img2ktx.cpp /link -incremental:no -opt:ref ispc_texcomp.lib
 */

#include "ispc_texcomp.h"

#pragma warning(push,3)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma warning(disable:4702)  // unreachable code
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#pragma warning(pop)

#include <algorithm>
#include <cstdio>
#include <cstdlib>

enum {
    // For glFormat
    GL_RED                           = 0x1903,
    GL_RG                            = 0x8227,
    GL_RGB                           = 0x1907,
    GL_RGBA                          = 0x1908,

    // For glInternalFormat
    GL_COMPRESSED_RGB_S3TC_DXT1_EXT           = 0x83F0, // BC1 (no alpha)
    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT          = 0x83F1, // BC1 (alpha)
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT          = 0x83F3, // BC3
    GL_COMPRESSED_RGBA_BPTC_UNORM_ARB         = 0x8E8C, // BC7
    GL_COMPRESSED_RGBA_ASTC_4x4_KHR           = 0x93B0,
    GL_COMPRESSED_RGBA_ASTC_5x4_KHR           = 0x93B1,
    GL_COMPRESSED_RGBA_ASTC_5x5_KHR           = 0x93B2,
    GL_COMPRESSED_RGBA_ASTC_6x5_KHR           = 0x93B3,
    GL_COMPRESSED_RGBA_ASTC_6x6_KHR           = 0x93B4,
    GL_COMPRESSED_RGBA_ASTC_8x5_KHR           = 0x93B5,
    GL_COMPRESSED_RGBA_ASTC_8x6_KHR           = 0x93B6,
    GL_COMPRESSED_RGBA_ASTC_8x8_KHR           = 0x93B7,
    GL_COMPRESSED_RGBA_ASTC_10x5_KHR          = 0x93B8,
    GL_COMPRESSED_RGBA_ASTC_10x6_KHR          = 0x93B9,
    GL_COMPRESSED_RGBA_ASTC_10x8_KHR          = 0x93BA,
    GL_COMPRESSED_RGBA_ASTC_10x10_KHR         = 0x93BB,
    GL_COMPRESSED_RGBA_ASTC_12x10_KHR         = 0x93BC,
    GL_COMPRESSED_RGBA_ASTC_12x12_KHR         = 0x93BD,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR   = 0x93D0,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR   = 0x93D1,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR   = 0x93D2,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR   = 0x93D3,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR   = 0x93D4,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR   = 0x93D5,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR   = 0x93D6,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR   = 0x93D7,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR  = 0x93D8,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR  = 0x93D9,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR  = 0x93DA,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = 0x93DB,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = 0x93DC,
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = 0x93DD,
};

struct GlFormatInfo {
    const char *name;
    uint32_t internal_format;
    uint32_t base_format;
    uint32_t block_dim_x;
    uint32_t block_dim_y;
    uint32_t block_bytes;
};
const GlFormatInfo g_formats[] = {
    { "BC1",     GL_COMPRESSED_RGB_S3TC_DXT1_EXT,   GL_RGB,   4, 4,  8 },
    { "BC1a",    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,  GL_RGBA,  4, 4, 16 },
    { "BC3",     GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,  GL_RGBA,  4, 4, 16 },
    { "BC7",     GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_RGBA,  4, 4, 16 },
    { "ASTC4x4", GL_COMPRESSED_RGBA_ASTC_4x4_KHR,   GL_RGBA,  4, 4, 16 },
    { "ASTC5x4", GL_COMPRESSED_RGBA_ASTC_5x4_KHR,   GL_RGBA,  5, 4, 16 },
    { "ASTC5x5", GL_COMPRESSED_RGBA_ASTC_5x5_KHR,   GL_RGBA,  5, 5, 16 },
    { "ASTC6x5", GL_COMPRESSED_RGBA_ASTC_6x5_KHR,   GL_RGBA,  6, 5, 16 },
    { "ASTC6x6", GL_COMPRESSED_RGBA_ASTC_6x6_KHR,   GL_RGBA,  6, 6, 16 },
    { "ASTC8x5", GL_COMPRESSED_RGBA_ASTC_8x5_KHR,   GL_RGBA,  8, 5, 16 },
    { "ASTC8x6", GL_COMPRESSED_RGBA_ASTC_8x6_KHR,   GL_RGBA,  8, 6, 16 },
    { "ASTC8x8", GL_COMPRESSED_RGBA_ASTC_8x8_KHR,   GL_RGBA,  8, 8, 16 },
};
const size_t g_format_count = sizeof(g_formats) / sizeof(g_formats[0]);

struct KtxHeader {
    uint8_t identifier[12];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t pixelWidth;
    uint32_t pixelHeight;
    uint32_t pixelDepth;
    uint32_t numberOfArrayElements;
    uint32_t numberOfFaces;
    uint32_t numberOfMipmapLevels;
    uint32_t bytesOfKeyValueData;
};

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s [input] [output] [compress_type]\n",
                argv[0]);
        return 1;
    }
    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    const char *output_format_name = argv[3];
    bool generate_mipmaps = true;

    // Look up the output format info
    const GlFormatInfo *format_info = NULL;
    for(int f = 0; f < g_format_count; ++f) {
        if (strcmp(g_formats[f].name, output_format_name) == 0) {
            format_info = g_formats + f;
            break;
        }
    }
    if (format_info == NULL) {
        fprintf(stderr, "Error: format %s not supported\n",
                output_format_name);
        return 1;
    }
    const int bytes_per_block = format_info->block_bytes;
    const int block_dim_x = format_info->block_dim_x;
    const int block_dim_y = format_info->block_dim_y;

    // Load the input file
    int base_width = 0, base_height = 0;
    int mip_width = 0, mip_height = 0, mip_pitch_x = 0, mip_pitch_y = 0;
    int input_components = 4; // ispc_texcomp requires 32-bit RGBA input
    int original_components = 0;
    stbi_uc *input_pixels = stbi_load(input_filename, &base_width,
            &base_height, &original_components, input_components);
    if (!input_pixels) {
        fprintf(stderr, "Error loading input '%s'\n", input_filename);
        return 2;
    }
    printf("Loaded %s -- width=%d height=%d comp=%d\n",
            input_filename, base_width, base_height,
            original_components);
    int mip_levels = 1;
    if (generate_mipmaps) {
        int mip_width = base_width, mip_height = base_height;
        while (mip_width > 1 || mip_height > 1) {
            mip_levels += 1;
            mip_width  = std::max(1, mip_width / 2);
            mip_height = std::max(1, mip_height / 2);
        }
    }

    // Generate the input mipmap chain. At every level, the input
    // width and height must be padded up to a multiple of the output
    // block dimensions.
    stbi_uc **input_mip_pixels =
        (stbi_uc**)malloc(mip_levels * sizeof(stbi_uc**));
    mip_width  = base_width;
    mip_height = base_height;
    mip_pitch_x = ((mip_width  + block_dim_x - 1) / block_dim_x) * block_dim_x;
    mip_pitch_y = ((mip_height + block_dim_y - 1) / block_dim_y) * block_dim_y;
    input_mip_pixels[0] =
        (stbi_uc*)malloc(mip_pitch_x * mip_pitch_y * input_components);
    // memset(input_mip_pixels[0], 0, mip_pitch_x * mip_pitch_y * input_components);
    for(int y=0; y<base_height; ++y) {
        memcpy(input_mip_pixels[0] + y * mip_width * input_components,
                input_pixels + y * base_width * input_components,
                base_width * input_components);
    }
    for(int i=1; i<mip_levels; ++i) {
        int src_width  = mip_width;
        int src_height = mip_height;
        int src_pitch_x = mip_pitch_x;
        //int src_pitch_y = mip_pitch_y;
        mip_width  = std::max(1, mip_width  / 2);
        mip_height = std::max(1, mip_height / 2);
        mip_pitch_x = ((mip_width  + block_dim_x - 1) / block_dim_x) * block_dim_x;
        mip_pitch_y = ((mip_height + block_dim_y - 1) / block_dim_y) * block_dim_y;
        input_mip_pixels[i] =
            (stbi_uc*)malloc(mip_pitch_x * mip_pitch_y * input_components);
        // memset(input_mip_pixels[i], 0, mip_pitch_x * mip_pitch_y * num_components);
        //printf("mip %u: width=%d height=%d\n", i, mip_width, mip_height);
        stbir_resize_uint8(
            input_mip_pixels[i-1], src_width, src_height, src_pitch_x * input_components,
            input_mip_pixels[i], mip_width, mip_height, mip_pitch_x * input_components,
            input_components);
    }
    stbi_image_free(input_pixels);

    // Generate output mip chain
    uint8_t **output_mip_pixels =
        (uint8_t**)malloc(mip_levels * sizeof(uint8_t**));
    uint32_t *output_mip_sizes =
        (uint32_t*)malloc(mip_levels * sizeof(uint32_t));
    mip_width = base_width;
    mip_height = base_height;
    for(int i=0; i<mip_levels; ++i) {
        rgba_surface input_surface = {};
        input_surface.ptr = input_mip_pixels[i];
        input_surface.width  = ((mip_width  + block_dim_x - 1) / block_dim_x) * block_dim_x;
        input_surface.height = ((mip_height + block_dim_y - 1) / block_dim_y) * block_dim_y;
        input_surface.stride = input_surface.width * input_components;
        printf("compressing mip %u: width=%d height=%d pitch_x=%d pitch_y=%d\n", i, mip_width, mip_height, input_surface.width, input_surface.height);
 
        int num_blocks = (input_surface.width / block_dim_x)
            * (input_surface.height / block_dim_y);
        output_mip_sizes[i] = num_blocks * bytes_per_block;
        output_mip_pixels[i] = (uint8_t*)malloc(output_mip_sizes[i]);
        if (      (strcmp(output_format_name, "BC1")  == 0) ||
                (  strcmp(output_format_name, "BC1a") == 0)) {
            CompressBlocksBC1(&input_surface, output_mip_pixels[i]);
        } else if (strcmp(output_format_name, "BC3") == 0) {
            CompressBlocksBC3(&input_surface, output_mip_pixels[i]);
        } else if (strcmp(output_format_name, "BC7") == 0) {
            bc7_enc_settings enc_settings = {};
            if (original_components == 3) {
                GetProfile_basic(&enc_settings);
            } else if (original_components == 4) {
                GetProfile_alpha_basic(&enc_settings);
            }
            CompressBlocksBC7(&input_surface, output_mip_pixels[i],
                    &enc_settings);
        } else if (strncmp(output_format_name, "ASTC", 4) == 0) {
            astc_enc_settings enc_settings = {};
            if (original_components == 3) {
                GetProfile_astc_fast(&enc_settings,
                        format_info->block_dim_x, format_info->block_dim_y);
            } else if (original_components == 4) {
                GetProfile_astc_alpha_fast(&enc_settings,
                        format_info->block_dim_x, format_info->block_dim_y);
            }
            CompressBlocksASTC(&input_surface, output_mip_pixels[i],
                    &enc_settings);
        }
        mip_width  = std::max(1, mip_width  / 2);
        mip_height = std::max(1, mip_height / 2);
    }
    for(int i=0; i<mip_levels; ++i) {
        free(input_mip_pixels[i]);
    }
    free(input_mip_pixels);

    KtxHeader header = {};
    const uint8_t ktx_magic_id[12] = {
        0xAB, 0x4B, 0x54, 0x58,
        0x20, 0x31, 0x31, 0xBB,
        0x0D, 0x0A, 0x1A, 0x0A
    };
    memcpy(header.identifier, ktx_magic_id, 12);
    header.endianness = 0x04030201;
    header.glType = 0;
    header.glTypeSize = 1;
    header.glFormat = 0;
    header.glInternalFormat = format_info->internal_format;
    header.glBaseInternalFormat = format_info->base_format;
    header.pixelWidth = base_width;
    header.pixelHeight = base_height;
    header.pixelDepth = 1;
    header.numberOfArrayElements = 1;
    header.numberOfFaces = 1;
    header.numberOfMipmapLevels = mip_levels;
    header.bytesOfKeyValueData = 0;
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Error opening output '%s'\n", output_filename);
        return 3;
    }
    fwrite(&header, 1, sizeof(KtxHeader), output_file);
    for(int i=0; i<mip_levels; ++i) {
        fwrite(&output_mip_sizes[i], 1, sizeof(uint32_t), output_file);
        fwrite(output_mip_pixels[i], 1, output_mip_sizes[i], output_file);
        uint32_t mip_padding = 3 - ((output_mip_sizes[i] + 3) % 4);
        uint32_t zero = 0;
        fwrite(&zero, 1, mip_padding, output_file);
    }
    size_t output_file_size = ftell(output_file);
    fclose(output_file);
    printf("Wrote %s (format=%s, mips=%u, size=%lu)\n", output_filename,
            output_format_name, mip_levels, output_file_size);

    for(int i=0; i<mip_levels; ++i) {
        free(output_mip_pixels[i]);
    }
    free(output_mip_pixels);
    free(output_mip_sizes);
    
    return 0;
}
