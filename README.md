img2ktx
=======

img2ktx is a simple command-line utility.

It currently runs on Windows, Linux and MacOS.  Use [CMake](https://cmake.org) to generate
a project file for your platform.

It loads images with [stb_image](https://github.com/nothings/stb). Supported formats include
JPEG, PNG, BMP, TGA, GIF, etc.

It optionally generates mipmap chains with [stb_image_resize](http://github.com/nothings/stb).

It compresses the mipmaps to BC1, BC3, BC7, ETC1, or ASTC with AMD's
[Compressonator](https://github.com/GPUOpen-Tools/Compressonator/). It can also
output uncompressed 32-bit RGBA images. Compressonator is included as a submodule; remember to
pass `--recursive` to `git clone` in order to get it and its dependencies.

It writes the compressed images to a [KTX](https://www.khronos.org/opengles/sdk/tools/KTX/) file.
If more than one image is provided with identical dimensions, the output KTX file can be either a
2D texture array or a cubemap.

Download pre-built binaries from the [Releases](https://github.com/cdwfs/img2ktx/releases) page.

That's it!

It may eventually do the following things as well:

- Output [DDS](https://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx) files,
  because inevitably somebody is going to ask for it.
- Convert animated GIFs directly into array textures, because what 3D graphics application isn't
  improved by animated GIFs?
