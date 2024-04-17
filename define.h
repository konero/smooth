//----------------------------------------------------------------------------//
// Summary: Functions and structures used within the smooth function
// Updated: April 21, 2003
//----------------------------------------------------------------------------//

#ifndef __DEFINE_H
#define __DEFINE_H

// Types
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned long u_long;

// After Effects uses ARGB

struct Cinfo
{
    long length;      // Number of counted pixels
    float start, end; // Coordinates for actual blending (only valid for the corresponding direction)
    u_int flg;        // Flag for special processing

    Cinfo()
    {
        length = 0;
        flg = 0;
    } // Constructor
};

// Flag for the above structure
#define CR_FLG_FILL (1 << 0) // Fill mode flag

// Structure for blending information
template <typename PixelType>
struct BlendingInfo
{
    PF_LayerDef *input, *output; // Input and output images
    PixelType *in_ptr, *out_ptr; // Pointers to image data
    int i, j;                    // Current processing coordinates
    long in_target,              // 1D array index of the processing coordinates (for input)
        out_target;              // 1D array index of the processing coordinates (for output)
    Cinfo core[4];               // Information about the length to process, flags, etc. 0:left 1:right 2:top 3:bottom
    int flag;                    // Control flag (for correction counts, etc.)
    unsigned int range;          // Range to consider as the same color
    int mode;                    // Processing mode

    float LineWeight; // Line thickness
};

// Flag for the above structure
#define SECOND_COUNT (1 << 0) // Flag for second count for correction

// Modes for the above structure
enum
{
    BLEND_MODE_UP_H = 0, // Upward direction, horizontal angle
    BLEND_MODE_UP_V,     // Upward direction, vertical angle
    BLEND_MODE_DOWN_H,   // Downward direction, horizontal angle
    BLEND_MODE_DOWN_V,   // Downward direction, vertical angle
};

#endif
