//----------------------------------------------------------------------------//
// Overview: Processing of 8-link system
//----------------------------------------------------------------------------//

#include "8link.h"
#include "define.h"
#include "util.h"

// Macros
//----------------------------------------------------------------------------//
#define MAX_LENGTH (128)

// The pattern of the 8-link system is to divide vertically into its own area only
// |----|
//    |----|
// Each | | takes care of the space in between. Consider inside and outside separately in this case, dividing top and bottom

//----------------------------------------------------------------------------//
// A single column. Is this correct? Not being used?
//----------------------------------------------------------------------------//
template <typename PixelType>
static inline int CountLength(BlendingInfo<PixelType> *info, long target,
                              int NextPixelStepIn, int Min, int Max,
                              int LimitFromHere) {
  int Length = 0;
  int Sign = GET_SIGN(NextPixelStepIn); // Sign
  int LenDiff = Sign * 1;               // -1 or +1

  while (Min < Length + LimitFromHere && Length + LimitFromHere < Max) {
    long t = target + Length * NextPixelStepIn;

    Length += LenDiff;

    if (ComparePixel(t, t + NextPixelStepIn)) // Different color
      break;
  }

  return ABS(Length);
}

//----------------------------------------------------------------------------//
// Counting two columns simultaneously
//----------------------------------------------------------------------------//
template <typename PixelType>
static inline int CountLengthTwoLines(BlendingInfo<PixelType> *info,
                                      long target0, long target1,
                                      int NextPixelStepIn, int Min, int Max,
                                      int LimitFromHere, bool *t0_flg) {
  int Length = 0;
  int Sign = GET_SIGN(NextPixelStepIn); // Sign
  int LenDiff = Sign * 1;               // -1 or +1

  *t0_flg = false; // Flag indicating if t0 had a different color

  while (Min < Length + LimitFromHere && Length + LimitFromHere < Max) {
    long t0 = target0 + ABS(Length) * NextPixelStepIn,
         t1 = target1 + ABS(Length) * NextPixelStepIn;

    Length += LenDiff;

    if (ComparePixel(t0, t0 + NextPixelStepIn)) {
      *t0_flg = true;
      break;
    }

    if (ComparePixel(t1, t1 + NextPixelStepIn)) // Different color
    {
      break;
    }
  }

  return ABS(Length);
}

//----------------------------------------------------------------------------//
// Outside direction blending
//----------------------------------------------------------------------------//
template <typename PixelType>
void BlendOutside(
    BlendingInfo<PixelType> *info, //
    double length,                 // Length of this pattern
    long blend_target,             // Blend target (input)
    long out_target,               // Blend target (output)
    int ref_offset, // Blend reference target (input)
    int NextPixelStepIn, // Add this value to move to the next pixel (input)
    int NextPixelStepOut, // Add this value to move to the next pixel (output)
    bool ratio_invert, bool no_line_weight) {
  double len;      // Base of the whole
  int blend_count; // Number of pixels to blend, rounded up
  double pre_ratio = 0.0;

  // Line thickness
  if (no_line_weight)
    len = (length * 0.5);
  else
    len = (length * (double)info->LineWeight);

  blend_count = CEIL(len);

  // Since blending is done from the opposite direction of the boundary, move accordingly
  blend_target += (blend_count - 1) * NextPixelStepIn;
  out_target += (blend_count - 1) * NextPixelStepOut;

  // Whole: Triangle of the entire blend, None: Triangle of that pixel
  // Base x height ÷ 2 = base x ((base / whole base) x whole height) ÷ 2
  //        ↑Because it's a similar triangle
  // = l(base) * l(base) * 0.5(whole height) * 0.5(÷2) / len(whole base)
  int t;
  for (t = 0; t < blend_count; t++) {
    double l = len - (float)(((int)CEIL(len) - 1) -
                             t); // Base of this pixel, CEIL(len)-1 is
                                 // (1.000...1 ～ 2.0) -> want to make it 1.0
    double ratio = (l * l * 0.5 * 0.5) / len;
    double r;

    r = ratio_invert ? 1.0 - (ratio - pre_ratio) : (ratio - pre_ratio);

    // Blend
    Blendingf(info->in_ptr, info->out_ptr, blend_target,
              blend_target + ref_offset, out_target, (float)r);

    pre_ratio = ratio;

    // To the next pixel
    blend_target -= NextPixelStepIn;
    out_target -= NextPixelStepOut;
  }
}

//----------------------------------------------------------------------------//
// Inside direction blending
//----------------------------------------------------------------------------//
template <typename PixelType>
void BlendInside(
    PixelType TempPixel[2][MAX_LENGTH], int index,
    BlendingInfo<PixelType> *info,
    double length,     // Length of this pattern
    long blend_target, // Blend target (input)
    int ref_offset,    // Blend reference target (input)
    int NextPixelStepIn, // Add this value to move to the next pixel (input)
    bool ratio_invert, bool no_line_weight) {
  double len;      // Base of the whole
  int blend_count; // Number of pixels to blend, rounded up
  double pre_ratio = 0.0;

  // Line thickness
  if (no_line_weight)
    len = (length * 0.5);
  else
    len = (length * (double)info->LineWeight);

  blend_count = CEIL(len);

  // Since blending is done from the opposite direction of the boundary, move accordingly
  blend_target += (blend_count - 1) * NextPixelStepIn;

  int t;
  for (t = 0; t < blend_count; t++) {
    double l = len - (float)(((int)CEIL(len) - 1) -
                             t); // Base of this pixel, (int)CEIL(len)-1 is
                                 // (1.000...1 ～ 2.0) -> want to make it 1
    double ratio = (l * l * 1.0 * 0.5) /
                   len; // Consider height as 1.0 !! This is different (compared to Outside)!!
    double r;

    r = ratio_invert ? 1.0 - (ratio - pre_ratio) : (ratio - pre_ratio);

    // Blend !! This is different (compared to Outside) !!
    BlendingPixelf(&info->in_ptr[blend_target],
                   &info->in_ptr[blend_target + ref_offset],
                   &TempPixel[index][blend_count - 1 - t], (float)r);

    pre_ratio = ratio;

    // To the next pixel
    blend_target -= NextPixelStepIn;
  }
}

//----------------------------------------------------------------------------//
// Actual common execution function
//----------------------------------------------------------------------------//
template <typename PixelType>
static void
Link8Execute(BlendingInfo<PixelType> *info,
             int RefPixelStepIn,                         // 1
             int RefPixelStepOut,                        // 1
             int NextPixelStepIn,                        // width
             int NextPixelStepOut,                       // width
             int Min, int Max, int LimitFromHere,        // 0, height-1, info->j
             int AreaMin, int AreaMax, int AreaPosition, // 1, width-2, info->i
             int mode) {
  PixelType *in_ptr = info->in_ptr, *out_ptr = info->out_ptr;
  long in_target = info->in_target, out_target = info->out_target;

  int i;
  PixelType TempPixel[2][MAX_LENGTH];
  int TempLength;
  int Length[2]; // 0 : left or top  1 : right or bottom
  bool inside_flg[2] = {false, false};
  bool flag;

  // Counting left and right (or up and down) separately
  //--------------------------------------------------------------------------//
  // Left side
  Length[0] =
      CountLengthTwoLines(info, in_target, in_target - RefPixelStepIn,
                          NextPixelStepIn, Min, Max, LimitFromHere, &flag);

  // Right side
  Length[1] =
      CountLengthTwoLines(info, in_target, in_target + RefPixelStepIn,
                          NextPixelStepIn, Min, Max, LimitFromHere, &flag);

  // Clamp
  Length[0] = MIN(MAX_LENGTH, Length[0]);
  Length[1] = MIN(MAX_LENGTH, Length[1]);

  // Maximum length
  TempLength = MAX(Length[0], Length[1]);

  // Are both sides also closed positions?
  //--------------------------------------------------------------------------//
  bool ForceInsideFlag = false;
  if (AreaMin < AreaPosition && AreaPosition < AreaMax) {
    if (ComparePixel(in_target - RefPixelStepIn,
                     in_target - RefPixelStepIn * 2) &&
        ComparePixel(in_target + RefPixelStepIn,
                     in_target + RefPixelStepIn * 2)) { // Must be inside
      ForceInsideFlag = true;
    }
  }

  // Left (or top) side
  //--------------------------------------------------------------------------//
  // Outside? Inside?
  if (ComparePixelEqual(in_target,
                        in_target - NextPixelStepIn - RefPixelStepIn) &&
      !ForceInsideFlag) {
    bool flg = false;

    // Outside
    //------------------------------------------------------------------------//

    // Don't process if the target to be blended is also a closed position (outside only)
    if (AreaMin < AreaPosition && AreaPosition < AreaMax) {
      if (ComparePixelEqual(in_target - RefPixelStepIn,
                            in_target - RefPixelStepIn * 2)) {
        flg = true;
      }
    } else {
      flg = true;
    }

    if (flg) {
      BlendOutside(info, Length[0], info->in_target - RefPixelStepIn,
                   info->out_target - RefPixelStepOut, RefPixelStepIn,
                   NextPixelStepIn, NextPixelStepOut, true, true);
    }
  } else {
    // Inside
    //------------------------------------------------------------------------//
    BlendInside(TempPixel, 0, info, Length[0], info->in_target, -RefPixelStepIn,
                NextPixelStepIn, true, true);

    inside_flg[0] = true;
  }

  // Right (bottom) side
  //--------------------------------------------------------------------------//
  if (ComparePixelEqual(in_target,
                        in_target - NextPixelStepIn + RefPixelStepIn) &&
      !ForceInsideFlag) {
    bool flg = false;

    // Outside
    //------------------------------------------------------------------------//

    // Don't process if the target to be blended is also a closed position (outside only)
    if (AreaMin < AreaPosition && AreaPosition < AreaMax) {
      if (ComparePixelEqual(in_target + RefPixelStepIn,
                            in_target + RefPixelStepIn * 2)) {
        flg = true;
      }
    } else {
      flg = true;
    }

    if (flg) {
      BlendOutside(info, Length[1], info->in_target + RefPixelStepIn,
                   info->out_target + RefPixelStepOut, -RefPixelStepIn,
                   NextPixelStepIn, NextPixelStepOut, true, true);
    }
  } else {
    // Inside
    //------------------------------------------------------------------------//
    BlendInside(TempPixel, 1, info, Length[1], info->in_target, RefPixelStepIn,
                NextPixelStepIn, true, true);

    inside_flg[1] = true;
  }

  //----------------------------------------------------------------------------
  // Processing for cases where both sides are inside and their colors are different
  //   Γ
  //__Γ____ Pattern where three colors intersect
  //
  if (ComparePixel(in_target - RefPixelStepIn, in_target + RefPixelStepIn) &&
      inside_flg[0] == true && inside_flg[1] == true) {
    bool blend_flg = false;
    bool f[2] = {false, false};

    f[0] = ComparePixel(info->in_target - NextPixelStepIn,
                        info->in_target - NextPixelStepIn + RefPixelStepIn);
    f[1] = ComparePixel(info->in_target - NextPixelStepIn,
                        info->in_target - NextPixelStepIn - RefPixelStepIn);

    //--------------------------------------------------------------------------
    // Need to avoid collisions with upmode and downmode
    // 2 and 4 are half bad, 3 is all bad, 1 is all good
    // However, if it doesn't create a collision, then do it
    //--------------------------------------------------------------------------

    // Both sides have the same color, so there's no collision-inducing angle
    if (f[0] == false && f[1] == false) {
      blend_flg = true;
    }

    switch (mode) {
    case 2:
    case 4: {
      if ((mode == 2 && f[0] == false && f[1] == true) ||
          (mode == 4 && f[0] == false && f[1] == true)) {
        blend_flg = true;
      }
    } break;

    default:
    case 3:
      break;

    case 1:
      blend_flg = true;
      break;
    }

    if (blend_flg) {
      // Blend with the color considered to be the same as the color above (or to the right) of the protrusion
      // Blending with just one pixel can introduce noise
      double len = (double)MIN(Length[0], Length[1]);

      if (ComparePixelEqual(info->in_target - NextPixelStepIn,
                            info->in_target + RefPixelStepIn)) {
        BlendLine<PixelType>(info, len, in_target, out_target,
                             RefPixelStepIn, //
                             NextPixelStepIn, NextPixelStepOut, true, true);
      } else if (ComparePixelEqual(info->in_target - NextPixelStepIn,
                                   info->in_target - RefPixelStepIn)) {
        BlendLine<PixelType>(info, len, in_target, out_target,
                             -RefPixelStepIn, //
                             NextPixelStepIn, NextPixelStepOut, true, true);
      }
    }
  } else {
    // Average the inner sides and output
    //--------------------------------------------------------------------------
    int total_len, len[2];

    // Calculate the total length to be averaged
    total_len = CEIL((float)TempLength * 0.5f);

    // Calculate the lengths of each side to be averaged
    len[0] = CEIL((float)Length[0] * 0.5f);
    len[1] = CEIL((float)Length[1] * 0.5f);

    for (i = 0; i < total_len; i++) {

      // Both sides exist
      if ((i < len[0] && inside_flg[0] == true) &&
          (i < len[1] && inside_flg[1] == true)) {
            // Average the colors of corresponding pixels from both sides
        out_ptr[out_target + i * NextPixelStepOut].red =
            (TempPixel[0][i].red + TempPixel[1][i].red) / 2;
        out_ptr[out_target + i * NextPixelStepOut].green =
            (TempPixel[0][i].green + TempPixel[1][i].green) / 2;
        out_ptr[out_target + i * NextPixelStepOut].blue =
            (TempPixel[0][i].blue + TempPixel[1][i].blue) / 2;
        out_ptr[out_target + i * NextPixelStepOut].alpha =
            (TempPixel[0][i].alpha + TempPixel[1][i].alpha) / 2;
      } else if (i < len[0] && inside_flg[0] == true) { // Only side 0 exists
      // Blend the pixel from side 0 with the output using a blending factor of 0.5
        BlendingPixelf(&in_ptr[in_target + i * NextPixelStepIn],
                       &TempPixel[0][i],
                       &out_ptr[out_target + i * NextPixelStepOut], 0.5f);
      } else if (i < len[1] && inside_flg[1] == true) { // Only side 1 exists
      // Blend the pixel from side 1 with the output using a blending factor of 0.5
        BlendingPixelf(&in_ptr[in_target + i * NextPixelStepIn],
                       &TempPixel[1][i],
                       &out_ptr[out_target + i * NextPixelStepOut], 0.5f);
      }
    }
  }

  //--------------------------------------------------------------------------//
  // Patterns not handled by upmode and downmode
  // |_
  //  _|
  // |
  // Like this
  //--------------------------------------------------------------------------//
  if (ComparePixelEqual(in_target,
                        in_target + NextPixelStepIn - RefPixelStepIn) &&
      ComparePixelEqual(in_target,
                        in_target + NextPixelStepIn + RefPixelStepIn)) {
    switch (mode) {
    default:
    case 3:
      // Don't handle both up and down
      break;

    case 1:
      // Handle both up and down
      {
        int len[2] = {0, 0};
        // Count
        // Down
        len[0] = CountLengthTwoLines(
            info, in_target - RefPixelStepIn,
            in_target - RefPixelStepIn + NextPixelStepIn, -RefPixelStepIn,
            AreaMin, AreaMax, AreaPosition, &flag);

        // Blend
        BlendLine<PixelType>(info, len[0], in_target - RefPixelStepIn,
                             out_target - RefPixelStepOut,
                             -RefPixelStepIn + NextPixelStepIn, -RefPixelStepIn,
                             -RefPixelStepOut, true, true);

        // Count
        // Up
        len[1] = CountLengthTwoLines(
            info, in_target + RefPixelStepIn,
            in_target + RefPixelStepIn + NextPixelStepIn, RefPixelStepIn,
            AreaMin, AreaMax, AreaPosition, &flag);
        // Blend
        BlendLine<PixelType>(info, len[1], in_target + RefPixelStepIn,
                             out_target + RefPixelStepOut,
                             RefPixelStepIn + NextPixelStepIn, RefPixelStepIn,
                             RefPixelStepOut, true, true);
      }
      break;

    case 2:
    case 4:
      // Only handle right side
      {
        int len;

        // Count
        len = CountLengthTwoLines(info, in_target + RefPixelStepIn,
                                  in_target + RefPixelStepIn + NextPixelStepIn,
                                  RefPixelStepIn, AreaMin, AreaMax,
                                  AreaPosition, &flag);
        // Blend
        BlendLine<PixelType>(info, len, in_target + RefPixelStepIn,
                             out_target + RefPixelStepOut,
                             RefPixelStepIn + NextPixelStepIn, RefPixelStepIn,
                             RefPixelStepOut, true, true);
      }
      break;
    }
  }
}

//----------------------------------------------------------------------------//
// Main function for Mode 2
// Since it's possible that two inference lines are affecting a single pixel,
// the image is divided vertically into two parts, and the pixel values of each part are averaged to get the result.
//----------------------------------------------------------------------------//
template <typename PixelType>
void Link8Mode02Execute(BlendingInfo<PixelType> *info) {

  int in_width = GET_WIDTH(info->input), in_height = GET_HEIGHT(info->input),
      out_width = GET_WIDTH(info->output);

  Link8Execute(info, 1, 1, in_width, out_width, 0, in_height - 1, info->j, 1,
               in_width - 2, info->i, 2);
}

//----------------------------------------------------------------------------//
// Main function for Mode 4
//----------------------------------------------------------------------------//
template <typename PixelType>
void Link8Mode04Execute(BlendingInfo<PixelType> *info) {
  int in_width = GET_WIDTH(info->input), in_height = GET_HEIGHT(info->input),
      out_width = GET_WIDTH(info->output);

  Link8Execute(info, 1, 1, -in_width, -out_width, 0, in_height - 1, info->j, 1,
               in_width - 2, info->i, 4);
}

//----------------------------------------------------------------------------//
// Main function for Mode 1
//----------------------------------------------------------------------------//
template <typename PixelType>
void Link8Mode01Execute(BlendingInfo<PixelType> *info) {
  int in_width = GET_WIDTH(info->input), in_height = GET_HEIGHT(info->input),
      out_width = GET_WIDTH(info->output);

  Link8Execute(info, -in_width, -out_width, -1, -1, 0, in_width - 1, info->i, 1,
               in_height - 2, info->j, 1);
}

//----------------------------------------------------------------------------//
// Main function for Mode 3
//----------------------------------------------------------------------------//
template <typename PixelType>
void Link8Mode03Execute(BlendingInfo<PixelType> *info) {
  int in_width = GET_WIDTH(info->input), in_height = GET_HEIGHT(info->input),
      out_width = GET_WIDTH(info->output);

  Link8Execute(info, -in_width, -out_width, 1, 1, 0, in_width - 1, info->i, 1,
               in_height - 2, info->j, 3);
}

//----------------------------------------------------------------------------//
// Mode
//----------------------------------------------------------------------------//
template <typename PixelType>
void Link8SquareBlendOutside(BlendingInfo<PixelType> *info, long in_target,
                             long out_target, int ref_offset,
                             int next_pixel_step_in, int next_pixel_step_out,
                             int min, int max, int limit_from_here) {
  int count;
  bool no_line_weight;

  count = CountLengthTwoLines(info, in_target, in_target + ref_offset,
                              next_pixel_step_in, min, max, limit_from_here,
                              &no_line_weight);

  BlendLine<PixelType>(info, (double)count, in_target, out_target, ref_offset,
                       next_pixel_step_in, next_pixel_step_out, true,
                       no_line_weight);
}

//----------------------------------------------------------------------------//
// Overview:
// Function Name:
// Arguments:
// Return Value:
//----------------------------------------------------------------------------//
template <typename PixelType>
void Link8SquareExecute(BlendingInfo<PixelType> *info) {
  PixelType *in_ptr = info->in_ptr;
  int in_width = GET_WIDTH(info->input), out_width = GET_WIDTH(info->output),
      in_height = GET_HEIGHT(info->input);
  int i;
  unsigned int flg = 0;

  // First, check the 8-link state
  if (ComparePixelEqual(info->in_target, info->in_target - in_width - 1))
    flg |= (1 << 0); // Pattern 0
  if (ComparePixelEqual(info->in_target, info->in_target - in_width + 1))
    flg |= (1 << 1); // Pattern 1
  if (ComparePixelEqual(info->in_target, info->in_target + in_width + 1))
    flg |= (1 << 2); // Pattern 2
  if (ComparePixelEqual(info->in_target, info->in_target + in_width - 1))
    flg |= (1 << 3); // Pattern 3

  // Pixel of self ------------------------------------------------------------
  {
    PixelType temp_pixel[4];
    int ref_tbl[4];
    int sum_color[4];

    // Initialization
    ref_tbl[0] = -in_width - 1;
    ref_tbl[1] = -in_width + 1;
    ref_tbl[2] = in_width + 1;
    ref_tbl[3] = in_width - 1;

    for (i = 0; i < 4; i++)
      sum_color[i] = 0;

    // Processing
    for (i = 0; i < 4; i++) {
      temp_pixel[i] = in_ptr[info->in_target];

      if (!(flg & (1 << i))) {
        BlendingPixelf(&in_ptr[info->in_target],
                       &in_ptr[info->in_target + ref_tbl[i]], &temp_pixel[i],
                       0.5f);
      }

      // Sum up in passing
      sum_color[0] += temp_pixel[i].red;
      sum_color[1] += temp_pixel[i].green;
      sum_color[2] += temp_pixel[i].blue;
      sum_color[3] += temp_pixel[i].alpha;
    }

    // Output
    info->out_ptr[info->out_target].red = sum_color[0] / 4;
    info->out_ptr[info->out_target].green = sum_color[1] / 4;
    info->out_ptr[info->out_target].blue = sum_color[2] / 4;
    info->out_ptr[info->out_target].alpha = sum_color[3] / 4;
  }

  // ← -------------------------------------------------------------------------
  // Count
  if ((flg & 0x9) !=
      0x9) // If both were linked, process with other patterns. Not pattern 0&3
  {

    if (flg & (1 << 0)) // Only pattern 0
    {
      Link8SquareBlendOutside(info, info->in_target - 1, info->out_target - 1,
                              -in_width, -1, -1, 1, in_width - 2, info->i);
    } else if (flg & (1 << 3)) // Only pattern 3
    {
      Link8SquareBlendOutside(info, info->in_target - 1, info->out_target - 1,
                              in_width, -1, -1, 1, in_width - 2, info->i);
    }
  }

  // ↑ -------------------------------------------------------------------------
  if ((flg & 0x3) !=
      0x3) // If both were linked, process with other patterns. Not pattern 0&1
  {

    if (flg & (1 << 0)) // Only pattern 0
    {
      Link8SquareBlendOutside(info, info->in_target - in_width,
                              info->out_target - out_width, -1, -in_width,
                              -out_width, 1, in_height - 2, info->j);
    } else if (flg & (1 << 1)) // Only pattern 1
    {
      Link8SquareBlendOutside(info, info->in_target - in_width,
                              info->out_target - out_width, 1, -in_width,
                              -out_width, 1, in_height - 2, info->j);
    }
  }

  // → -------------------------------------------------------------------------
  if ((flg & 0x6) !=
      0x6) // If both were linked, process with other patterns. Not pattern 1&2
  {
    if (flg & (1 << 1)) // Only pattern 1
    {
      Link8SquareBlendOutside(info, info->in_target + 1, info->out_target + 1,
                              -in_width, 1, 1, 1, in_width - 2, info->i);
    } else if (flg & (1 << 2)) // Only pattern 2
    {
      Link8SquareBlendOutside(info, info->in_target + 1, info->out_target + 1,
                              in_width, 1, 1, 1, in_width - 2, info->i);
    }
  }

  // ↓ -------------------------------------------------------------------------
  if ((flg & 0xc) !=
      0xc) // If both were linked, process with other patterns. Not pattern 2&3
  {

    if (flg & (1 << 2)) // Only pattern 2
    {
      Link8SquareBlendOutside(info, info->in_target + in_width,
                              info->out_target + out_width, 1, in_width,
                              out_width, 1, in_height - 2, info->j);
    } else if (flg & (1 << 3)) // Only pattern 3
    {
      Link8SquareBlendOutside(info, info->in_target + in_width,
                              info->out_target + out_width, -1, in_width,
                              out_width, 1, in_height - 2, info->j);
    }
  }
}

// Explicit instantiation
template void Link8Mode01Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *pInfo);
template void Link8Mode01Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *pInfo);

template void Link8Mode02Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *pInfo);
template void Link8Mode02Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *pInfo);

template void Link8Mode03Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *pInfo);
template void Link8Mode03Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *pInfo);

template void Link8Mode04Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *pInfo);
template void Link8Mode04Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *pInfo);

template void Link8SquareExecute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void Link8SquareExecute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);
