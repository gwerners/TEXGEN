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

// 4x4 matrix multiply
void MatMult(Matrix44& dest, const Matrix44& a, const Matrix44& b) {
  for (sInt i = 0; i < 4; i++)
    for (sInt j = 0; j < 4; j++)
      dest[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] +
                   a[i][3] * b[3][j];
}

// Create a scaling matrix
void MatScale(Matrix44& dest, sF32 sx, sF32 sy, sF32 sz) {
  sSetMem(dest, 0, sizeof(dest));
  dest[0][0] = sx;
  dest[1][1] = sy;
  dest[2][2] = sz;
  dest[3][3] = 1.0f;
}

// Create a translation matrix
void MatTranslate(Matrix44& dest, sF32 tx, sF32 ty, sF32 tz) {
  MatScale(dest, 1.0f, 1.0f, 1.0f);
  dest[3][0] = tx;
  dest[3][1] = ty;
  dest[3][2] = tz;
}

// Create a z-axis rotation matrix
void MatRotateZ(Matrix44& dest, sF32 angle) {
  sF32 s = sFSin(angle);
  sF32 c = sFCos(angle);

  MatScale(dest, 1.0f, 1.0f, 1.0f);
  dest[0][0] = c;
  dest[0][1] = s;
  dest[1][0] = -s;
  dest[1][1] = c;
}

GenTexture LinearGradient(sU32 startCol, sU32 endCol) {
  GenTexture tex;

  tex.Init(2, 1);
  tex.Data[0].Init(startCol);
  tex.Data[1].Init(endCol);

  return tex;
}

// Transforms a grayscale image to a colored one with a matrix transform
void Colorize(GenTexture& img, sU32 startCol, sU32 endCol) {
  Matrix44 m;
  Pixel s, e;

  s.Init(startCol);
  e.Init(endCol);

  // calculate matrix
  sSetMem(m, 0, sizeof(m));
  m[0][0] = (e.r - s.r) / 65535.0f;
  m[1][1] = (e.g - s.g) / 65535.0f;
  m[2][2] = (e.b - s.b) / 65535.0f;
  m[3][3] = 1.0f;
  m[0][3] = s.r / 65535.0f;
  m[1][3] = s.g / 65535.0f;
  m[2][3] = s.b / 65535.0f;

  // transform
  img.ColorMatrixTransform(img, m, sTRUE);
}

// Save an image as .TGA file
bool SaveImage(GenTexture& img, const char* filename) {
  FILE* f = fopen(filename, "wb");
  if (!f)
    return false;

  // prepare header
  sU8 buffer[18];
  sSetMem(buffer, 0, sizeof(buffer));

  buffer[2] = 2;                 // image type code 2 (RGB, uncompressed)
  buffer[12] = img.XRes & 0xff;  // width (low byte)
  buffer[13] = img.XRes >> 8;    // width (high byte)
  buffer[14] = img.YRes & 0xff;  // height (low byte)
  buffer[15] = img.YRes >> 8;    // height (high byte)
  buffer[16] = 32;               // pixel size (bits)

  // write header
  if (!fwrite(buffer, sizeof(buffer), 1, f)) {
    fclose(f);
    return false;
  }

  // write image data
  sU8* lineBuf = new sU8[img.XRes * 4];
  for (sInt y = 0; y < img.YRes; y++) {
    const Pixel* in = &img.Data[y * img.XRes];

    // convert a line of pixels (as simple as possible - no gamma correction
    // etc.)
    for (sInt x = 0; x < img.XRes; x++) {
      lineBuf[x * 4 + 0] = in->b >> 8;
      lineBuf[x * 4 + 1] = in->g >> 8;
      lineBuf[x * 4 + 2] = in->r >> 8;
      lineBuf[x * 4 + 3] = in->a >> 8;

      in++;
    }

    // write to file
    if (!fwrite(lineBuf, img.XRes * 4, 1, f)) {
      fclose(f);
      return false;
    }
  }

  fclose(f);
  return true;
}

// Create a pattern of randomly colored voronoi cells
void randomVoronoi(GenTexture& dest,
                   const GenTexture& grad,
                   sInt intensity,
                   sInt maxCount,
                   sF32 minDist) {
  sVERIFY(maxCount <= 256);
  CellCenter centers[256];
  //  generate random center points
#ifdef USE_GWEN_VERSION
  auto rnd = [](int min, int max) -> int {
    int range = max - min + 1;                  // Size of the range [min, max]
    int limit = RAND_MAX - (RAND_MAX % range);  // Limit to avoid bias

    int r;
    do {
      r = rand();  // Generate a random number
    } while (r >= limit);  // Reject numbers that would introduce bias

    return min + (r % range);  // Scale to the desired range [min, max]
  };
  // Lambda for generating a random float in the range [min, max]
  auto rndf = [](float min, float max) -> float {
    float r = (float)rand() / RAND_MAX;  // Scale to [0.0, 1.0]
    return min + r * (max - min);        // Scale to [min, max]
  };

  // Lambda for generating a random double in the range [min, max]
  auto rndd = [](double min, double max) -> double {
    double r = (double)rand() / RAND_MAX;  // Scale to [0.0, 1.0]
    return min + r * (max - min);          // Scale to [min, max]
  };
#endif
#ifdef USE_UNIFORM
  // chatgpt version
  auto rnd = [](int min, int max) -> int {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
  };

  auto rndf = [](float min, float max) -> float {
    static std::random_device rd;
    static std::mt19937 gen(rd());  // Mersenne Twister PRNG
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
  };
  auto rndd = [](double min, double max) -> double {
    static std::random_device rd;
    static std::mt19937 gen(rd());  // Mersenne Twister PRNG
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
  };
#else

  auto rnd = [](int min, int max) -> int {
    return min + rand() % (max - min + 1);
  };
  auto rndf = [](float min, float max) -> float {
    return min + (rand() / (float)RAND_MAX) * (max - min);
  };
#endif
#ifdef ORIGINAL_KTG
  for (sInt i = 0; i < maxCount; i++) {
    int intens = (rand() * intensity) / RAND_MAX;

    centers[i].x = 1.0f * rand() / RAND_MAX;
    centers[i].y = 1.0f * rand() / RAND_MAX;
    centers[i].color.Init(intens, intens, intens, 255);
  }
#else
  for (sInt i = 0; i < maxCount; i++) {
    int R = rnd(0, intensity);
    int G = rnd(0, intensity);
    int B = rnd(0, intensity);
    int transparency = rnd(0, 255);
    centers[i].x = rndd(0.0, 1.0f);
    centers[i].y = rndd(0.0, 1.0f);
    centers[i].color.Init(R, G, B, transparency);
  }
#endif

  // remove points too close together
  sF32 minDistSq = minDist * minDist;
  for (sInt i = 1; i < maxCount;) {
    sF32 x = centers[i].x;
    sF32 y = centers[i].y;

    // try to find a point closer than minDist
    sInt j;
    for (j = 0; j < i; j++) {
      sF32 dx = centers[j].x - x;
      sF32 dy = centers[j].y - y;

      if (dx < 0.0f)
        dx += 1.0f;
      if (dy < 0.0f)
        dy += 1.0f;

      dx = sMin(dx, 1.0f - dx);
      dy = sMin(dy, 1.0f - dy);

      if (dx * dx + dy * dy < minDistSq)  // point is too close, stop
        break;
    }

    if (j < i)                           // we found such a point
      centers[i] = centers[--maxCount];  // remove this one
    else                                 // accept this one
      i++;
  }

  // generate the image
  dest.Cells(grad, centers, maxCount, 0.0f, GenTexture::CellInner);
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
