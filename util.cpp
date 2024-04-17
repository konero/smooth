
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

//----------------------------------------------------------------------------//
// Overview:
// Function Name:
// Arguments:
// Return Value:
//----------------------------------------------------------------------------//
void PrintAPIErr(APIErr *perr) {

#ifdef AE_OS_WIN
  wchar_t str[1024];
  swprintf(str, 1024, L"!! AEGP Err ( code : %d   file : %hs   line : %d )",
           perr->Err, perr->FileName.c_str(), perr->Line);
  OutputDebugString(str);
#else
  char str[1024];
  snprintf(str, 1024, "!! AEGP Err ( code : %d   file : %s   line : %d )",
           perr->Err, perr->FileName.c_str(), perr->Line);
  printf("%s", str);
#endif
}

//----------------------------------------------------------------------------//
// DEBUG system

#ifdef _DEBUG

void DebugPrint(char *format, ...) {
  char str[1024];
  va_list args;

  va_start(args, format);

  vsnprintf(str, 1024, format, args);

  va_end(args);

  OutputDebugStringA(str);
}

#endif /* _DEBUG */

//----------------------------------------------------------------------------//
// Speed measurement

#if _PROFILE
#ifdef AE_OS_WIN

#define MAX_PROFILE_INDEX (16)
ProfileData Profiles[MAX_PROFILE_INDEX];

static LARGE_INTEGER StartCounter = {0LL};

void BeginProfile() { QueryPerformanceCounter(&StartCounter); }

void EndProfile() {
  LARGE_INTEGER now, freq;
  double time;
  double overhead;

  if (QueryPerformanceCounter(&now) && QueryPerformanceFrequency(&freq)) {
    time =
        (double)(now.QuadPart - StartCounter.QuadPart) / (double)freq.QuadPart;

    wchar_t str[1024];
    swprintf(str, 1024, L"total : %f ms\n", time * 1000.0);
    OutputDebugString(str);

    for (int i = 0; i < MAX_PROFILE_INDEX; i++) {
      ProfileData *profile = &Profiles[i];

      if (profile->isCounting) {
        time = (double)(profile->sum.QuadPart) / (double)freq.QuadPart;

        // オーバーヘッド計測
        profile->isCounting = false;

        int lapcount = profile->lapCount;

        for (int t = 0; t < lapcount; t++) {
          BeginProfileLap(i);
          EndProfileLap(i);
        }

        overhead = (double)(profile->sum.QuadPart) / (double)freq.QuadPart;

        swprintf(str, 1024, L"%02d : %f ms  overhead= %f ms  %d\n", i,
                 time * 1000.0, overhead * 1000.0, profile->lapCount);
        OutputDebugString(str);
      }

      profile->isCounting = false;
    }
  } else {
    OutputDebugString(L"time : err\n");
  }
}

void BeginProfileLap(int index) {
  ProfileData *profile = &Profiles[index];

  if (profile->isCounting == false) {
    profile->isCounting = true;
    profile->sum.QuadPart = 0LL;
    profile->lapCount = 0;
  }

  QueryPerformanceCounter(&profile->lapStart); // Start time
}

void EndProfileLap(int index) {
  ProfileData *profile = &Profiles[index];

  if (profile->isCounting == true) {
    LARGE_INTEGER now;

    QueryPerformanceCounter(&now);

    profile->sum.QuadPart += now.QuadPart - profile->lapStart.QuadPart;
    profile->lapCount++;
  }
}

#else

#endif /* AE_OS_WIN */
#endif /* _DEBUG */

//----------------------------------------------------------------------------//
// Image Processing Related

//----------------------------------------------------------------------------//
// Overview:  Creation of a gamma table
// Arguments: table : pointer to table
//----------------------------------------------------------------------------//
void CreateGanmmaTable(u_char table[256], float Ganmma) {
  int t;

  for (t = 0; t < 256; t++) {
    table[t] = (u_char)(255.0f * pow(((float)t / 255.0f), 1.0f / Ganmma));
  }
}

//----------------------------------------------------------------------------//
// Name: 			SetDebugPixel()
// Arguments: 		output : output image
// x: 				x-coordinate
// y: 				y-coordinate
// Return value: 	none
// Overview: 		Place a color at the specified coordinates for debugging
//----------------------------------------------------------------------------//
template <typename PixelType> static inline void getDebugPixel(PixelType *p) {
  p->red = 255;
  p->green = 0;
  p->blue = 0;
  p->alpha = 255;
}

template <> inline void getDebugPixel<PF_Pixel16>(PF_Pixel16 *p) {
  p->red = 0x8000;
  p->green = 0;
  p->blue = 0;
  p->alpha = 0x8000;
}

template <typename PixelType>
void SetDebugPixel(PixelType *out_ptr, PF_LayerDef *output, int x, int y) {
  PixelType debug_pixel;
  getDebugPixel(&debug_pixel);

  out_ptr[y * GET_WIDTH(output) + x] = debug_pixel;
}

template <typename PixelType>
void SetDebugPixel(PixelType *out_ptr, PF_LayerDef *output, long target) {
  PixelType debug_pixel;
  getDebugPixel(&debug_pixel);

  out_ptr[target] = debug_pixel;
}

// Explicit instantiation
template void SetDebugPixel<PF_Pixel16>(PF_Pixel16 *out_ptr,
                                        PF_LayerDef *output, int x, int y);
template void SetDebugPixel<PF_Pixel8>(PF_Pixel8 *out_ptr, PF_LayerDef *output,
                                       int x, int y);

template void SetDebugPixel<PF_Pixel16>(PF_Pixel16 *out_ptr,
                                        PF_LayerDef *output, long target);
template void SetDebugPixel<PF_Pixel8>(PF_Pixel8 *out_ptr, PF_LayerDef *output,
                                       long target);

//----------------------------------------------------------------------------//
// Overview   : Blends a line of pixels
// Function Name : BlendLine
// Arguments   :
//    pinfo: Information for blending
//    length: The length of this pattern
//    blend_target: The target pixel to blend from (input)
//    out_target: The target pixel to blend to (output)
//    ref_offset: The target pixel to refer to for blending (input)
//    next_pixel_step_in: The value to add when moving to the next pixel (input)
//    next_pixel_step_out: The value to add when moving to the next pixel
//    (output) ratio_invert: If true, inverts the blending ratio no_line_weight:
//    If true, does not consider line weight in blending
// Return Value : None
//----------------------------------------------------------------------------//
template <typename PixelType>
void BlendLine(
    BlendingInfo<PixelType> *pinfo, //
    double length,                  // The length of this pattern
    long blend_target,              // The target pixel to blend from (input)
    long out_target,                // The target pixel to blend to (output)
    int ref_offset,         // The target pixel to refer to for blending (input)
    int next_pixel_step_in, // The value to add when moving to the next pixel
                            // (input)
    int next_pixel_step_out, // The value to add when moving to the next pixel
                             // (output)
    bool ratio_invert, bool no_line_weight) {
  double len;
  int blend_count; // The number of pixels to blend, rounded up
  double pre_ratio = 0.0;

  if (no_line_weight)
    len = (length * 0.5); // The base of the entire triangle
  else
    len =
        (length * (double)pinfo->LineWeight); // The base of the entire triangle

  blend_count = CEIL(len);

  // Since blending is done from the boundary in the opposite direction, move
  // accordingly //
  blend_target += (blend_count - 1) * next_pixel_step_in;
  out_target += (blend_count - 1) * next_pixel_step_out;

  // The entire ~ : The entire triangle for blending, none : The triangle for
  // that pixel Base × height ÷ 2 = Base × ((Base / entire base) × entire
  // height) ÷ 2
  //                          ↑ Because it's a similar triangle
  // = l(base) * l(base) * 0.5(entire height) * 0.5(÷2) / len(entire base)
  int t;
  for (t = 0; t < blend_count; t++) {
    double l = len - (float)(((int)CEIL(len) - 1) -
                             t); // The base at this pixel, CEIL(len)-1 is
                                 // (1.000...1 ~ 2.0) -> want to make it 1.0
    double ratio = (l * l * 0.5 * 0.5) / len;
    double r;

    r = ratio_invert ? 1.0 - (ratio - pre_ratio) : (ratio - pre_ratio);

    // Blend
    Blendingf(pinfo->in_ptr, pinfo->out_ptr, blend_target,
              blend_target + ref_offset, out_target, (float)r);

    pre_ratio = ratio;

    // To the next pixel
    blend_target -= next_pixel_step_in;
    out_target -= next_pixel_step_out;
  }
}

// Explicit instantiation declarations
template void BlendLine<PF_Pixel8>(BlendingInfo<PF_Pixel8> *pinfo,
                                   double length, long blend_target,
                                   long out_target, int ref_offset,
                                   int next_pixel_step_in,
                                   int next_pixel_step_out, bool ratio_invert,
                                   bool no_line_weight);

template void BlendLine<PF_Pixel16>(BlendingInfo<PF_Pixel16> *pinfo,
                                    double length, long blend_target,
                                    long out_target, int ref_offset,
                                    int next_pixel_step_in,
                                    int next_pixel_step_out, bool ratio_invert,
                                    bool no_line_weight);
