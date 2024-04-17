//----------------------------------------------------------------------------//
// Upper-Angle System Processing
//----------------------------------------------------------------------------//

#include "define.h"
#include "util.h"

#include "upMode.h"

#include <math.h>
#include <stdio.h>

//----------------------------------------------------------------------------//
// LEFT
//
// Name:     upMode_LeftCountLength()
// Overview: Calculates the length of the left side for anti-aliasing blending.
// Arguments:
//    - info: Pointer to a structure containing count information.
// Return Value: None
//----------------------------------------------------------------------------//
template <typename PixelType>
void upMode_LeftCountLength(BlendingInfo<PixelType> *info) {
  PF_LayerDef *output = info->output;

  long count_target = 0;
  int len = 1, width = GET_WIDTH(info->input), height = output->height;
  u_int *flg = &info->core[0].flg;

  // Continuously check until reaching x = 0 (leftmost point of the image)
  while (1) {
    count_target =
        info->in_target - (len - 1); // Change the target to be examined

    // Is it a flat fill? Check the bottom side
    if (ComparePixel(count_target, count_target - 1)) {

      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[0].start = (float)(info->i + 1);
      info->core[0].end = (float)(info->i + 1) - (float)len;

      // Set a flag for special processing //!?
      (*flg) |= CR_FLG_FILL;

      break;
    }

    count_target =
        info->in_target - width - (len - 1); // Change the target to be examined

    // Is it a flat fill? Check the top side
    if (ComparePixel(count_target, count_target - 1)) {

      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[0].start = (float)(info->i + 1);
      info->core[0].end = (float)(info->i + 1) - (float)len;

      if (width - 2 > info->i && info->i > 2 && height - 2 > info->j &&
          info->j > 2) {
        // Is end value correction necessary?
        // Check the length of the upper corner on the line, and if the
        // difference is 1, correct it
        if (!(info->flag & SECOND_COUNT) &&
            ComparePixel(count_target - 1,
                         count_target - 1 - width)) // Check if it's a corner
        {
          // Count here
          BlendingInfo<PixelType> sc_info;

          sc_info = *info; // Copy and initialize

          sc_info.i = info->i - len;
          sc_info.j = info->j - 1;
          sc_info.out_target = sc_info.in_target =
              sc_info.j * width + sc_info.i;
          sc_info.flag = SECOND_COUNT;

          upMode_LeftCountLength<PixelType>(&sc_info);

          if (sc_info.core[0].length - len == 1) {
            info->core[0].end -=
                0.5f; // Adjusting for a half-pixel offset to the left
          }

#if 0
          if(len - sc_info.core[0].length == 1)
          {
            // Correct with half a pixel to the right
            info->core[0].end -= 0.5f;
          }
#endif
        }
      }

      break;
    }

    len++;

    // Check if x = 0 (leftmost point of the image) is reached
    if (info->i - len <= 1) {
      len = info->i - 1;

      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[0].start = (float)(info->i + 1);
      info->core[0].end = (float)(info->i + 1) - (float)len;
      break;
    }
  }

  info->core[0].length = len;
}

//----------------------------------------------------------------------------//
// RIGHT
//
// Name:     upMode_RightCountLength()
// Overview: Calculates the length of the right side for anti-aliasing blending.
// Arguments:
//    - info: Pointer to a structure containing count information.
// Return Value: None
//----------------------------------------------------------------------------//
template <typename PixelType>
void upMode_RightCountLength(BlendingInfo<PixelType> *info) {
  PF_LayerDef *output = info->output;

  long count_target = 0;
  int len = 0, width = GET_WIDTH(info->input), height = output->height;
  u_int *flg = &info->core[1].flg;

  // Start by checking only the left side
  count_target = info->in_target + width; // Change the target to be examined

  // Check if it's a flat fill and handle it
  if (ComparePixel(count_target, count_target + 1) &&
      ComparePixelEqual(info->in_target + 1, info->in_target + 1 + width)) {
    info->core[1].length = 0;
    return;
  } else {
    len++;

    // Check if reaching x = 0 (leftmost point of the image)
    if ((info->i + 1) + len >= (width - 1)) {
      len = width - 1 - (info->i + 1);

      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[1].start = (float)(info->i + 1);
      info->core[1].end = (float)(info->i + 1) + (float)len;
      info->core[1].length = len;
      return;
    }
  }

  // Continue checking until reaching x = 0 (leftmost point of the image)
  while (1) {
    count_target = info->in_target + len; // Change the target to be examined

    // Is it a flat fill?
    if (ComparePixel(count_target, count_target + 1)) {
      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[1].start = (float)(info->i + 1);
      info->core[1].end = (float)(info->i + 1 + len);

      // Set a flag for special processing //!?
      (*flg) |= CR_FLG_FILL;

      break;
    }

    count_target =
        info->in_target + width + len; // Change the target to be examined

    if (ComparePixel(count_target, count_target + 1)) {
      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[1].start = (float)(info->i + 1);
      info->core[1].end = (float)(info->i + 1 + len);

      if (width - 2 > info->i && info->i > 2 && height - 2 > info->j &&
          info->j > 2) {
        // Is end value correction necessary?
        // Check the length of the lower corner on the line, and if the
        // difference is 1, correct it Check if it's a corner
        if (!(info->flag & SECOND_COUNT) &&
            ComparePixel(count_target, count_target + 1)) // is it a corner?
        {
          // Count here
          BlendingInfo<PixelType> sc_info;

          sc_info = *info; // Copy and initialize

          sc_info.i = info->i + len;
          sc_info.j = info->j + 1;
          sc_info.out_target = sc_info.in_target =
              sc_info.j * width + sc_info.i;
          sc_info.flag = SECOND_COUNT;

          upMode_RightCountLength<PixelType>(&sc_info);

          // Adjust for a half-pixel offset to the right if needed
          if (len - sc_info.core[1].length == 1 &&
              sc_info.core[1].length != 0) {
            info->core[1].end -= 0.5f;
          }
        }
      }

      break;
    }

    len++;

    // Check if x = 0 (leftmost point of the image) is reached
    if ((info->i + 1) + len >= (width - 1)) {
      len = width - 1 - (info->i + 1);

      // Image coordinates start from (0,0) at top-left, but logical line starts
      // from the right, hence +1
      info->core[1].start = (float)(info->i + 1);
      info->core[1].end = (float)(info->i + 1) + (float)len;
      break;
    }
  }

  // Copy the calculated length
  info->core[1].length = len;
}

//----------------------------------------------------------------------------//
// TOP
//
// Name:     upMode_TopCountLength()
// Overview: Calculates the length of the top side for anti-aliasing blending.
// Arguments:
//    - info: Pointer to a structure containing count information.
// Return Value: None
//----------------------------------------------------------------------------//

template <typename PixelType>
void upMode_TopCountLength(BlendingInfo<PixelType> *info) {
  PF_LayerDef *output = info->output;

  long count_target = 0;
  int len = 0, width = GET_WIDTH(info->input), height = output->height;
  u_int *flg = &info->core[2].flg;

  //----------------------------------------------------------//
  //         Counting the length of the top side (top_len)    //
  //----------------------------------------------------------//
  // First time checks only the left side
  count_target = info->in_target - 1; // change target to be examined

  // Check if it's a flat fill and handle it
  if (ComparePixel(count_target, count_target - width) &&
      ComparePixelEqual(info->in_target - width, info->in_target - 1 - width)) {
    info->core[2].length = 0;
    info->core[2].start = (float)(info->j);
    info->core[2].end = info->core[2].start;
    return;
  } else {
    len++;

    // Check if y = 0 is reached (top of the image)
    if (info->j - len <= 1) {
      len = info->j - 1;

      info->core[2].start = (float)(info->j);
      info->core[2].end = (float)(info->j - len);
      info->core[2].length = len;
      return;
    }
  }

  // Continue checking until reaching y = 0 (top of the image)
  while (1) {
    count_target =
        info->in_target - (len * width); // change target to be examined

    // Is it a flat fill?
    if (ComparePixel(count_target, count_target - width)) {
      info->core[2].start = (float)(info->j);
      info->core[2].end = (float)(info->j - len);

      // It's a flat fill
      // DEBUG_PIXEL( info->in_target, DEBUG_COL_BLUE);

      (*flg) |= CR_FLG_FILL;

      break;
    }

    count_target =
        info->in_target - (len * width) - 1; // change target to be examined

    // Does it change color pixel by pixel?
    if (ComparePixel(count_target, count_target - width)) {
      info->core[2].start = (float)(info->j);
      info->core[2].end = (float)(info->j - len);

      if (width - 2 > info->i && info->i > 2 && height - 2 > info->j &&
          info->j > 2) {
        // Is end value correction necessary?
        // Check the length of the lower corner on the line, and if the
        // difference is 1, correct it Check if it's a corner (non-protrusion)
        if (!(info->flag & SECOND_COUNT) &&
            ComparePixel(count_target, count_target + 1)) {
          // Count here
          BlendingInfo<PixelType> sc_info;

          sc_info = *info; // copy and initlialize

          sc_info.i = info->i - 1;
          sc_info.j = info->j - len;
          sc_info.out_target = sc_info.in_target =
              sc_info.j * width + sc_info.i;
          sc_info.flag = SECOND_COUNT;

          upMode_TopCountLength<PixelType>(&sc_info);

          // Adjust for a half-pixel offset to the right if needed
          if (len - sc_info.core[2].length == 1 &&
              sc_info.core[2].length != 0) {
            info->core[2].end += 0.5f; // adjusting for a half-pixel offset
          }
        }
      }

      break;
    }

    len++;

    // Check if y = 0 (top of the image) is reached
    if (info->j - len <= 1) {
      len = info->j - 1;

      info->core[2].start = (float)(info->j);
      info->core[2].end = (float)(info->j - len);

      break;
    }
  }

  // Copy the calculated length
  info->core[2].length = len;
}

//----------------------------------------------------------------------------//
// BOTTOM
//
// Name:     upMode_BottomCountLength()
// Overview: Counts the length of the bottom side for anti-aliasing
// Arguments: info : Structure of count information
// Return Value: None
//----------------------------------------------------------------------------//

template <typename PixelType>
void upMode_BottomCountLength(BlendingInfo<PixelType> *info) {
  PF_LayerDef *output = info->output;

  long count_target = 0;
  int len = 1, width = GET_WIDTH(info->input), height = output->height;
  u_int *flg = &info->core[3].flg;

  //----------------------------------------------------------//
  // Counting the length of the bottom side (bottom_len)      //
  //----------------------------------------------------------//
  while (1) {
    count_target =
        info->in_target + (len - 1) * width; // Change target to be examined

    // Is it a flat fill?
    if (ComparePixel(count_target, count_target + width)) {
      info->core[3].start = (float)(info->j);
      info->core[3].end = (float)(info->j + len);

      // It's a flat fill
      // DEBUG_PIXEL( info->in_target, DEBUG_COL_BLUE);

      (*flg) |= CR_FLG_FILL;

      break;
    }

    count_target =
        info->in_target + (len - 1) * width + 1; // Change target to be examined

    if (ComparePixel(count_target, count_target + width)) {
      info->core[3].start = (float)(info->j);
      info->core[3].end = (float)(info->j + len);

      if (width - 2 > info->i && info->i > 2 && height - 2 > info->j &&
          info->j > 2) {
        // Is end value correction necessary?
        // Examine the length of the lower corner on the line, and if the
        // difference is 1, correct it Check if it's a corner
        if (!(info->flag & SECOND_COUNT) &&
            ComparePixel(count_target + width,
                         count_target + width + 1)) // is it a corner?
        {
          // Count here
          BlendingInfo<PixelType> sc_info;

          sc_info = *info; // copy and initialize

          sc_info.i = info->i + 1;
          sc_info.j = info->j + len;
          sc_info.out_target = sc_info.in_target =
              sc_info.j * width + sc_info.i;
          sc_info.flag = SECOND_COUNT;

          upMode_BottomCountLength<PixelType>(&sc_info);

          if (sc_info.core[3].length - len == 1) {
            info->core[3].end +=
                0.5f; // adjusting for a half-pixel offset to the right
          }
        }
      }

      break;
    }

    len++;

    // When x = 0, end
    if (info->j + len >= height - 1) {
      len = height - 1 - info->j;

      info->core[3].start = (float)(info->j);
      info->core[3].end = (float)(info->j + len);
      break;
    }
  }

  info->core[3].length = len;
}

//----------------------------------------------------------------------------//
// LEFT
//
// Name:     upMode_????Blending()
// Overview: Actually applies anti-aliasing
// Arguments: info : Structure of count information
// Return Value: None
//----------------------------------------------------------------------------//

template <typename PixelType>
void upMode_LeftBlending(BlendingInfo<PixelType> *info) {
  long t;
  int in_width = GET_WIDTH(info->input);
  float start = info->core[0].start;
  float end = info->core[0].end;

  // Normally, the value of Length is halved and used, but this time the unit is
  // half pixel, so it's doubled, len*(1/2)*2=len So you can use it as it is
#if 0
    if(flg & CR_FLG_FILL)
    {
        //----------------------//
        //      Solid Fill      //
        //----------------------//
        int t;

        blend_target = target;

        for(t=0;t<len;t++)
        {
            Blendingf(in_ptr, out_ptr, blend_target, blend_target-width, 0.8f);
            blend_target--;
        }
    
    }
#endif
  { // Normal
    long blend_target = 0, out_target = 0;
    int blend_count;
    float pre_ratio, // The area ratio of the previous triangle
        ratio,       // The current area ratio
        l,           // The length of the base
        len;         // The length of the entire base
    int end_p; // The starting pixel (the name is 'end' because of info->end)

    end_p = (int)end;

    len = start - end;

    // How many pixels to blend? Round up
    blend_count = CEIL((float)(info->i + 1) - end);

    // Initialize the previous ratio
    pre_ratio = 0.0f;

    // Initialize the target to blend
    blend_target = info->in_target - (blend_count - 1);
    out_target = info->out_target - (blend_count - 1);

    // base x height / 2 = base x ((base/whole base) x whole height) / 2 = | * |
    // * 0.5 * 0.5 / len // calculate from left to right
    for (t = 0; t < blend_count; t++) {
      l = (float)(end_p + 1 + t) - end;
      ratio = (l * l * 0.5f * 0.5f) / len;

      // Blend
      Blendingf(info->in_ptr, info->out_ptr, blend_target,
                blend_target - in_width, out_target,
                1.0f - (ratio - pre_ratio));

      pre_ratio = ratio;

      blend_target++;
      out_target++;
    }
  }
}

//----------------------------------------------------------------------------//
// RIGHT
//
// Overview   : Blends a line of pixels from the right to the left
// Function Name : upMode_RightBlending
// Arguments   :
//    info: Information for blending
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
void upMode_RightBlending(BlendingInfo<PixelType> *info) {
  long t;
  long length = info->core[1].length;
  float start = info->core[1].start;
  float end = info->core[1].end;
  int in_width = GET_WIDTH(info->input);

#if 0
    else if(flg & CR_FLG_FILL)
    {
        //----------------------//
        //      Solid Fill      //
        //----------------------//
    }
#endif
  { // Normal
    if (length > 0) {
      long blend_target = 0, out_target = 0;
      int blend_count;
      float pre_ratio, // The area ratio of the previous triangle
          ratio,       // The current area ratio
          l,           // The length of the base
          len;         // The length of the entire base
      int end_p; // The starting pixel (the name is 'end' because of info->end)

      end_p = (int)(end - 0.000001); // A desperate measure because it gets
                                     // weird at exactly 4.0f etc.

      len = end - start;

      // How many pixels to blend? Round up //
      blend_count = CEIL(end - (float)(info->i + 1));

      // Initialize the previous ratio //
      pre_ratio = 0.0f;

      // Initialize the target to blend //
      blend_target = info->in_target + blend_count;
      out_target = info->out_target + blend_count;

      // Calculate from the right to the left //
      for (t = 0; t < blend_count; t++) {
        l = end - (float)(end_p - t);
        ratio = (l * l * 0.5f * 0.5f) / len;

        // Blend //
        Blendingf(info->in_ptr, info->out_ptr, blend_target,
                  blend_target + in_width, out_target,
                  1.0f - (ratio - pre_ratio));

        pre_ratio = ratio;

        blend_target--;
        out_target--;
      }
    }
  }
}

//----------------------------------------------------------------------------//
// TOP
//
// Overview   : Blends a line of pixels from the top down
// Function Name : upMode_TopBlending
// Arguments   :
//    info: Information for blending
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
void upMode_TopBlending(BlendingInfo<PixelType> *info) {
  long t;
  long length = info->core[2].length;
  float start = info->core[2].start;
  float end = info->core[2].end;
  int in_width = GET_WIDTH(info->input);
  int out_width = GET_WIDTH(info->output);

  // Normally, the value of Length is halved and used, but this time the unit is
  // half pixel, so it's doubled, len*(1/2)*2=len So you can use it as it is

#if 0
    else if(flg->top & CR_FLG_FILL)
    {
        //----------------------//
        //      Solid Fill      //
        //----------------------//
    }
#endif
  { // Normal
    if (length > 0) {
      long blend_target = 0, out_target = 0;
      int blend_count;
      float pre_ratio, // The area ratio of the previous triangle
          ratio,       // The current area ratio
          l,           // The length of the base
          len;         // The length of the entire base
      int end_p; // The starting pixel (the name is 'end' because of info->end)

      end_p = (int)(end); // A desperate measure because it gets weird at
                          // exactly 4.0f etc.

      len = start - end;

      // How many pixels to blend? Round up //
      blend_count = CEIL((float)(info->j) - end);

      // Initialize the previous ratio //
      pre_ratio = 0.0f;

      // Initialize the target to blend //
      blend_target = info->in_target - blend_count * in_width;
      out_target = info->out_target - blend_count * out_width;

      // Calculate from the top down //
      for (t = 0; t < blend_count; t++) {
        l = (float)(end_p + 1 + t) - end;
        ratio = (l * l * 0.5f * 0.5f) / len;

        // Blend //
        Blendingf(info->in_ptr, info->out_ptr, blend_target, blend_target - 1,
                  out_target, 1.0f - (ratio - pre_ratio));

        pre_ratio = ratio;

        blend_target += in_width;
        out_target += out_width;
      }
    }
  }
}

//----------------------------------------------------------------------------//
// BOTTOM
//
// Overview   : Blends a line of pixels from the bottom up
// Function Name : upMode_BottomBlending
// Arguments   :
//    info: Information for blending
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
void upMode_BottomBlending(BlendingInfo<PixelType> *info) {
  long t;
  float start = info->core[3].start;
  float end = info->core[3].end;
  int in_width = GET_WIDTH(info->input);
  int out_width = GET_WIDTH(info->output);

#if 0
    else if(flg & CR_FLG_FILL)
    {
        //----------------//
        //   Solid Fill   //
        //----------------//
    }
#endif
  { // Normal
    long blend_target = 0, out_target = 0;
    int blend_count;
    float pre_ratio, // The area ratio of the previous triangle
        ratio,       // The current area ratio
        l,           // The length of the base
        len;         // The length of the entire base
    int end_p; // The starting pixel (the name is 'end' because of info->end)

    end_p = (int)(end - 0.00001);

    len = end - start;

    // How many pixels to blend? Round up
    blend_count = CEIL(end - (float)(info->j));

    // Initialize the previous ratio
    pre_ratio = 0.0f;

    // Initialize the target to blend
    blend_target =
        info->in_target +
        (blend_count - 1) *
            in_width; // info->target is the final processing pixel, so -1
    out_target =
        info->out_target +
        (blend_count - 1) *
            out_width; // info->target is the final processing pixel, so -1

    // Calculate from the bottom up
    for (t = 0; t < blend_count; t++) {
      l = end - (float)(end_p - t);
      ratio = (l * l * 0.5f * 0.5f) / len;

      // Blend
      Blendingf(info->in_ptr, info->out_ptr, blend_target, blend_target + 1,
                out_target, 1.0f - (ratio - pre_ratio));

      pre_ratio = ratio;

      blend_target -= in_width;
      out_target -= out_width;
    }
  }
}

//----------------------------------------------------------------------------//

// Explicit instantiation declarations
template void upMode_LeftCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void
upMode_LeftCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void upMode_RightCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void
upMode_RightCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void upMode_TopCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void upMode_TopCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void
upMode_BottomCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void
upMode_BottomCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void upMode_LeftBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void upMode_LeftBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void upMode_RightBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void upMode_RightBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void upMode_TopBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void upMode_TopBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void upMode_BottomBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void upMode_BottomBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);
