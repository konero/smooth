//----------------------------------------------------------------------------//
// Processing Lower Angles
//----------------------------------------------------------------------------//

#include "util.h"
#include "define.h"

#include "downMode.h"

#include <stdio.h>
#include <math.h>

//----------------------------------------------------------------------------//
// Name:        downMode_????CountLength
// Description: Counts the length to apply antialiasing.
// Arguments:   info: Structure containing count information.
// Returns:     None
//----------------------------------------------------------------------------//

// Left side count for antialiasing length
template <typename PixelType>
void downMode_LeftCountLength(BlendingInfo<PixelType> *info)
{
    PF_LayerDef *output = info->output;

    long count_target = 0;
    int len = 1,
        width = GET_WIDTH(info->input),
        height = output->height;
    u_int *flg = &info->core[0].flg;

    while (1)
    {
        count_target = info->in_target - (len - 1); // Change the target for examination

        // Check if it's a solid fill
        if (ComparePixel(count_target, count_target - 1))
        { // Scan from the bottom

            info->core[0].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[0].end = (float)(info->i + 1) - (float)len;

            // Set a flag or perform special processing if needed
            (*flg) |= CR_FLG_FILL;

            break;
        }

        count_target = info->in_target + width - (len - 1); // Change the target for examination ******

        if (ComparePixel(count_target, count_target - 1))
        { // Scan from the top
            info->core[0].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[0].end = (float)(info->i + 1) - (float)len;

            if (width - 2 > info->i && info->i > 2 &&
                height - 2 > info->j && info->j > 2)
            {
                // Check if end value correction is needed
                // Examine the length of the corner above the line, and if the difference is 1, make a correction
                if (!(info->flag & SECOND_COUNT) &&
                    ComparePixel(count_target - 1, count_target - 1 + width)) // 角かどうか調べる // **********
                {
                    // Count there
                    BlendingInfo<PixelType> sc_info;

                    sc_info = *info; // Copy and initialize

                    sc_info.i = info->i - len;
                    sc_info.j = info->j + 1; // **************
                    sc_info.out_target =
                        sc_info.in_target = sc_info.j * width + sc_info.i;
                    sc_info.flag = SECOND_COUNT;

                    downMode_LeftCountLength(&sc_info);

                    if (sc_info.core[0].length - len == 1)
                    {
                        info->core[0].end -= 0.5f; // Correct for something half a pixel to the left
                    }
                }
            }

            break;
        }

        len++;

        // End if it reaches x = 0
        if (info->i - len <= 1)
        {
            len = info->i - 1;

            info->core[0].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[0].end = (float)(info->i + 1) - (float)len;
            break;
        }
    }

    info->core[0].length = len;
}

//----------------------------------------------------------------------------//

// Right side count for antialiasing length
template <typename PixelType>
void downMode_RightCountLength(BlendingInfo<PixelType> *info)
{
    PF_LayerDef *output = info->output;

    long count_target = 0;
    int len = 0,
        width = GET_WIDTH(info->input),
        height = output->height;
    u_int *flg = &info->core[1].flg;

    // First check only the left side
    count_target = info->in_target - width; // Change the target for examination ******
    if (ComparePixel(count_target, count_target + 1) &&
        ComparePixelEqual(count_target + 1, count_target + 1 + width))
    {
        info->core[1].length = 0;
        return;
    }
    else
    {
        len++;

        if ((info->i + 1) + len >= (width - 1))
        {
            len = width - 1 - (info->i + 1);

            info->core[1].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[1].end = (float)(info->i + 1) + (float)len;
            info->core[1].length = len;
            return;
        }
    }

    while (1)
    {
        count_target = info->in_target + len; // Change the target for examination

        // Check if it's a solid fill
        if (ComparePixel(count_target, count_target + 1))
        {
            info->core[1].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[1].end = (float)(info->i + 1 + len);

            // Set a flag or perform special processing if needed
            (*flg) |= CR_FLG_FILL;

            break;
        }

        count_target = info->in_target - width + len; // Change the target for examination *********

        if (ComparePixel(count_target, count_target + 1))
        {
            info->core[1].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[1].end = (float)(info->i + 1 + len);

            if (width - 2 > info->i && info->i > 2 &&
                height - 2 > info->j && info->j > 2)
            {
                // Check if end value correction is needed
                // Examine the length of the corner below the line, and if the difference is 1, make a correction
                // Check if it's a corner
                if (!(info->flag & SECOND_COUNT) &&
                    ComparePixel(count_target, count_target + 1))
                {
                    // Count there
                    BlendingInfo<PixelType> sc_info;

                    sc_info = *info; // Copy and initialize

                    sc_info.i = info->i + len;
                    sc_info.j = info->j - 1; // *********
                    sc_info.out_target =
                        sc_info.in_target = sc_info.j * width + sc_info.i;
                    sc_info.flag = SECOND_COUNT;

                    downMode_RightCountLength(&sc_info);

                    if (len - sc_info.core[1].length == 1 && sc_info.core[1].length != 0)
                    {
                        info->core[1].end -= 0.5f; // Correct for something half a pixel to the right
                    }
                }
            }

            break;
        }

        len++;

        // End if it reaches x = 0
        if ((info->i + 1) + len >= (width - 1))
        {
            len = width - 1 - (info->i + 1);

            info->core[1].start = (float)(info->i + 1); // Image coordinates start from top-left (0,0), but logical lines start from right
            info->core[1].end = (float)(info->i + 1) + (float)len;
            break;
        }
    }

    info->core[1].length = len;
}

//----------------------------------------------------------------------------//

// Top side count for antialiasing length
template <typename PixelType>
void downMode_TopCountLength(BlendingInfo<PixelType> *info)
{
    PF_LayerDef *output = info->output;

    long count_target = 0;
    int len = 1,
        width = GET_WIDTH(info->input),
        height = output->height;
    u_int *flg = &info->core[2].flg;

    while (1)
    {
        count_target = info->in_target - (len - 1) * width; // Change the target for examination ****

        // Check if it's a solid fill
        if (ComparePixel(count_target, count_target - width)) // *******
        {
            info->core[2].start = (float)(info->j);
            info->core[2].end = (float)(info->j - len); // ******

            // It's a solid fill //
            // DEBUG_PIXEL( info->in_target, DEBUG_COL_BLUE);

            (*flg) |= CR_FLG_FILL;

            break;
        }

        count_target = info->in_target - (len - 1) * width + 1; // Change the target for examination ***

        if (ComparePixel(count_target, count_target - width)) // *******
        {
            info->core[2].start = (float)(info->j);
            info->core[2].end = (float)(info->j - len); // ******

            if (width - 2 > info->i && info->i > 2 &&
                height - 2 > info->j && info->j > 2)
            {
                // Check if end value correction is needed
                // Examine the length of the corner below the line, and if the difference is 1, make a correction
                // Check if it's a corner
                if (!(info->flag & SECOND_COUNT) &&
                    ComparePixel(count_target - width, count_target - width + 1)) // *******
                {
                    // Count there //
                    BlendingInfo<PixelType> sc_info;

                    sc_info = *info; // Copy and initialize

                    sc_info.i = info->i + 1;
                    sc_info.j = info->j - len; // *******
                    sc_info.out_target =
                        sc_info.in_target = sc_info.j * width + sc_info.i;
                    sc_info.flag = SECOND_COUNT;

                    downMode_TopCountLength(&sc_info);

                    if (len - sc_info.core[2].length == 1)
                    {
                        info->core[2].end += 0.5f; // Correct for something half a pixel to the right
                    }
                }
            }

            break;
        }

        len++;

        // End if it reaches y = 0 (the top of the image) // *****
        if ((info->j + 1) - len <= 1)
        {
            len = info->j + 1 - 1;

            info->core[2].start = (float)(info->j);
            info->core[2].end = (float)(info->j - len);

            break;
        }
    }

    info->core[2].length = len;
}

//----------------------------------------------------------------------------//

// Bottom side count for antialiasing length
template <typename PixelType>
void downMode_BottomCountLength(BlendingInfo<PixelType> *info)
{
    PF_LayerDef *output = info->output;

    long count_target = 0;
    int len = 0,
        width = GET_WIDTH(info->input),
        height = output->height;
    u_int *flg = &info->core[3].flg;

    //----------------------------------------------------------//
    //              Counting length for the top side (top_len)             //
    //----------------------------------------------------------//
    // Initially, only examine towards the left side //
    count_target = info->in_target - 1; // Change the target for examination

    if (ComparePixel(count_target, count_target + width) &&
        ComparePixelEqual(count_target + width, count_target + 1 + width))
    {
        info->core[3].length = 0;
        return;
    }
    else
    {
        len++;

        if ((info->j + 1) + len >= height - 1)
        {
            len = height - 1 - (info->j + 1);

            info->core[3].start = (float)(info->j);
            info->core[3].end = (float)(info->j - len);
            info->core[3].length = len;
            return;
        }
    }

    while (1)
    {
        count_target = info->in_target + (len * width); // Change the target for examination *****

        // Is it a solid color area?
        //--------------------------------------------------------------------//
        if (ComparePixel(count_target, count_target + width)) // ******
        {
            info->core[3].start = (float)(info->j);
            info->core[3].end = (float)(info->j + len); // *******

            // It's a solid color area //
            // DEBUG_PIXEL( info->in_target, DEBUG_COL_BLUE);

            (*flg) |= CR_FLG_FILL;

            break;
        }

        count_target = info->in_target + (len * width) - 1; // Change the target for examination ****

        // Check pixel by pixel, does it change color?
        //--------------------------------------------------------------------//
        if (ComparePixel(count_target, count_target + width)) // *********
        {
            info->core[3].start = (float)(info->j);
            info->core[3].end = (float)(info->j + len); // *******

            if (width - 2 > info->i && info->i > 2 &&
                height - 2 > info->j && info->j > 2)
            {
                // Does the end value need correction?
                // Check the length of the angle one pixel below the line, if the difference is one, correct it
                // Check if it's an angle 
                if (!(info->flag & SECOND_COUNT) &&
                    ComparePixel(count_target, count_target + 1))
                {
                    // Count here //
                    BlendingInfo<PixelType> sc_info;

                    sc_info = *info; // Copy and initialize

                    sc_info.i = info->i - 1;
                    sc_info.j = info->j + len; // *******
                    sc_info.out_target =
                        sc_info.in_target = sc_info.j * width + sc_info.i;
                    sc_info.flag = SECOND_COUNT;

                    downMode_BottomCountLength(&sc_info);

                    if (sc_info.core[3].length - len == 1)
                    {
                        info->core[3].end += 0.5f; // Correct for half a pixel to the right
                    }
                }
            }

            break;
        }

        len++;

        // Finish if we've reached the end ****
        if ((info->j + 1) + len >= height - 1)
        {
            len = height - 1 - (info->j + 1);

            info->core[3].start = (float)(info->j);
            info->core[3].end = (float)(info->j + len);
            break;
        }
    }

    // Copy
    info->core[3].length = len;
}

//----------------------------------------------------------------------------//
// Name:        downMode_????Blending()
// Description: Performs anti-aliasing blending
// Arguments:   info: Blending information structure
// Returns:     None
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

// Left
template <typename PixelType>
void downMode_LeftBlending(BlendingInfo<PixelType> *info)
{
    int t;
    float start = info->core[0].start;
    float end = info->core[0].end;
    int in_width = GET_WIDTH(info->input);

    // Normally, the Length value is halved for use, but in this case, the unit is 1/2 pixel, so it's doubled.
    // len*(1/2)*2 = len, so it can be used directly
#if 0
    if(flg & CR_FLG_FILL)
    {
        //----------------------//
        //      Fill Mode       //
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
    {
        //--------------//
        //     Normal    //
        //--------------//
        long blend_target = 0, out_target = 0;
        int blend_count;
        float pre_ratio, // Previous triangle area ratio
            ratio,       // Current area ratio
            l,           // Base length
            len;         // Total base length
        int end_p;       // Starting pixel (named end because it's related to info->end)

        end_p = (int)end;

        len = start - end;

        // Number of pixels to blend, rounded up
        blend_count = CEIL((float)(info->i + 1) - end);

        // Initialize previous ratio
        pre_ratio = 0.0f;

        // Initialize blend target
        blend_target = info->in_target - (blend_count - 1);
        out_target = info->out_target - (blend_count - 1);

        // Base × height ÷ 2 = Base × ((Base / Total Base) × Total Height) ÷ 2 = l * l * 0.5 * 0.5 / len
        // Calculated from the left
        for (t = 0; t < blend_count; t++)
        {
            l = (float)(end_p + 1 + t) - end;
            ratio = (l * l * 0.5f * 0.5f) / len;

            // Blend
            Blendingf(info->in_ptr,
                      info->out_ptr,
                      blend_target,
                      blend_target + in_width,
                      out_target,
                      1.0f - (ratio - pre_ratio));

            pre_ratio = ratio;

            blend_target++;
            out_target++;
        }
    }
}

//----------------------------------------------------------------------------//

// Right
template <typename PixelType>
void downMode_RightBlending(BlendingInfo<PixelType> *info)
{
    long t;
    long length = info->core[1].length;
    float start = info->core[1].start;
    float end = info->core[1].end;
    int in_width = GET_WIDTH(info->input);
#if 0
    else if(flg & CR_FLG_FILL)
    {
        //----------------------//
        //      Fill Mode       //
        //----------------------//
    }
#endif
    {
        //--------------//
        //     Normal     //
        //--------------//
        if (length > 0)
        {
            long blend_target = 0, out_target = 0;
            int blend_count;
            float pre_ratio, // Previous triangle area ratio
                ratio,       // Current area ratio
                l,           // Base length
                len;         // Total base length
            int end_p;       // Starting pixel (named end because it's related to info->end)

            // This end_p is needed? The calculation of the ratio doesn't depend on coordinate values, so it might not be necessary? Referring to link 8
            end_p = (int)(end - 0.000001); // A workaround for exact values like 4.0f causing issues

            len = end - start;

            // Number of pixels to blend, rounded up
            blend_count = CEIL(end - (float)(info->i + 1));

            // Initialize previous ratio
            pre_ratio = 0.0f;

            // Initialize blend target
            blend_target = info->in_target + blend_count;
            out_target = info->out_target + blend_count;

            // Calculated from the right
            for (t = 0; t < blend_count; t++)
            {
                l = end - (float)(end_p - t);
                ratio = (l * l * 0.5f * 0.5f) / len;

                // Blend
                Blendingf(info->in_ptr,
                          info->out_ptr,
                          blend_target,
                          blend_target - in_width,
                          out_target,
                          1.0f - (ratio - pre_ratio)); // ******

                pre_ratio = ratio;

                blend_target--;
                out_target--;
            }
        }
    }
}

//----------------------------------------------------------------------------//

// Top
template <typename PixelType>
void downMode_TopBlending(BlendingInfo<PixelType> *info)
{
    long t;
    float start = info->core[2].start;
    float end = info->core[2].end;
    int in_width = GET_WIDTH(info->input);
    int out_width = GET_WIDTH(info->output);

#if 0
    else if(flg & CR_FLG_FILL)
    {
        //----------------------//
        //      Fill Mode       //
        //----------------------//
    }
#endif
    { // Normal
        long blend_target = 0, out_target = 0;
        int blend_count;
        float pre_ratio, // Previous triangle area ratio
            ratio,       // Current area ratio
            l,           // Base length
            len;         // Total base length
        int end_p;       // Starting pixel (named end because it's related to info->end)

        end_p = (int)end;

        len = start - end; // Calculate the total base length *****

        // Number of pixels to blend, rounded up
        blend_count = CEIL((float)(info->j) - end); // ****

        // Initialize previous ratio
        pre_ratio = 0.0f;

        // Initialize blend target
        blend_target = info->in_target - (blend_count - 1) * in_width; // info->targetが最終処理ピクセルなので-1 // ******:
        out_target = info->out_target - (blend_count - 1) * out_width;

        // Calculated from the bottom
        for (t = 0; t < blend_count; t++)
        {
            l = (float)(end_p + 1 + t) - end; // Calculate the base length for each pixel blend // ****
            ratio = (l * l * 0.5f * 0.5f) / len; // Calculate the area ratio for each pixel blend

            // Blend
            Blendingf(info->in_ptr,
                      info->out_ptr,
                      blend_target,
                      blend_target + 1,
                      out_target,
                      1.0f - (ratio - pre_ratio)); // Blend based on the area ratio

            pre_ratio = ratio;

            blend_target += in_width; // Move to the next pixel in the input buffer // ******
            out_target += out_width; // Move to the next pixel in the output buffer // ******
        }
    }
}

//----------------------------------------------------------------------------//

// Bottom
template <typename PixelType>
void downMode_BottomBlending(BlendingInfo<PixelType> *info)
{
    long t;
    long length = info->core[3].length;
    float start = info->core[3].start;
    float end = info->core[3].end;
    int in_width = GET_WIDTH(info->input);
    int out_width = GET_WIDTH(info->output);

    // Normally, the Length value is halved, but in this case, since the unit is half-pixel units, it's doubled, len*(1/2)*2=len, so we can use it as is.

#if 0
    else if(flg->top & CR_FLG_FILL)
    {
        //----------------------//
        //      Fill Mode       //
        //----------------------//
    }
#endif
    { // Normal
        if (length > 0)
        {
            long blend_target = 0, out_target = 0;
            int blend_count;
            float pre_ratio, // Previous triangle area ratio
                ratio,       // Current area ratio
                l,           // Base length
                len;         // Total base length
            int end_p;       // Starting pixel (named end because it's related to info->end)

            end_p = (int)(end - 0.00001); // A workaround for cases like 4.0f where it becomes incorrect

            len = end - start; // Calculate the total base length ***:

            // Number of pixels to blend, rounded up
            blend_count = CEIL(end - (float)(info->j)); // *****:

            // Initialize previous ratio
            pre_ratio = 0.0f;

            // Initialize blend target
            blend_target = info->in_target + blend_count * in_width; // *******
            out_target = info->out_target + blend_count * out_width;

            // Calculated from the top
            for (t = 0; t < blend_count; t++)
            {
                l = end - (float)(end_p - t); // Calculate the base length for each pixel blend // *****
                ratio = (l * l * 0.5f * 0.5f) / len; // Calculate the area ratio for each pixel blend

                // Blend
                Blendingf(info->in_ptr,
                          info->out_ptr,
                          blend_target,
                          blend_target - 1,
                          out_target,
                          1.0f - (ratio - pre_ratio));

                pre_ratio = ratio;

                blend_target -= in_width; // *******
                out_target -= out_width;  // *******
            }
        }
    }
}

// Explicit instantiation declarations
template void downMode_LeftCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_LeftCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_RightCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_RightCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_TopCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_TopCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_BottomCountLength<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_BottomCountLength<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_LeftBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_LeftBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_RightBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_RightBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_TopBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_TopBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);

template void downMode_BottomBlending<PF_Pixel8>(BlendingInfo<PF_Pixel8> *info);
template void downMode_BottomBlending<PF_Pixel16>(BlendingInfo<PF_Pixel16> *info);
