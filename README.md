img2ktx
=======

img2ktx is a simple command-line utility.

It is so simple, it doesn't even have a project file. Look at the top of the source file for
build instructions. (The .sln file is for debugging only)

It currently runs on Windows, Linux and MacOS. 
Only the Windows ispc_texcomp library is included in
the repo; Linux and MacOS users must build their own.

It loads images with [stb_image](http://github.com/nothings/stb). Supported formats include
JPEG, PNG, BMP, TGA, GIF, etc.

It optionally generates mipmap chains with [stb_image_resize](http://github.com/nothings/stb).

It compresses the mipmaps to BC1, BC3, BC7, ETC1, or ASTC with Intel's
[ISPC Texture Compressor](https://github.com/GameTechDev/ISPCTextureCompressor). It can also
output uncompressed 32-bit RGBA images.

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
- Run as a web server, with CGI and XMLRPC interfaces.
