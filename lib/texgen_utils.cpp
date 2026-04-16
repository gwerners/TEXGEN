#include "texgen_utils.h"
#include <cstdio>
#include <cstring>
#include <random>

void MatMult(Matrix44 &dest, const Matrix44 &a, const Matrix44 &b) {
  for (sInt i = 0; i < 4; i++)
    for (sInt j = 0; j < 4; j++)
      dest[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] +
                   a[i][2] * b[2][j] + a[i][3] * b[3][j];
}

void MatScale(Matrix44 &dest, sF32 sx, sF32 sy, sF32 sz) {
  sSetMem(dest, 0, sizeof(dest));
  dest[0][0] = sx;
  dest[1][1] = sy;
  dest[2][2] = sz;
  dest[3][3] = 1.0f;
}

void MatTranslate(Matrix44 &dest, sF32 tx, sF32 ty, sF32 tz) {
  MatScale(dest, 1.0f, 1.0f, 1.0f);
  dest[3][0] = tx;
  dest[3][1] = ty;
  dest[3][2] = tz;
}

void MatRotateZ(Matrix44 &dest, sF32 angle) {
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

void Colorize(GenTexture &img, sU32 startCol, sU32 endCol) {
  Matrix44 m;
  Pixel s, e;
  s.Init(startCol);
  e.Init(endCol);
  sSetMem(m, 0, sizeof(m));
  m[0][0] = (e.r - s.r) / 65535.0f;
  m[1][1] = (e.g - s.g) / 65535.0f;
  m[2][2] = (e.b - s.b) / 65535.0f;
  m[3][3] = 1.0f;
  m[0][3] = s.r / 65535.0f;
  m[1][3] = s.g / 65535.0f;
  m[2][3] = s.b / 65535.0f;
  img.ColorMatrixTransform(img, m, sTRUE);
}

bool SaveImage(GenTexture &img, const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (!f)
    return false;

  sU8 buffer[18];
  sSetMem(buffer, 0, sizeof(buffer));
  buffer[2] = 2;
  buffer[12] = img.XRes & 0xff;
  buffer[13] = img.XRes >> 8;
  buffer[14] = img.YRes & 0xff;
  buffer[15] = img.YRes >> 8;
  buffer[16] = 32;

  if (!fwrite(buffer, sizeof(buffer), 1, f)) {
    fclose(f);
    return false;
  }

  sU8 *lineBuf = new sU8[img.XRes * 4];
  for (sInt y = 0; y < img.YRes; y++) {
    const Pixel *in = &img.Data[y * img.XRes];
    for (sInt x = 0; x < img.XRes; x++) {
      lineBuf[x * 4 + 0] = in->b >> 8;
      lineBuf[x * 4 + 1] = in->g >> 8;
      lineBuf[x * 4 + 2] = in->r >> 8;
      lineBuf[x * 4 + 3] = in->a >> 8;
      in++;
    }
    if (!fwrite(lineBuf, img.XRes * 4, 1, f)) {
      delete[] lineBuf;
      fclose(f);
      return false;
    }
  }

  delete[] lineBuf;
  fclose(f);
  return true;
}

void randomVoronoi(GenTexture &dest, const GenTexture &grad, sInt intensity,
                   sInt maxCount, sF32 minDist) {
  sVERIFY(maxCount <= 256);
  CellCenter centers[256];

  static std::random_device rd;
  static std::mt19937 gen(rd());
  auto rnd = [](int mn, int mx) -> int {
    std::uniform_int_distribution<int> dist(mn, mx);
    return dist(gen);
  };
  auto rndd = [](double mn, double mx) -> double {
    std::uniform_real_distribution<double> dist(mn, mx);
    return dist(gen);
  };

  for (sInt i = 0; i < maxCount; i++) {
    int R = rnd(0, intensity);
    int G = rnd(0, intensity);
    int B = rnd(0, intensity);
    int transparency = rnd(0, 255);
    centers[i].x = rndd(0.0, 1.0);
    centers[i].y = rndd(0.0, 1.0);
    centers[i].color.Init(R, G, B, transparency);
  }

  sF32 minDistSq = minDist * minDist;
  for (sInt i = 1; i < maxCount;) {
    sF32 x = centers[i].x;
    sF32 y = centers[i].y;
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
      if (dx * dx + dy * dy < minDistSq)
        break;
    }
    if (j < i)
      centers[i] = centers[--maxCount];
    else
      i++;
  }

  dest.Cells(grad, centers, maxCount, 0.0f, GenTexture::CellInner);
}
