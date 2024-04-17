//----------------------------------------------------------------------------//
// Processing of Upper-Angle Systems
//----------------------------------------------------------------------------//

#ifndef __UPMODE_H
#define __UPMODE_H

// Length count on the left side
//----------------------------------------------------------------------------//
template <typename PixelType>
void upMode_LeftCountLength(BlendingInfo<PixelType> *info); // Left

template <typename PixelType>
void upMode_RightCountLength(BlendingInfo<PixelType> *info); // Right

template <typename PixelType>
void upMode_TopCountLength(BlendingInfo<PixelType> *info); // Top

template <typename PixelType>
void upMode_BottomCountLength(BlendingInfo<PixelType> *info); // Bottom

// Blend
//----------------------------------------------------------------------------//
template <typename PixelType>
void upMode_LeftBlending(BlendingInfo<PixelType> *info); // Left blend

template <typename PixelType>
void upMode_RightBlending(BlendingInfo<PixelType> *info); // Right blend

template <typename PixelType>
void upMode_TopBlending(BlendingInfo<PixelType> *info); // Top blend

template <typename PixelType>
void upMode_BottomBlending(BlendingInfo<PixelType> *info); // Bottom blend

#endif
