#ifndef UTILS_H
#define UTILS_H

#include <raylib.h>
#include <sys/stat.h>
#include <string>

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <time.h>
#include "gentexture.hpp"

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#include <windows.h>
#else
#include <string.h>
#include <sys/time.h>
#include "rlgl.h"  // OpenGL abstraction layer to multiple versions

#define USE_UNIFORM
#ifdef USE_UNIFORM
#include <random>
#endif
long timeGetTime();
int timeBeginPeriod(unsigned int period);
int timeEndPeriod(unsigned int period);

#endif
bool exists(const std::string& filename);
std::string readFile(bool debug, const std::string& filename);

void MatMult(Matrix44& dest, const Matrix44& a, const Matrix44& b);
void MatScale(Matrix44& dest, sF32 sx, sF32 sy, sF32 sz);
void MatScale(Matrix44& dest, sF32 sx, sF32 sy, sF32 sz);
void MatTranslate(Matrix44& dest, sF32 tx, sF32 ty, sF32 tz);
void MatRotateZ(Matrix44& dest, sF32 angle);

GenTexture LinearGradient(sU32 startCol, sU32 endCol);
void Colorize(GenTexture& img, sU32 startCol, sU32 endCol);

bool SaveImage(GenTexture& img, const char* filename);
void randomVoronoi(GenTexture& dest,
                   const GenTexture& grad,
                   sInt intensity,
                   sInt maxCount,
                   sF32 minDist);
Texture2D LoadTextureFromGenTexture(GenTexture tex);

#endif  // UTILS_H
