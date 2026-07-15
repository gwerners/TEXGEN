#include "Utils.h"
#include <fmt/color.h>
#include <fmt/format.h>
#include <fstream>

#ifndef _WIN32

long timeGetTime() {
  timeval tim;
  gettimeofday(&tim, NULL);
  return tim.tv_sec * 1000000 + tim.tv_usec / 10;
}
int timeBeginPeriod(unsigned int period) {
  return 0;
}
int timeEndPeriod(unsigned int period) {
  return 0;
}
#endif

bool exists(const std::string& filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}
std::string readFile(bool debug, const std::string& filename) {
  std::ifstream t(filename);
  std::string ret = std::string((std::istreambuf_iterator<char>(t)),
                                std::istreambuf_iterator<char>());
  if (debug) {
    fmt::print(fg(fmt::color::blue), "{}:{} - {} contents[{}]\n", __FUNCTION__,
               __LINE__, filename, ret);
  }
  return ret;
}







Texture2D LoadTextureFromGenTexture(GenTexture tex) {
  Texture2D texture = {0};
  PixelFormat format = PixelFormat::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  int mipmapCount = 1;
  if ((tex.XRes != 0) && (tex.YRes != 0)) {
    // write image data
    sU8* data = new sU8[tex.XRes * tex.YRes * 4];
    sU8* lineBuf = new sU8[tex.XRes * 4];
    sU8* dataPtr = data;
    for (sInt y = 0; y < tex.YRes; y++) {
      const Pixel* in = &tex.Data[y * tex.XRes];

      // convert a line of pixels (as simple as possible - no gamma correction
      // etc.)
      for (sInt x = 0; x < tex.XRes; x++) {
        lineBuf[x * 4 + 0] = in->r >> 8;
        lineBuf[x * 4 + 1] = in->g >> 8;
        lineBuf[x * 4 + 2] = in->b >> 8;
        lineBuf[x * 4 + 3] = in->a >> 8;
        in++;
      }
      // write to file
      memcpy(dataPtr, lineBuf, tex.XRes * 4);
      dataPtr += (tex.XRes * 4);
    }
    texture.id = rlLoadTexture((const void*)data, tex.XRes, tex.YRes, format,
                               mipmapCount);
    delete[] lineBuf;
    delete[] data;
  }
  texture.width = tex.XRes;
  texture.height = tex.YRes;
  texture.mipmaps = mipmapCount;
  texture.format = format;
  return texture;
}
