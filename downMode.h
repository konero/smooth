//----------------------------------------------------------------------------//
// Processing Lower Angles
//----------------------------------------------------------------------------//

#ifndef __DOWNMODE_H
#define __DOWNMODE_H

// Length count
template <typename PixelType>
void downMode_LeftCountLength(BlendingInfo<PixelType> *info); // left

template <typename PixelType>
void downMode_RightCountLength(BlendingInfo<PixelType> *info); // right

template <typename PixelType>
void downMode_TopCountLength(BlendingInfo<PixelType> *info); // top

template <typename PixelType>
void downMode_BottomCountLength(BlendingInfo<PixelType> *info); // bottom

// Blend
template <typename PixelType>
void downMode_LeftBlending(BlendingInfo<PixelType> *info); // left blend

template <typename PixelType>
void downMode_RightBlending(BlendingInfo<PixelType> *info); // right blend

template <typename PixelType>
void downMode_TopBlending(BlendingInfo<PixelType> *info); // top blend

template <typename PixelType>
void downMode_BottomBlending(BlendingInfo<PixelType> *info); // bottom blend

#endif
