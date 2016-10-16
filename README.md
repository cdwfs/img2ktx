img2ktx
=======

img2ktx is a simple command-line utility.

It is so simple, it doesn't even have a project file. Look at the top of the source file for
build instructions. (The .sln file is for debugging only)

It currently runs on Windows, but that's mostly because it's the only place I've built ispc_texcomp.

It loads images with [stb_image](http://github.com/nothings/stb).

It generates mipmap chains with [stb_image_resize](http://github.com/nothings/stb).

It compresses the mipmaps to BC1, BC3, BC7, ETC1, or ASTC with Intel's
[ISPC Texture Compressor](https://github.com/GameTechDev/ISPCTextureCompressor).

It writes the compressed mips to a [KTX](https://www.khronos.org/opengles/sdk/tools/KTX/) file.

That's it!

It may eventually do the following things as well:

- Output [DDS](https://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx) files,
  because inevitably somebody is going to ask for it.
- Combine multiple input images into an [array texture](https://www.opengl.org/wiki/Array_Texture),
  if their dimensions match.
- Convert animated GIFs directly into array textures, because what 3D graphics application isn't
  improved by animated GIFs?
- Run on Linux, because I believe in the
  [LIMITLESS POWER OF COMMUNITY](https://www.youtube.com/watch?v=kkOXwXHvFa4). (**UPDATE:** I confirmed
  that this works; just BYO ispc_texcomp and you're good to go.)
- Run as a web server, with CGI and XMLRPC interfaces.