#pragma once
// Pure utility functions for texture generation (no UI/graphics dependencies).
#include "gentexture.hpp"

void MatMult(Matrix44 &dest, const Matrix44 &a, const Matrix44 &b);
void MatScale(Matrix44 &dest, sF32 sx, sF32 sy, sF32 sz);
void MatTranslate(Matrix44 &dest, sF32 tx, sF32 ty, sF32 tz);
void MatRotateZ(Matrix44 &dest, sF32 angle);

GenTexture LinearGradient(sU32 startCol, sU32 endCol);
void Colorize(GenTexture &img, sU32 startCol, sU32 endCol);
bool SaveImage(GenTexture &img, const char *filename);
void randomVoronoi(GenTexture &dest, const GenTexture &grad, sInt intensity,
                   sInt maxCount, sF32 minDist);
