#ifndef __UTIL_H
#define __UTIL_H

#include <math.h>
#include <string>

#include "AEConfig.h"
#include "A.h"
#include "AE_Effect.h"
#include "AE_Macros.h"
#include "version.h"

#include "define.h"

#ifdef AE_OS_WIN
#include <windows.h>
#endif

//----------------------------------------------------------------------------//
// Error handling
#define ACALL(fct)                                                             \
  {                                                                            \
    if (err == A_Err_NONE)                                                     \
      if (A_Err_NONE != (err = (fct)))                                         \
        throw APIErr(__FILE__, __LINE__, err);                                 \
  }

struct APIErr {
  A_Err Err;
  std::string FileName;
  int Line;

  APIErr(char *filename, int line, A_Err err) {
    Err = err;
    FileName = filename;
    Line = line;
  };
};

void PrintAPIErr(APIErr *perr);

//----------------------------------------------------------------------------//
// Debug file
#ifdef _DEBUG

#include <stdarg.h>

void DebugPrint(char *format, ...);

#define DEBUG_STR(str) OutputDebugStringA((str));

#else

#define DEBUG_STR(str) ((void)0)

#endif /* _DEBUG */

#ifdef AE_OS_WIN

#else

// Mac
#include <stdarg.h>
void DebugPrint(char *format, ...);

#endif

//----------------------------------------------------------------------------//
// Speed measurement
#define _PROFILE (0)

#if _PROFILE

struct ProfileData {
  bool isCounting;
  LARGE_INTEGER lapStart;
  LARGE_INTEGER sum;
  int lapCount;

  ProfileData() {
    isCounting = false;
    lapStart.QuadPart = 0LL;
    sum.QuadPart = 0LL;
    lapCount = 0;
  }
};
void BeginProfile();
void EndProfile();
void BeginProfileLap(int index);
void EndProfileLap(int index);

#define BEGIN_PROFILE() BeginProfile()
#define END_PROFILE() EndProfile()
#define BEGIN_LAP(x) BeginProfileLap((x))
#define END_LAP(x) EndProfileLap((x))

#else

#define BEGIN_PROFILE() ((void)0)
#define END_PROFILE() ((void)0)
#define BEGIN_LAP(x) ((void)0)
#define END_LAP(x) ((void)0)

#endif /* _DEBUG */

//----------------------------------------------------------------------------//
// Image processing utils
//----------------------------------------------------------------------------//

#define GET_WIDTH(image)                                                       \
  ((image)->rowbytes / sizeof(PixelType)) // rowbytes = number of horizontal
                                          // bytes 4 bytes per pixel, so /4
#define GET_HEIGHT(image) ((image)->height)

typedef unsigned long KP_PIXEL32;      // 8bbit per channel
typedef unsigned long long KP_PIXEL64; // 16bit per channel

template <typename PixelType> static inline unsigned int getMaxValue() {
  return ~0;
}
template <> inline unsigned int getMaxValue<PF_Pixel16>() { return 0x8000; }
template <> inline unsigned int getMaxValue<PF_Pixel8>() { return 0xff; }

//----------- Arithmetic system ------------//
#define CEIL(a) (int)ceil(a) // Float rounding up

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b)) // defined in AfterEffects header?
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#define GET_SIGN(a)                                                            \
  ((a) / ABS((a))) // Get sign -1 if negative, +1 if positive

//----------------------------------------------------------------------------//
// Functions
//----------------------------------------------------------------------------//

//-------------------------------------//
//     Pixel Comparison Functions
//-------------------------------------//

#define FAST_COMPARE_PIXEL(p1, p2)                                             \
  (*(PackedPixelType *)&in_ptr[p1] != *(PackedPixelType *)&in_ptr[p2])

#ifdef AE_OS_WIN
#define ComparePixel(p0, p1)                                                   \
  RangeComparePixelNotEqual(&(info->in_ptr[p0]), &(info->in_ptr[p1]),          \
                            info->range)
#define ComparePixelEqual(p0, p1)                                              \
  RangeComparePixelEqual(&(info->in_ptr[p0]), &(info->in_ptr[p1]), info->range)

//----------------------------------------------------------------------------//
// Pixel Comparison Functions
//----------------------------------------------------------------------------//
template <typename PixelType>
static inline bool RangeComparePixelNotEqual(const PixelType *p0,
                                             const PixelType *p1,
                                             const unsigned int range) {
  unsigned int delta;
  delta = ABS(p0->red - p1->red) + ABS(p0->green - p1->green) +
          ABS(p0->blue - p1->blue) + ABS(p0->alpha - p1->alpha);

  return (delta > range);
}

template <typename PixelType>
static inline bool RangeComparePixelEqual(const PixelType *p0,
                                          const PixelType *p1,
                                          const unsigned int range) {
  unsigned int delta;
  delta = ABS(p0->red - p1->red) + ABS(p0->green - p1->green) +
          ABS(p0->blue - p1->blue) + ABS(p0->alpha - p1->alpha);

  return (delta <= range);
}
#else
// for Mac
// Defines the ComparePixel and ComparePixelEqual macros for comparing pixel
// values. Note: These may not be expanded inline as expected by the compiler.
#define ComparePixel(p0, p1)                                                   \
  ((unsigned int)(ABS(info->in_ptr[p0].red - info->in_ptr[p1].red) +           \
                  ABS(info->in_ptr[p0].green - info->in_ptr[p1].green) +       \
                  ABS(info->in_ptr[p0].blue - info->in_ptr[p1].blue) +         \
                  ABS(info->in_ptr[p0].alpha - info->in_ptr[p1].alpha)) >      \
   info->range)

#define ComparePixelEqual(p0, p1)                                              \
  ((unsigned int)(ABS(info->in_ptr[p0].red - info->in_ptr[p1].red) +           \
                  ABS(info->in_ptr[p0].green - info->in_ptr[p1].green) +       \
                  ABS(info->in_ptr[p0].blue - info->in_ptr[p1].blue) +         \
                  ABS(info->in_ptr[p0].alpha - info->in_ptr[p1].alpha)) <=     \
   info->range)
#endif

// Detecting gradation
// If the average value of three pixels and the center pixel is less than or
// equal to the range, return true. It is assumed that ref1 and center are
// different colors.
template <typename PixelType>
static inline bool DetectGradation(const PixelType *center,
                                   const PixelType *ref1, const PixelType *ref2,
                                   const unsigned int range) {
  // Create an average pixel
  PixelType ave;
  ave.red = (center->red + ref1->red + ref2->red) / 3;
  ave.blue = (center->blue + ref1->blue + ref2->blue) / 3;
  ave.green = (center->green + ref1->green + ref2->green) / 3;
  ave.alpha = (center->alpha + ref1->alpha + ref2->alpha) / 3;

  // The center and the average are the same, and the other two are different
  // colors
  return RangeComparePixelEqual(center, &ave, 1 * 255 * 4);
}

//----------------------------------------------------------------------------//
// Overview: Blending pixels together
// Arguments: input, output: Familiar pointers to input and output images
//            blend_target: The target to blend. The result goes into this pixel
//            ref_target: The second target to blend. The blend will be between
//            this and the above target ratio: The blending ratio. 1.0f will
//            result in the same as blend_target 0.0f will result in ref
//----------------------------------------------------------------------------//
template <typename PixelType>
static inline void BlendingPixelf(PixelType *target_pixel, PixelType *ref_pixel,
                                  PixelType *output_pixel, float ratio) {
  unsigned int max_value = getMaxValue<PixelType>();
  unsigned int alpha = (unsigned int)(max_value * ratio), r_alpha;

  r_alpha = max_value - alpha;

  if (target_pixel->alpha == max_value && ref_pixel->alpha == max_value) {
    // Both are opaque
    output_pixel->alpha = max_value;

    output_pixel->red =
        (((target_pixel->red * alpha) + (ref_pixel->red * r_alpha)) /
         max_value);
    output_pixel->green =
        (((target_pixel->green * alpha) + (ref_pixel->green * r_alpha)) /
         max_value);
    output_pixel->blue =
        (((target_pixel->blue * alpha) + (ref_pixel->blue * r_alpha)) /
         max_value);
  } else if (target_pixel->alpha == 0) {
    // The target is transparent
    output_pixel->alpha =
        (((target_pixel->alpha * alpha) + (ref_pixel->alpha * r_alpha)) /
         max_value);

    output_pixel->red = ref_pixel->red;
    output_pixel->green = ref_pixel->green;
    output_pixel->blue = ref_pixel->blue;

  } else if (ref_pixel->alpha == 0) {
    // The ref is transparent
    output_pixel->alpha =
        (((target_pixel->alpha * alpha) + (ref_pixel->alpha * r_alpha)) /
         max_value);

    output_pixel->red = target_pixel->red;
    output_pixel->green = target_pixel->green;
    output_pixel->blue = target_pixel->blue;
  } else {
    // Semi-transparent
    output_pixel->alpha =
        (((target_pixel->alpha * alpha) + (ref_pixel->alpha * r_alpha)) /
         max_value);
    output_pixel->red =
        (((target_pixel->red * alpha) + (ref_pixel->red * r_alpha)) /
         max_value);
    output_pixel->green =
        (((target_pixel->green * alpha) + (ref_pixel->green * r_alpha)) /
         max_value);
    output_pixel->blue =
        (((target_pixel->blue * alpha) + (ref_pixel->blue * r_alpha)) /
         max_value);
  }
}

// Float version of blending instruction
template <typename PixelType>
static inline void Blendingf(PixelType *in_ptr, PixelType *out_ptr,
                             long blend_target, long ref_target,
                             long output_target, float ratio) {
  BlendingPixelf<PixelType>(&(in_ptr[blend_target]), &(in_ptr[ref_target]),
                            &(out_ptr[output_target]), ratio);
}

// Gamma table creation
void CreateGanmmaTable(u_char table[256], float Ganmma);

// Debug Color Type
template <typename PixelType>
void SetDebugPixel(PixelType *out_ptr, PF_LayerDef *output, int x, int y);
template <typename PixelType>
void SetDebugPixel(PixelType *out_ptr, PF_LayerDef *output, long target);

#ifdef _DEBUG
#define DEBUG_PIXEL(out_ptr, output, x, y)                                     \
  SetDebugPixel((out_ptr), (output), (x), (y))
#define DEBUG_TARGET_PIXEL(out_ptr, output, t)                                 \
  SetDebugPixel((out_ptr), (output), (t))
#else
#define DEBUG_PIXEL(out_ptr, output, x, y) ((void)0)
#define DEBUG_TARGET_PIXEL(out_ptr, output, t) ((void)0)
#endif

template <typename PixelType>
void BlendLine(
    BlendingInfo<PixelType> *pinfo, //
    double length,                  // Length of this pattern
    long blend_target,              // Target of blend source (input)
    long out_target,                // Target for blending destination (output)
    int ref_offset, // Target of blend reference (input)
    int next_pixel_step_in, // Add this value when moving to the next pixel (input)
    int next_pixel_step_out, // Add this value when moving to the next pixel (output)
    bool ratio_invert, bool no_line_weight);

//----------------------------------------------------------------------------//
// Util
//----------------------------------------------------------------------------//
#define SAFE_DELETE(x)                                                         \
  if ((x) != NULL) {                                                           \
    delete (x);                                                                \
    (x) = NULL;                                                                \
  }

#endif
