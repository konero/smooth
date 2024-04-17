//----------------------------------------------------------------------------//
// Processing of corners that appear to be missing more than 2 pixels
//----------------------------------------------------------------------------//

#include "define.h"
#include "util.h"

#include "Lack.h"

//----------------------------------------------------------------------------//
// Mode 1
//   |
// __| 1
//----------------------------------------------------------------------------//
template <typename PixelType>
void LackMode01Execute(BlendingInfo<PixelType> *info) {
  long h, v, width;
  long range;

  PixelType *in_ptr = info->in_ptr;
  PF_LayerDef *input = info->input;

  width = GET_WIDTH(input);
  h = info->core[0].length; // left
  v = info->core[2].length; // top
  range = info->range;

  // Check to see if mode 01 is applicable
  if (ComparePixel(info->in_target, info->in_target - width - 1)) {
    return;
  }

  if ((3 >= h && h >= 2) && (3 >= v && v >= 2)) {
    // Blend
    PixelType ref0, ref1, ref2, ref_temp, src;

    src = in_ptr[info->in_target];
    ref0 = in_ptr[info->in_target + 1];
    ref1 = in_ptr[info->in_target + width];
    ref2 = in_ptr[info->in_target + width + 1];

    ref_temp.red = (ref0.red + ref1.red + ref2.red) / 3;
    ref_temp.green = (ref0.green + ref1.green + ref2.green) / 3;
    ref_temp.blue = (ref0.blue + ref1.blue + ref2.blue) / 3;
    ref_temp.alpha = (ref0.alpha + ref1.alpha + ref2.alpha) / 3;

    BlendingPixelf(&src, &ref_temp, &info->out_ptr[info->out_target], 0.5f);

    //      if( info->LineWeight == 1.0f )
    //      {
    //          DEBUG_PIXEL( info->output, info->out_target, 2 );
    //      }
  }
}

//----------------------------------------------------------------------------//
// Mode 2
// __
//   |
//   | 2
//----------------------------------------------------------------------------//
template <typename PixelType>
void LackMode02Execute(BlendingInfo<PixelType> *info) {
  // Check to see if mode 02 is applicable
  long h, v, width;
  long range;

  PixelType *in_ptr = info->in_ptr;
  PF_LayerDef *input = info->input;

  width = GET_WIDTH(input);
  h = info->core[0].length; // left
  v = info->core[3].length; // bottom
  range = info->range;

  // Check to see if mode 01 is applicable
  if (ComparePixel(info->in_target, info->in_target + width - 1)) {
    return;
  }

  // Blend
  if ((3 >= h && h >= 2) && (3 >= v && v >= 2)) {
    PixelType ref0, ref1, ref2, ref_temp, src;

    src = in_ptr[info->in_target];
    ref0 = in_ptr[info->in_target + 1];
    ref1 = in_ptr[info->in_target - width];
    ref2 = in_ptr[info->in_target - width + 1];

    ref_temp.red = (ref0.red + ref1.red + ref2.red) / 3;
    ref_temp.green = (ref0.green + ref1.green + ref2.green) / 3;
    ref_temp.blue = (ref0.blue + ref1.blue + ref2.blue) / 3;
    ref_temp.alpha = (ref0.alpha + ref1.alpha + ref2.alpha) / 3;

    BlendingPixelf(&src, &ref_temp, &info->out_ptr[info->out_target], 0.5f);

    //      if( info->LineWeight == 1.0f )
    //      {
    //          DEBUG_PIXEL( info->output, info->out_target, 3 );
    //      }
  }
}

//----------------------------------------------------------------------------//
// Mode 3,4
//
// |
// |__ 3
// ___
// |
// |   4
//
//----------------------------------------------------------------------------//
template <typename PixelType>
void LackMode0304Execute(BlendingInfo<PixelType> *info) {
  long i, j, target, h, v, width;
  long range;

  PixelType *in_ptr = info->in_ptr;
  PF_LayerDef *input = info->input;

  h = 1;
  v = 1;
  target = info->in_target;
  input = info->input;
  width = GET_WIDTH(input);
  range = info->range;

  // Check if mode 03 is applicable
  if (ComparePixelEqual(target, target - width + 1) &&
      ComparePixel(target, target + width)) {
    //-------------------------------
    // Mode 03
    //-------------------------------

    //-------------------------------
    // Length count
    //-------------------------------
    // →
    for (i = info->i; i < input->width - 1; i++, target++) {
      if (ComparePixel(target, target + 1) ||
          ComparePixel(target + width, target + width + 1)) {
        break;
      }

      h++;
    }

    // ↑
    target = info->in_target;
    for (j = info->j; j > 1; j--, target -= width) {
      if (ComparePixel(target, target - width) ||
          ComparePixel(target - 1, target - 1 - width)) {
        break;
      }

      v++;
    }

    if ((3 >= h && h >= 2) && (3 >= v && v >= 2)) {
      PixelType ref0, ref1, ref2, ref_temp, src;

      src = in_ptr[info->in_target];
      ref0 = in_ptr[info->in_target - 1];
      ref1 = in_ptr[info->in_target + width];
      ref2 = in_ptr[info->in_target + width - 1];

      ref_temp.red = (ref0.red + ref1.red + ref2.red) / 3;
      ref_temp.green = (ref0.green + ref1.green + ref2.green) / 3;
      ref_temp.blue = (ref0.blue + ref1.blue + ref2.blue) / 3;
      ref_temp.alpha = (ref0.alpha + ref1.alpha + ref2.alpha) / 3;

      BlendingPixelf(&src, &ref_temp, &info->out_ptr[info->out_target], 0.5f);

      //          if( info->LineWeight == 1.0f )
      //          {
      //              DEBUG_PIXEL( info->output, info->out_target, 0 );
      //          }
    }
  }
  // Check if mode 04 is applicable
  else if (ComparePixelEqual(target, target + width + 1) &&
           ComparePixel(target, target - width)) {
    //-------------------------------
    // Mode 04
    //-------------------------------

    //-------------------------------
    // Length count
    //-------------------------------
    // →
    for (i = info->i; i < input->width - 1; i++, target++) {
      if (ComparePixel(target, target + 1) ||
          ComparePixel(target - width, target - width + 1)) {
        break;
      }

      h++;
    }

    // ↓
    target = info->in_target;
    for (j = info->j; j < input->height - 1; j++, target += width) {
      if (ComparePixel(target, target + width) ||
          ComparePixel(target - 1, target - 1 + width)) {
        break;
      }

      v++;
    }

    if ((3 >= h && h >= 2) && (3 >= v && v >= 2)) {
      PixelType ref0, ref1, ref2, ref_temp, src;

      src = in_ptr[info->in_target];
      ref0 = in_ptr[info->in_target - 1];
      ref1 = in_ptr[info->in_target - width];
      ref2 = in_ptr[info->in_target - width - 1];

      ref_temp.red = (ref0.red + ref1.red + ref2.red) / 3;
      ref_temp.green = (ref0.green + ref1.green + ref2.green) / 3;
      ref_temp.blue = (ref0.blue + ref1.blue + ref2.blue) / 3;
      ref_temp.alpha = (ref0.alpha + ref1.alpha + ref2.alpha) / 3;

      BlendingPixelf(&src, &ref_temp, &info->out_ptr[info->out_target], 0.5f);

      //          if( info->LineWeight == 1.0f )
      //          {
      //              DEBUG_PIXEL( info->output, info->out_target, 1 );
      //          }
    }
  }
}

// Explicit instantiation declarations
template void LackMode01Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void LackMode01Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void LackMode02Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void LackMode02Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void LackMode0304Execute<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void LackMode0304Execute<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);
