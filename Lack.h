//----------------------------------------------------------------------------//
// Processing of corners that appear to be missing more than 2 pixels
//----------------------------------------------------------------------------//

#ifndef __LACK_H
#define __LACK_H

template <typename PixelType>
void LackMode01Execute(BlendingInfo<PixelType> *info);

template <typename PixelType>
void LackMode02Execute(BlendingInfo<PixelType> *info);

template <typename PixelType>
void LackMode0304Execute(BlendingInfo<PixelType> *info);

#endif
