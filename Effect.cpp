#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"

#include "Param_Utils.h"
#include "util.h"
#include "version.h"

#include "define.h"

#include "8link.h"
#include "Lack.h"
#include "downMode.h"
#include "upMode.h"

#include "Effect.h"

//----------------------------------------------------------------------------//
// Parameter Definitions
enum {
  PARAM_INPUT = 0,
  PARAM_WHITE_OPTION,
  PARAM_RANGE,
  PARAM_LINE_WEIGHT,
  PARAM_NUM,
};

//----------------------------------------------------------------------------//
// Prototypes
static PF_Err About(PF_InData *in_data, PF_OutData *out_data,
                    PF_ParamDef *params[], PF_LayerDef *output);

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output);

static PF_Err GlobalSetdown(PF_InData *in_data, PF_OutData *out_data,
                            PF_ParamDef *params[], PF_LayerDef *output);

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output);

static PF_Err Render(PF_InData *in_data, PF_OutData *out_data,
                     PF_ParamDef *params[], PF_LayerDef *output);

static PF_Err PopDialog(PF_InData *in_data, PF_OutData *out_data,
                        PF_ParamDef *params[], PF_LayerDef *output);

//----------------------------------------------------------------------------//
// Util Funcions
//----------------------------------------------------------------------------//
/**
 * @brief Retrieves a white pixel color in PF_Pixel16 format.
 * 
 * @param white Pointer to PF_Pixel16 to store the white pixel color.
 */
static inline void getWhitePixel(PF_Pixel16 *white) {
  PF_Pixel16 color = {0x8000, 0x8000, 0x8000, 0x8000};
  *white = color;
}

/**
 * @brief Retrieves a white pixel color in PF_Pixel8 format.
 * 
 * @param white Pointer to PF_Pixel8 to store the white pixel color.
 */
static inline void getWhitePixel(PF_Pixel8 *white) {
  PF_Pixel8 color = {0xFF, 0xFF, 0xFF, 0xFF};
  *white = color;
}

/**
 * @brief Retrieves a null (black) pixel color in PF_Pixel16 format.
 * 
 * @param null_pixel Pointer to PF_Pixel16 to store the null pixel color.
 */
static inline void getNullPixel(PF_Pixel16 *null_pixel) {
  PF_Pixel16 color = {0x0, 0x0, 0x0, 0x0};
  *null_pixel = color;
}

/**
 * @brief Retrieves a null (black) pixel color in PF_Pixel8 format.
 * 
 * @param null_pixel Pointer to PF_Pixel8 to store the null pixel color.
 */
static inline void getNullPixel(PF_Pixel8 *null_pixel) {
  PF_Pixel8 color = {0x0, 0x0, 0x0, 0x0};
  *null_pixel = color;
}

#if 0
template<typename PackedPixelType > 
static inline void ColorKey(PackedPixelType *in_ptr, int row_bytes, int height)
{
    int         limit, t=0;
    PackedPixelType	key;
	getWhitePixel( &key );	// 0xff or 0xffff

    limit = (row_bytes / sizeof(PackedPixelType)) * height;

    for( t=0; t<limit; t++)
    {
        if( key == in_ptr[t] )
        {
			in_ptr[t] = 0;
        }
    }
}
#endif

template <typename PixelType>
static inline void preProcess(PixelType *in_ptr, int row_bytes, int height,
                              PF_Rect *rect, bool is_white_trans) {
  PixelType key;
  PixelType null_pixel;
  getWhitePixel(&key); // 0xff or 0x8000
  getNullPixel(&null_pixel);

  int width = (row_bytes / sizeof(PixelType));

  int top = 0, left = width, right = 0, bottom = 0;
  bool top_found = false, left_found = false;

  if (is_white_trans) {
    // Ignore the alpha channel and remove pixels that are white
    int t = 0;
    for (int j = 0; j < height; j++) {
      if (!top_found) {
        top = j; // Set the top boundary if not already found
      }

      for (int i = 0; i < width; i++) {
        if (key.red == in_ptr[t].red && key.green == in_ptr[t].green &&
            key.blue == in_ptr[t].blue) {
          // Remove the key color
          in_ptr[t] = null_pixel; // Replace with a null pixel
        } else if (in_ptr[t].alpha == 0) {
          // Pixel already removed
          // The pixel has already been removed (alpha channel is 0)
        } else {
          // Update boundary flags if necessary
          top_found = true;  // Flag indicating top boundary found
          left_found = true; // Flag indicating left boundary found

          // Update boundary coordinates
          if (left > i) {
            left = i; // Update left boundary
          }

          if (right < i) {
            right = i; // Update right boundary
          }

          if (bottom < j) {
            bottom = j; // Update bottom boundary
          }
        }
        t++;
      }
    }
  } else {
    // Obtain region information without removing white pixels
    int t = 0;
    for (int j = 0; j < height; j++) {
      if (!top_found) {
        top = j; // Set the top boundary if not already found
      }

      for (int i = 0; i < width; i++) {
        if (!(key.red == in_ptr[t].red && key.green == in_ptr[t].green &&
              key.blue == in_ptr[t].blue) &&
            in_ptr[t].alpha != 0) {
          // Update boundary flags and coordinates if necessary
          top_found = true;  // Flag indicating top boundary found
          left_found = true; // Flag indicating left boundary found
                             // Update boundary coordinates
          if (left > i) {
            left = i; // Update left boundary
          }

          if (right < i) {
            right = i; // Update right boundary
          }

          if (bottom < j) {
            bottom = j; // Update bottom boundary
          }
        }
        t++;
      }
    }
  }

  if (top_found)
    rect->top =
        top; // Set the top coordinate of the rectangle to the found value
  else
    rect->top = 0; // Set the top coordinate of the rectangle to 0 if not found

  if (left_found)
    rect->left =
        left; // Set the left coordinate of the rectangle to the found value
  else
    rect->left =
        0; // Set the left coordinate of the rectangle to 0 if not found

  rect->right =
      right + 1; // Set the right coordinate of the rectangle (incremented by 1)
  rect->bottom =
      bottom +
      1; // Set the bottom coordinate of the rectangle (incremented by 1)
}

//----------------------------------------------------------------------------//
// Overview: Main function for the effect
// Function Name: EffectPluginMain
// Arguments: cmd - Command
//            in_data - Input data
//            out_data - Output data
//            params[] - Parameter array
//            output - Output layer definition
// Return Value: Error code
//----------------------------------------------------------------------------//
DllExport PF_Err EntryPointFunc(PF_Cmd cmd, PF_InData *in_data,
                                PF_OutData *out_data, PF_ParamDef *params[],
                                PF_LayerDef *output) {
  PF_Err err = PF_Err_NONE;

  try {
    switch (cmd) {
    case PF_Cmd_ABOUT: // Pressed the About button
      err = About(in_data, out_data, params, output);
      break;

    case PF_Cmd_GLOBAL_SETUP: // Called once when Global setup is loaded
      err = GlobalSetup(in_data, out_data, params, output);
      break;

    case PF_Cmd_GLOBAL_SETDOWN: // Called once when Global setdown is finished
      err = GlobalSetdown(in_data, out_data, params, output);
      break;

    case PF_Cmd_PARAMS_SETUP: // Set up parameters
      err = ParamsSetup(in_data, out_data, params, output);
      break;

    case PF_Cmd_RENDER: // Rendering
      err = Render(in_data, out_data, params, output);
      break;

    case PF_Cmd_DO_DIALOG:
      err = PopDialog(in_data, out_data, params, output);
      break;
    }
  } catch (APIErr api_err) { // API returned an error

    PrintAPIErr(&api_err); // Print

    err = PF_Err_INTERNAL_STRUCT_DAMAGED;
  } catch (...) {
    err = PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  return err;
}

//----------------------------------------------------------------------------//
// Summary:   Function called when the About button is pressed
// Name:      About
// Arguments:
// Returns:
//----------------------------------------------------------------------------//
static PF_Err About(PF_InData *in_data, PF_OutData *out_data,
                    PF_ParamDef *params[], PF_LayerDef *output) {
#if SK_STAGE_DEVELOP
  const char *stage_str = "Debug";
#elif SK_STAGE_RELEASE
  const char *stage_str = "";
#endif

  char str[256];
  memset(str, 0, 256);

  sprintf(out_data->return_msg, "%s, v%d.%d.%d %s\n%s\n", NAME, MAJOR_VERSION,
          MINOR_VERSION, BUILD_VERSION, stage_str, str);

  return PF_Err_NONE;
}

//----------------------------------------------------------------------------//
// Summary:     Function called when the plugin is loaded
// Name:        GlobalSetup
// Arguments:
// Returns:
//----------------------------------------------------------------------------//
static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output) {
  // The version must match with PiPL and PiPl only supports direct values,
  // but it's not user-friendly, so it's fixed to 0
  out_data->my_version = PF_VERSION(2, 0, 0, 0, 0);

  // Process input buffer
  out_data->out_flags |=
      PF_OutFlag_I_WRITE_INPUT_BUFFER | PF_OutFlag_DEEP_COLOR_AWARE;
  out_data->out_flags2 |= PF_OutFlag2_I_AM_THREADSAFE;

  return PF_Err_NONE;
}

static PF_Err GlobalSetdown(PF_InData *in_data, PF_OutData *out_data,
                            PF_ParamDef *params[], PF_LayerDef *output) {
  return PF_Err_NONE;
}

//----------------------------------------------------------------------------//
// Summary:     Parameter Setting
// Name:        ParamsSetup
// Arguments:
// Returns:
//----------------------------------------------------------------------------//
static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output) {

  PF_ParamDef def;
  AEFX_CLR_STRUCT(def); // Initialize def

  def.param_type = PF_Param_CHECKBOX;
  def.flags = PF_ParamFlag_START_COLLAPSED;
  PF_STRCPY(def.name, "white option");
  def.u.bd.value = def.u.bd.dephault = FALSE;
  def.u.bd.u.nameptr =
      "transparent"; /* this is strictly a pointer; don't STRCPY into it! */

  PF_ADD_PARAM(in_data, -1, &def);

  AEFX_CLR_STRUCT(def);

  PF_ADD_FLOAT_SLIDER("range",
                      0.0f,         // VALID_MIN,
                      100.0f,       // VALID_MAX,
                      0.0f,         // SLIDER_MIN,
                      10.0f,        // SLIDER_MAX,
                      1.00f,        // CURVE_TOLERANCE, (not sure)
                      1.0f,         // DFLT, (default)
                      1,            // DISP, (display as is)
                      0,            // PREC, (percentage display?)
                      FALSE,        // WANT_PHASE,
                      PARAM_RANGE); // ID

  PF_ADD_FLOAT_SLIDER("line weight",
                      0.0f,               // VALID_MIN,
                      1.0f,               // VALID_MAX,
                      0.0f,               // SLIDER_MIN,
                      1.0f,               // SLIDER_MAX,
                      1.0f,               // CURVE_TOLERANCE, (not sure)
                      0.0f,               // DFLT, (default)
                      1,                  // DISP, (display as is)
                      0,                  // PREC, (percentage display?)
                      FALSE,              // WANT_PHASE,
                      PARAM_LINE_WEIGHT); // ID

  // Set the number of parameters
  out_data->num_params = PARAM_NUM;

  return PF_Err_NONE;
}

//----------------------------------------------------------------------------//
// Smoothing Execution Function
// PixelType:      PF_Pixel8, PF_Pixel16
// PackedPixelType:    KP_PIXEL32, KP_PIXEL64
//----------------------------------------------------------------------------//
template <typename PixelType, typename PackedPixelType>
static PF_Err smoothing(PF_InData *in_data, PF_OutData *out_data,
                        PF_ParamDef *params[], PF_LayerDef *input,
                        PF_LayerDef *output, PixelType *in_ptr,
                        PixelType *out_ptr) {
  PF_Err err;

  PF_Rect extent_hint;
  BEGIN_PROFILE(); // Begin performance profiling

  // Pre-process: White exclusion & region information acquisition
  preProcess<PixelType>(in_ptr, input->rowbytes, input->height, &extent_hint,
                        params[PARAM_WHITE_OPTION]->u.bd.value ? true : false);

  err = PF_COPY(input, output, NULL, NULL); // Copy input to output

  int in_width, in_height, out_width, out_height, i, j;
  long in_target, out_target;
  unsigned int range = (unsigned int)(params[PARAM_RANGE]->u.fs_d.value *
                                      (getMaxValue<PixelType>() * 4)) /
                       100; // Calculate the range for MLAA
  float line_weight =
            (float)(params[PARAM_LINE_WEIGHT]->u.fs_d.value / 2.0 + 0.5),
        weight; // Calculate the line weight
  bool lack_flg;

  in_width = GET_WIDTH(input);
  in_height = GET_HEIGHT(input);
  out_width = GET_WIDTH(output);
  out_height = GET_HEIGHT(output);

  BlendingInfo<PixelType> blend_info, *info;

  info = &blend_info;

  // Initialize common parts
  blend_info.input = input;
  blend_info.output = output;
  blend_info.in_ptr = in_ptr;
  blend_info.out_ptr = out_ptr;
  blend_info.range = range;
  blend_info.LineWeight = line_weight;

  // Process region information
  if (extent_hint.top == 0)
    extent_hint.top = 1;
  if (extent_hint.left == 0)
    extent_hint.left = 1;
  if (extent_hint.right == in_width)
    extent_hint.right -= 1;
  if (extent_hint.bottom == in_height)
    extent_hint.bottom -= 1;

  // Antialiasing Process
  //------------------------------------------------------------------------//
  for (j = extent_hint.top; j < extent_hint.bottom; j++) {
    lack_flg = false; // Reset the lack flag for each row

    in_target = j * in_width + extent_hint.left;
    out_target = j * out_width + extent_hint.left;

    for (i = extent_hint.left; i < extent_hint.right;
         i++, in_target++, out_target++) {
      // Check if there might be a lack
      if (lack_flg) {
        // Reset the lack flag
        lack_flg = false;

        // Set the blending info
        blend_info.i = i;
        blend_info.j = j;
        blend_info.in_target = in_target;
        blend_info.out_target = out_target;
        blend_info.flag = 0;

        // Execute the lack mode processing
        LackMode0304Execute(&blend_info);
      }

      // Limit processing to potential corners only
      if (FAST_COMPARE_PIXEL(in_target, in_target + 1)) {
        unsigned char mode_flg = 0;

        // Initialize //

        blend_info.i = i;
        blend_info.j = j;
        blend_info.in_target = in_target;
        blend_info.out_target = out_target;
        blend_info.flag = 0;

        memset(&blend_info.core, 0, sizeof(Cinfo) * 4); // Fill with zeros

        // Antialiasing processing here
        if (ComparePixel(in_target, in_target + 1))
          (mode_flg |= 1 << 0);
        if (ComparePixel(in_target, in_target - in_width))
          (mode_flg |= 1 << 1);
        if (ComparePixel(in_target, in_target + in_width))
          (mode_flg |= 1 << 2);
        if (ComparePixel(in_target, in_target - 1))
          (mode_flg |= 1 << 3);

        if (mode_flg != 0) {
          // Potential lack in the next pixel
          if (i < input->width - 2 && (mode_flg & 1 << 0)) {
            lack_flg = true;
          }

          switch (mode_flg) {

          case 3: // Upward corner -------------------------------------//

            // Avoid conflicts with 8-connected mode
            if (ComparePixelEqual(in_target - in_width,
                                  in_target + 1) && // Diagonal is the same and
                ComparePixel(in_target - in_width + 1,
                             in_target - in_width) && // Corners of the diagonal
                                                      // are different colors
                ComparePixel(in_target - in_width + 1, in_target + 1)) {
              // Skip processing
              break;
            }

            // Count //
            upMode_LeftCountLength<PixelType>(&blend_info);

            upMode_RightCountLength<PixelType>(&blend_info);

            upMode_TopCountLength<PixelType>(&blend_info);

            upMode_BottomCountLength<PixelType>(&blend_info);

            // Correction Processing
            //----------------------------------------------------//

            // Horizontal Direction
            //----------------------------------------------------//
            // Start Coordinate (only for longer left)
            if (blend_info.core[0].length - blend_info.core[1].length == 1) {
              blend_info.core[0].start -= 0.5f;
              blend_info.core[1].start -= 0.5f;
            }

            if ((blend_info.core[0].flg & CR_FLG_FILL) ||
                (blend_info.core[1].flg & CR_FLG_FILL)) {
              weight = 0.5f; // Weight adjustment for filled areas
            } else {
              weight = blend_info.LineWeight; // Default weight
            }

            // END
            // Recalculate 'end' to avoid confusion during correction
            // (The 'end' value output by Count is the same as Len and not
            // multiplied by 0.5)
            blend_info.core[0].end =
                blend_info.core[0].start -
                (blend_info.core[0].start - blend_info.core[0].end) * weight;
            blend_info.core[1].end =
                blend_info.core[1].start +
                (blend_info.core[1].end - blend_info.core[1].start) * weight;

            // Vertical Direction
            //----------------------------------------------------//
            // Start Coordinate (only for longer bottom)
            if (blend_info.core[3].length - blend_info.core[2].length == 1) {
              blend_info.core[2].start += 0.5f;
              blend_info.core[3].start += 0.5f;
            }

            if ((blend_info.core[2].flg & CR_FLG_FILL) ||
                (blend_info.core[3].flg & CR_FLG_FILL)) {
              weight = 0.5f; // Weight adjustment for filled areas
            } else {
              weight = blend_info.LineWeight; // Default weight
            }

            // END
            // Recalculate 'end' to avoid confusion during correction
            blend_info.core[2].end =
                blend_info.core[2].start -
                (blend_info.core[2].start - blend_info.core[2].end) * weight;
            blend_info.core[3].end =
                blend_info.core[3].start +
                (blend_info.core[3].end - blend_info.core[3].start) * weight;

#if 0 // No correction processing
            {
              blend_info.core[0].start =
                  (float)(i +
                          1); // Image coordinate starts from top left (0,0),
                              // but logical line starts from right, so +1
              blend_info.core[0].end = blend_info.core[0].start -
                                       (float)blend_info.core[0].length * 0.5f;

              blend_info.core[1].start =
                  (float)(i +
                          1); // Image coordinate starts from top left (0,0),
                              // but logical line starts from right, so +1
              blend_info.core[1].end = blend_info.core[1].start +
                                       (float)blend_info.core[1].length * 0.5f;

              blend_info.core[2].start = (float)(j);
              blend_info.core[2].end = blend_info.core[2].start -
                                       (float)blend_info.core[2].length * 0.5f;

              blend_info.core[3].start = (float)(j);
              blend_info.core[3].end = blend_info.core[3].start +
                                       (float)blend_info.core[3].length * 0.5f;
            }
#endif

            if (blend_info.core[0].length >=
                    2 && // Check if left and bottom lengths are greater than or
                         // equal to 2
                blend_info.core[3].length >= 2) {
              // Lack Mode 2
              LackMode02Execute(&blend_info); // Execute Lack Mode 2 processing
            } else if (blend_info.core[1].length >
                       0) // Check if left and right lengths are greater than 0
            {             // Horizontal Blend

              blend_info.mode = BLEND_MODE_UP_H; // Set blend mode to horizontal

              // Perform horizontal blending
              upMode_LeftBlending<PixelType>(&blend_info);

              upMode_RightBlending<PixelType>(&blend_info);

              if (blend_info.core[2].length >
                  1) { // Check if there is also a vertical direction
                // Perform vertical blending
                upMode_TopBlending<PixelType>(&blend_info);

                upMode_BottomBlending<PixelType>(&blend_info);
              }
            } else if (blend_info.core[2].length >
                       0) // Check if bottom and top lengths are greater than 0
            {             // Vertical Blend

              blend_info.mode = BLEND_MODE_UP_V; // Set blend mode to vertical

              upMode_TopBlending<PixelType>(&blend_info);

              upMode_BottomBlending<PixelType>(&blend_info);
            }

            break; // Exit the loop

          case 5: // Downward angle -------------------------------------//
            // Avoid collision with 8-connectivity mode
            if (ComparePixelEqual(in_target + in_width,
                                  in_target + 1) && // Diagonals are equal
                ComparePixel(
                    in_target + in_width + 1,
                    in_target +
                        in_width) && // Diagonal corners have different colors
                ComparePixel(in_target + in_width + 1, in_target + 1)) {
              // Do not process
              break;
            }

            // Count -----//
            downMode_LeftCountLength<PixelType>(&blend_info);

            downMode_RightCountLength<PixelType>(&blend_info);

            downMode_TopCountLength<PixelType>(&blend_info);

            downMode_BottomCountLength<PixelType>(&blend_info);

            // Correction Process -----//

            // Horizontal direction
            //----------------------------------------------------//
            // Start coordinate (only if left is longer)
            if (blend_info.core[0].length - blend_info.core[1].length == 1) {
              blend_info.core[0].start -= 0.5f;
              blend_info.core[1].start -= 0.5f;
            }

            if ((blend_info.core[0].flg & CR_FLG_FILL) ||
                (blend_info.core[1].flg & CR_FLG_FILL)) {
              weight = 0.5f;
            } else {
              weight = blend_info.LineWeight;
            }

            // END
            // Recalculate the 'end' values (the end values output by Count are
            // the same as Len and haven't been multiplied by 0.5) to avoid
            // confusion during correction recalculations
            blend_info.core[0].end =
                blend_info.core[0].start -
                (blend_info.core[0].start - blend_info.core[0].end) * weight;
            blend_info.core[1].end =
                blend_info.core[1].start +
                (blend_info.core[1].end - blend_info.core[1].start) * weight;

            // Vertical direction
            //----------------------------------------------------//
            // Start coordinate (only if bottom is longer)
            if (blend_info.core[3].length - blend_info.core[2].length == 1) {
              blend_info.core[2].start += 0.5f;
              blend_info.core[3].start += 0.5f;
            }

            if ((blend_info.core[2].flg & CR_FLG_FILL) ||
                (blend_info.core[3].flg & CR_FLG_FILL)) {
              weight = 0.5f;
            } else {
              weight = blend_info.LineWeight;
            }

            // END
            // Recalculate the 'end' values (the end values output by Count are
            // the same as Len and haven't been multiplied by 0.5) to avoid
            // confusion during correction recalculations
            blend_info.core[2].end =
                blend_info.core[2].start -
                (blend_info.core[2].start - blend_info.core[2].end) * weight;
            blend_info.core[3].end =
                blend_info.core[3].start +
                (blend_info.core[3].end - blend_info.core[3].start) * weight;

#if 0 // No correction processing
            {
              blend_info.core[0].start =
                  (float)(i + 1); // Image coordinates start from the top-left
                                  // corner as (0,0), but the logical line
                                  // starts from the right, hence the +1
              blend_info.core[0].end = blend_info.core[0].start -
                                       (float)blend_info.core[0].length * 0.5f;

              blend_info.core[1].start =
                  (float)(i + 1); // Image coordinates start from the top-left
                                  // corner as (0,0), but the logical line
                                  // starts from the right, hence the +1
              blend_info.core[1].end = blend_info.core[1].start +
                                       (float)blend_info.core[1].length * 0.5f;

              blend_info.core[2].start = (float)(j);
              blend_info.core[2].end = blend_info.core[2].start -
                                       (float)blend_info.core[2].length * 0.5f;

              blend_info.core[3].start = (float)(j);
              blend_info.core[3].end = blend_info.core[3].start +
                                       (float)blend_info.core[3].length * 0.5f;
            }
#endif

            // Blend
            if (blend_info.core[0].length >= 2 && // left >= 2 && top >= 2
                blend_info.core[2].length >= 2) {
              // Lack Mode 1
              LackMode01Execute(&blend_info);
            } else if (blend_info.core[1].length > 0) // Left > 0 && Right > 0
            {                                         // Horizontal blend

              blend_info.mode = BLEND_MODE_UP_H; // Set mode

              // Blend //
              downMode_LeftBlending<PixelType>(&blend_info);

              downMode_RightBlending<PixelType>(&blend_info);

              if (blend_info.core[3].length >
                  1) { // Also exists in vertical direction
                downMode_TopBlending<PixelType>(&blend_info);

                downMode_BottomBlending<PixelType>(&blend_info);
              }
            } else if (blend_info.core[3].length > 0) // bottom > 0 && top > 0
            {                                         // Vertical blend

              blend_info.mode = BLEND_MODE_UP_V; // Set mode

              downMode_TopBlending<PixelType>(&blend_info);

              downMode_BottomBlending<PixelType>(&blend_info);
            }

            break;
            // Protrusion 1
          case 7:
            Link8Mode01Execute(&blend_info);
            break;

          case 11: // Protrusion 2
            Link8Mode02Execute(&blend_info);
            break;

          case 13: // Protrusion 4
            Link8Mode04Execute(&blend_info);
            break;

          case 15: // Four corner pixels
            Link8SquareExecute(&blend_info);
            break;

          default:
            break;
          }

          // Protrusion mode 3
          if (i < input->width - 2) {
            // Initialization //
            blend_info.i = i + 1;
            blend_info.j = j;
            blend_info.in_target = in_target + 1;
            blend_info.out_target = out_target + 1;
            blend_info.flag = 0;

            mode_flg = 0;
            if (ComparePixel(blend_info.in_target,
                             blend_info.in_target - in_width))
              (mode_flg |= 1 << 0);
            if (ComparePixel(blend_info.in_target,
                             blend_info.in_target + in_width))
              (mode_flg |= 1 << 1);
            if (ComparePixel(blend_info.in_target, blend_info.in_target + 1))
              (mode_flg |= 1 << 2);

            if (3 == mode_flg) {
              // Protrusion 3
              Link8Mode03Execute(&blend_info);
            }
          }
        }
      }
    }
  }

  DEBUG_PIXEL(out_ptr, output, extent_hint.left, extent_hint.top);
  DEBUG_PIXEL(out_ptr, output, extent_hint.left, extent_hint.bottom);
  DEBUG_PIXEL(out_ptr, output, extent_hint.right, extent_hint.top);
  DEBUG_PIXEL(out_ptr, output, extent_hint.right, extent_hint.bottom);

  END_PROFILE();

  return err;
}

//----------------------------------------------------------------------------//
// Summary: Rendering
// Name: Render
// Arguments:
// Returns:
//----------------------------------------------------------------------------//
static PF_Err Render(PF_InData *in_data, PF_OutData *out_data,
                     PF_ParamDef *params[], PF_LayerDef *output) {
  PF_Err err = PF_Err_NONE;

  PF_LayerDef *input = &params[PARAM_INPUT]->u.ld;

  PF_Pixel16 *in_ptr16, *out_ptr16;
  PF_GET_PIXEL_DATA16(output, NULL, &out_ptr16);
  PF_GET_PIXEL_DATA16(input, NULL, &in_ptr16);

  if (out_ptr16 != NULL && in_ptr16 != NULL) {
    // 16bpc or 32bpc
    err = smoothing<PF_Pixel16, KP_PIXEL64>(in_data, out_data, params, input,
                                            output, in_ptr16, out_ptr16);
  } else {
    // 8bpc
    PF_Pixel8 *in_ptr8, *out_ptr8;
    PF_GET_PIXEL_DATA8(output, NULL, &out_ptr8);
    PF_GET_PIXEL_DATA8(input, NULL, &in_ptr8);

    err = smoothing<PF_Pixel8, KP_PIXEL32>(in_data, out_data, params, input,
                                           output, in_ptr8, out_ptr8);
  }

  return err;
}

//----------------------------------------------------------------------------//
// Create Dialog Window
//----------------------------------------------------------------------------//
static PF_Err PopDialog(PF_InData *in_data, PF_OutData *out_data,
                        PF_ParamDef *params[], PF_LayerDef *output) {
  PF_Err err = PF_Err_NONE;

  char str[256];
  memset(str, 0, 256);

  sprintf(out_data->return_msg, "%s, v%d.%d.%d\n%s\n", NAME, MAJOR_VERSION,
          MINOR_VERSION, BUILD_VERSION, str);

  return err;
}
