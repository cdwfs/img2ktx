/* img2ktx
 *
 * For a unit test on g++/Clang:
 *   cc -Wall -pthread -std=c++11 -D_POSIX_C_SOURCE=199309L -g -x c -DCDS_JOB_TEST -o test_cds_job.exe cds_job.h -lstdc++ -lpthread
 * Clang users may also pass -fsanitize=thread to enable Clang's
 * ThreadSanitizer feature.
 *
 * For a unit test on Visual C++:
 *   "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
 *   cl -W4 -MT -nologo -EHsc -wd4996 /Feimg2ktx.exe img2ktx.cpp /link -incremental:no -opt:ref ispc_texcomp.lib
 * Debug-mode:
 *   cl -W4 -Od -Z7 -FC -MTd -nologo -EHsc -wd4996 /Feimg2ktx.exe img2ktx.cpp /link -incremental:no -opt:ref ispc_texcomp.lib
 */

#include "ispc_texcomp.h"

#pragma warning(push)
#pragma warning(disable:4244)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

#include <stdio.h>
#include <stdlib.h>

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
};

struct GlFormatInfo {
    const char *name;
    uint32_t internal_format;
    uint32_t base_format;
    uint32_t components;
    uint32_t block_dim;
    uint32_t block_bytes;
};
const GlFormatInfo g_formats[] = {
    { "BC1",     GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB,            3, 4,  8 },
    { "BC1a",    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA,          4, 4, 16 },
    { "BC3",     GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA,          4, 4, 16 },
    { "BC7",     GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_RGBA,         4, 4, 16 },
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
    
    int input_width = 0, input_height = 0;
    int input_components = 4;
    int original_components = 0;
    stbi_uc *input_pixels = stbi_load(input_filename, &input_width,
            &input_height, &original_components, input_components);
    if (!input_pixels) {
        fprintf(stderr, "Error loading input '%s'\n", input_filename);
        return 2;
    }
    printf("Loaded %s -- width=%d height=%d comp=%d\n",
            input_filename, input_width, input_height,
            original_components);
        
    rgba_surface input_surface = {};
    input_surface.ptr = input_pixels;
    input_surface.width = input_width;
    input_surface.height = input_height;
    input_surface.stride = input_width * input_components;

    const int bytes_per_block = format_info->block_bytes;
    const int block_dim = format_info->block_dim;
    int num_blocks = ((input_width+block_dim-1)/block_dim)
        * ((input_height+block_dim-1)/block_dim);
    uint32_t output_surface_size = num_blocks * bytes_per_block;
    uint8_t *output_surface = (uint8_t*)malloc(output_surface_size);
    if (       strcmp(output_format_name, "BC1") == 0 ||
               strcmp(output_format_name, "BC1a") == 0) {
        CompressBlocksBC1(&input_surface, output_surface);
    } else if (strcmp(output_format_name, "BC3") == 0) {
        CompressBlocksBC3(&input_surface, output_surface);
    } else if (strcmp(output_format_name, "BC7") == 0) {
        bc7_enc_settings enc_settings = {};
        if (original_components == 3)
            GetProfile_basic(&enc_settings);
        else if (original_components == 4) 
            GetProfile_alpha_basic(&enc_settings);
        CompressBlocksBC7(&input_surface, output_surface, &enc_settings);
    }
    stbi_image_free(input_pixels);

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
    header.pixelWidth = input_width;
    header.pixelHeight = input_height;
    header.pixelDepth = 1;
    header.numberOfArrayElements = 1;
    header.numberOfFaces = 1;
    header.numberOfMipmapLevels = 1;
    header.bytesOfKeyValueData = 0;
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Error opening output '%s'\n", output_filename);
        return 3;
    }
    fwrite(&header, 1, sizeof(KtxHeader), output_file);
    fwrite(&output_surface_size, 1, sizeof(uint32_t), output_file);
    fwrite(output_surface, 1, output_surface_size, output_file);
    uint32_t mip_padding = 3 - ((output_surface_size+3) % 4);
    uint32_t zero = 0;
    fwrite(&zero, 1, mip_padding, output_file);
    size_t output_file_size = ftell(output_file);
    fclose(output_file);
    printf("Wrote %s (format=%s, size=%lu)\n", output_filename,
            output_format_name, output_file_size);
    free(output_surface);
    return 0;
}
