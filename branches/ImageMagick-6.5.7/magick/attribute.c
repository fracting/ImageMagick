/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         AAA   TTTTT  TTTTT  RRRR   IIIII  BBBB   U   U  TTTTT  EEEEE        %
%        A   A    T      T    R   R    I    B   B  U   U    T    E            %
%        AAAAA    T      T    RRRR     I    BBBB   U   U    T    EEE          %
%        A   A    T      T    R R      I    B   B  U   U    T    E            %
%        A   A    T      T    R  R   IIIII  BBBB    UUU     T    EEEEE        %
%                                                                             %
%                                                                             %
%                    MagickCore Get / Set Image Attributes                    %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                October 2002                                 %
%                                                                             %
%                                                                             %
%  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/cache-view.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colormap.h"
#include "magick/colormap-private.h"
#include "magick/colorspace.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/deprecate.h"
#include "magick/draw.h"
#include "magick/draw-private.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/histogram.h"
#include "magick/identify.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/magick.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/paint.h"
#include "magick/pixel.h"
#include "magick/pixel-private.h"
#include "magick/property.h"
#include "magick/quantize.h"
#include "magick/random_.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/segment.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/thread-private.h"
#include "magick/threshold.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I m a g e B o u n d i n g B o x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageBoundingBox() returns the bounding box of an image canvas.
%
%  The format of the GetImageBoundingBox method is:
%
%      RectangleInfo GetImageBoundingBox(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o bounds: Method GetImageBoundingBox returns the bounding box of an
%      image canvas.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport RectangleInfo GetImageBoundingBox(const Image *image,
  ExceptionInfo *exception)
{
  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    target[3],
    zero;

  RectangleInfo
    bounds;

  register const PixelPacket
    *p;

  CacheView
    *image_view;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  bounds.width=0;
  bounds.height=0;
  bounds.x=(long) image->columns;
  bounds.y=(long) image->rows;
  GetMagickPixelPacket(image,&target[0]);
  image_view=AcquireCacheView(image);
  p=GetCacheViewVirtualPixels(image_view,0,0,1,1,exception);
  if (p == (const PixelPacket *) NULL)
    {
      image_view=DestroyCacheView(image_view);
      return(bounds);
    }
  SetMagickPixelPacket(image,p,GetCacheViewAuthenticIndexQueue(image_view),
    &target[0]);
  GetMagickPixelPacket(image,&target[1]);
  p=GetCacheViewVirtualPixels(image_view,(long) image->columns-1,0,1,1,
    exception);
  SetMagickPixelPacket(image,p,GetCacheViewAuthenticIndexQueue(image_view),
    &target[1]);
  GetMagickPixelPacket(image,&target[2]);
  p=GetCacheViewVirtualPixels(image_view,0,(long) image->rows-1,1,1,exception);
  SetMagickPixelPacket(image,p,GetCacheViewAuthenticIndexQueue(image_view),
    &target[2]);
  status=MagickTrue;
  GetMagickPixelPacket(image,&zero);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    MagickPixelPacket
      pixel;

    RectangleInfo
      bounding_box;

    register const IndexPacket
      *__restrict indexes;

    register const PixelPacket
      *__restrict p;

    register long
      x;

    if (status == MagickFalse)
      continue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
#  pragma omp critical (MagickCore_GetImageBoundingBox)
#endif
    bounding_box=bounds;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    pixel=zero;
    for (x=0; x < (long) image->columns; x++)
    {
      SetMagickPixelPacket(image,p,indexes+x,&pixel);
      if ((x < bounding_box.x) &&
          (IsMagickColorSimilar(&pixel,&target[0]) == MagickFalse))
        bounding_box.x=x;
      if ((x > (long) bounding_box.width) &&
          (IsMagickColorSimilar(&pixel,&target[1]) == MagickFalse))
        bounding_box.width=(unsigned long) x;
      if ((y < bounding_box.y) &&
          (IsMagickColorSimilar(&pixel,&target[0]) == MagickFalse))
        bounding_box.y=y;
      if ((y > (long) bounding_box.height) &&
          (IsMagickColorSimilar(&pixel,&target[2]) == MagickFalse))
        bounding_box.height=(unsigned long) y;
      p++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
#  pragma omp critical (MagickCore_GetImageBoundingBox)
#endif
    {
      if (bounding_box.x < bounds.x)
        bounds.x=bounding_box.x;
      if (bounding_box.y < bounds.y)
        bounds.y=bounding_box.y;
      if (bounding_box.width > bounds.width)
        bounds.width=bounding_box.width;
      if (bounding_box.height > bounds.height)
        bounds.height=bounding_box.height;
    }
  }
  image_view=DestroyCacheView(image_view);
  if ((bounds.width == 0) || (bounds.height == 0))
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "GeometryDoesNotContainImage","`%s'",image->filename);
  else
    {
      bounds.width-=(bounds.x-1);
      bounds.height-=(bounds.y-1);
    }
  return(bounds);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelDepth() returns the depth of a particular image channel.
%
%  The format of the GetImageChannelDepth method is:
%
%      unsigned long GetImageDepth(const Image *image,ExceptionInfo *exception)
%      unsigned long GetImageChannelDepth(const Image *image,
%        const ChannelType channel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel: the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport unsigned long GetImageDepth(const Image *image,
  ExceptionInfo *exception)
{
  return(GetImageChannelDepth(image,AllChannels,exception));
}

MagickExport unsigned long GetImageChannelDepth(const Image *image,
  const ChannelType channel,ExceptionInfo *exception)
{
  long
    y;

  MagickBooleanType
    status;

  register long
    id;

  unsigned long
    *current_depth,
    depth,
    number_threads;

  CacheView
    *image_view;

  /*
    Compute image depth.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  number_threads=GetOpenMPMaximumThreads();
  current_depth=(unsigned long *) AcquireQuantumMemory(number_threads,
    sizeof(*current_depth));
  if (current_depth == (unsigned long *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  status=MagickTrue;
  for (id=0; id < (long) number_threads; id++)
    current_depth[id]=1;
  if ((image->storage_class == PseudoClass) && (image->matte == MagickFalse))
    {
      register const PixelPacket
        *__restrict p;

      register long
        i;

      p=image->colormap;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
      for (i=0; i < (long) image->colors; i++)
      {
        if (status == MagickFalse)
          continue;
        id=GetOpenMPThreadId();
        while (current_depth[id] < MAGICKCORE_QUANTUM_DEPTH)
        {
          MagickStatusType
            status;

          QuantumAny
            range;

          status=0;
          range=GetQuantumRange(current_depth[id]);
          if ((channel & RedChannel) != 0)
            status|=p->red != ScaleAnyToQuantum(ScaleQuantumToAny(p->red,
              range),range);
          if ((channel & GreenChannel) != 0)
            status|=p->green != ScaleAnyToQuantum(ScaleQuantumToAny(p->green,
              range),range);
          if ((channel & BlueChannel) != 0)
            status|=p->blue != ScaleAnyToQuantum(ScaleQuantumToAny(p->blue,
              range),range);
          if (status == 0)
            break;
          current_depth[id]++;
        }
        p++;
      }
      depth=current_depth[0];
      for (id=1; id < (long) number_threads; id++)
        if (depth < current_depth[id])
          depth=current_depth[id];
      current_depth=(unsigned long *) RelinquishMagickMemory(current_depth);
      return(depth);
    }
  image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    register const IndexPacket
      *__restrict indexes;

    register const PixelPacket
      *__restrict p;

    register long
      id,
      x;

    if (status == MagickFalse)
      continue;
    id=GetOpenMPThreadId();
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      continue;
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    for (x=0; x < (long) image->columns; x++)
    {
      while (current_depth[id] < MAGICKCORE_QUANTUM_DEPTH)
      {
        MagickStatusType
          status;

        QuantumAny
          range;

        status=0;
        range=GetQuantumRange(current_depth[id]);
        if ((channel & RedChannel) != 0)
          status|=p->red != ScaleAnyToQuantum(ScaleQuantumToAny(p->red,range),
            range);
        if ((channel & GreenChannel) != 0)
          status|=p->green != ScaleAnyToQuantum(ScaleQuantumToAny(p->green,
            range),range);
        if ((channel & BlueChannel) != 0)
          status|=p->blue != ScaleAnyToQuantum(ScaleQuantumToAny(p->blue,range),
            range);
        if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
          status|=p->opacity != ScaleAnyToQuantum(ScaleQuantumToAny(p->opacity,
            range),range);
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          status|=indexes[x] != ScaleAnyToQuantum(ScaleQuantumToAny(indexes[x],
            range),range);
        if (status == 0)
          break;
        current_depth[id]++;
      }
      p++;
    }
    if (current_depth[id] == MAGICKCORE_QUANTUM_DEPTH)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  depth=current_depth[0];
  for (id=1; id < (long) number_threads; id++)
    if (depth < current_depth[id])
      depth=current_depth[id];
  current_depth=(unsigned long *) RelinquishMagickMemory(current_depth);
  return(depth);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e Q u a n t u m D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageQuantumDepth() returns the depth of the image rounded to a legal
%  quantum depth: 8, 16, or 32.
%
%  The format of the GetImageQuantumDepth method is:
%
%      unsigned long GetImageQuantumDepth(const Image *image,
%        const MagickBooleanType constrain)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o constrain: A value other than MagickFalse, constrains the depth to
%      a maximum of MAGICKCORE_QUANTUM_DEPTH.
%
*/

static inline double MagickMin(const double x,const double y)
{
  if (x < y)
    return(x);
  return(y);
}

MagickExport unsigned long GetImageQuantumDepth(const Image *image,
  const MagickBooleanType constrain)
{
  unsigned long
    depth;

  depth=image->depth;
  if (depth <= 8)
    depth=8;
  else
    if (depth <= 16)
      depth=16;
    else
      if (depth <= 32)
        depth=32;
      else
        if (depth <= 64)
          depth=64;
  if (constrain != MagickFalse)
    depth=(unsigned long) MagickMin((double) depth,(double)
      MAGICKCORE_QUANTUM_DEPTH);
  return(depth);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e T y p e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageType() returns the potential type of image:
%
%        Bilevel         Grayscale        GrayscaleMatte
%        Palette         PaletteMatte     TrueColor
%        TrueColorMatte  ColorSeparation  ColorSeparationMatte
%
%  To ensure the image type matches its potential, use SetImageType():
%
%    (void) SetImageType(image,GetImageType(image));
%
%  The format of the GetImageType method is:
%
%      ImageType GetImageType(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport ImageType GetImageType(const Image *image,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->colorspace == CMYKColorspace)
    {
      if (image->matte == MagickFalse)
        return(ColorSeparationType);
      return(ColorSeparationMatteType);
    }
  if (IsMonochromeImage(image,exception) != MagickFalse)
    return(BilevelType);
  if (IsGrayImage(image,exception) != MagickFalse)
    {
      if (image->matte != MagickFalse)
        return(GrayscaleMatteType);
      return(GrayscaleType);
    }
  if (IsPaletteImage(image,exception) != MagickFalse)
    {
      if (image->matte != MagickFalse)
        return(PaletteMatteType);
      return(PaletteType);
    }
  if (image->matte != MagickFalse)
    return(TrueColorMatteType);
  return(TrueColorType);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s G r a y I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGrayImage() returns MagickTrue if all the pixels in the image have the
%  same red, green, and blue intensities.
%
%  The format of the IsGrayImage method is:
%
%      MagickBooleanType IsGrayImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType IsGrayImage(const Image *image,
  ExceptionInfo *exception)
{
  ImageType
    type;

  register const PixelPacket
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->type == BilevelType) || (image->type == GrayscaleType) ||
      (image->type == GrayscaleMatteType))
    return(MagickTrue);
  if (image->colorspace == CMYKColorspace)
    return(MagickFalse);
  type=BilevelType;
  switch (image->storage_class)
  {
    case DirectClass:
    case UndefinedClass:
    {
      long
        y;

      register long
        x;

      CacheView
        *image_view;

      image_view=AcquireCacheView(image);
      for (y=0; y < (long) image->rows; y++)
      {
        p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          if (IsGrayPixel(p) == MagickFalse)
            {
              type=UndefinedType;
              break;
            }
          if ((type == BilevelType) && (IsMonochromePixel(p) == MagickFalse))
            type=GrayscaleType;
          p++;
        }
        if (type == UndefinedType)
          break;
      }
      image_view=DestroyCacheView(image_view);
      break;
    }
    case PseudoClass:
    {
      register long
        i;

      p=image->colormap;
      for (i=0; i < (long) image->colors; i++)
      {
        if (IsGrayPixel(p) == MagickFalse)
          {
            type=UndefinedType;
            break;
          }
        if ((type == BilevelType) && (IsMonochromePixel(p) == MagickFalse))
          type=GrayscaleType;
        p++;
      }
      break;
    }
  }
  if (type == UndefinedType)
    return(MagickFalse);
  ((Image *) image)->type=type;
  if ((type == GrayscaleType) && (image->matte != MagickFalse))
    ((Image *) image)->type=GrayscaleMatteType;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M o n o c h r o m e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMonochromeImage() returns MagickTrue if all the pixels in the image have
%  the same red, green, and blue intensities and the intensity is either
%  0 or QuantumRange.
%
%  The format of the IsMonochromeImage method is:
%
%      MagickBooleanType IsMonochromeImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType IsMonochromeImage(const Image *image,
  ExceptionInfo *exception)
{
  ImageType
    type;

  register const PixelPacket
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->type == BilevelType)
    return(MagickTrue);
  if (image->colorspace == CMYKColorspace)
    return(MagickFalse);
  type=BilevelType;
  switch (image->storage_class)
  {
    case DirectClass:
    case UndefinedClass:
    {
      long
        y;

      register long
        x;

      CacheView
        *image_view;

      image_view=AcquireCacheView(image);
      for (y=0; y < (long) image->rows; y++)
      {
        p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          if (IsMonochromePixel(p) == MagickFalse)
            {
              type=UndefinedType;
              break;
            }
          p++;
        }
        if (type == UndefinedType)
          break;
      }
      image_view=DestroyCacheView(image_view);
      if (y == (long) image->rows)
        ((Image *) image)->type=BilevelType;
      break;
    }
    case PseudoClass:
    {
      register long
        i;

      p=image->colormap;
      for (i=0; i < (long) image->colors; i++)
      {
        if (IsMonochromePixel(p) == MagickFalse)
          {
            type=UndefinedType;
            break;
          }
        p++;
      }
      break;
    }
  }
  if (type == UndefinedType)
    return(MagickFalse);
  ((Image *) image)->type=type;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s O p a q u e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsOpaqueImage() returns MagickTrue if none of the pixels in the image have
%  an opacity value other than opaque (0).
%
%  The format of the IsOpaqueImage method is:
%
%      MagickBooleanType IsOpaqueImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType IsOpaqueImage(const Image *image,
  ExceptionInfo *exception)
{
  long
    y;

  register const PixelPacket
    *p;

  register long
    x;

  CacheView
    *image_view;

  /*
    Determine if image is opaque.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->matte == MagickFalse)
    return(MagickTrue);
  image_view=AcquireCacheView(image);
  for (y=0; y < (long) image->rows; y++)
  {
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      if (p->opacity != OpaqueOpacity)
        break;
      p++;
    }
    if (x < (long) image->columns)
     break;
  }
  image_view=DestroyCacheView(image_view);
  return(y < (long) image->rows ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C h a n n e l D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageChannelDepth() sets the depth of the image.
%
%  The format of the SetImageChannelDepth method is:
%
%      MagickBooleanType SetImageDepth(Image *image,const unsigned long depth)
%      MagickBooleanType SetImageChannelDepth(Image *image,
%        const ChannelType channel,const unsigned long depth)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel: the channel.
%
%    o depth: the image depth.
%
*/

MagickExport MagickBooleanType SetImageDepth(Image *image,
  const unsigned long depth)
{
  return(SetImageChannelDepth(image,AllChannels,depth));
}

MagickExport MagickBooleanType SetImageChannelDepth(Image *image,
  const ChannelType channel,const unsigned long depth)
{
  ExceptionInfo
    *exception;

  long
    y;

  MagickBooleanType
    status;

  QuantumAny
    range;

  CacheView
    *image_view;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (GetImageDepth(image,&image->exception) <= (unsigned long)
      MagickMin((double) depth,(double) MAGICKCORE_QUANTUM_DEPTH))
    {
      image->depth=depth;
      return(MagickTrue);
    }
  /*
    Scale pixels to desired depth.
  */
  status=MagickTrue;
  range=GetQuantumRange(depth);
  exception=(&image->exception);
  image_view=AcquireCacheView(image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    register IndexPacket
      *__restrict indexes;

    register long
      x;

    register PixelPacket
      *__restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (PixelPacket *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewAuthenticIndexQueue(image_view);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=ScaleAnyToQuantum(ScaleQuantumToAny(q->red,range),range);
      if ((channel & GreenChannel) != 0)
        q->green=ScaleAnyToQuantum(ScaleQuantumToAny(q->green,range),range);
      if ((channel & BlueChannel) != 0)
        q->blue=ScaleAnyToQuantum(ScaleQuantumToAny(q->blue,range),range);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=ScaleAnyToQuantum(ScaleQuantumToAny(q->opacity,range),range);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        indexes[x]=ScaleAnyToQuantum(ScaleQuantumToAny(indexes[x],range),range);
      q++;
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
  }
  image_view=DestroyCacheView(image_view);
  if (image->storage_class == PseudoClass)
    {
      QuantumAny
        range;

      register long
        i;

      register PixelPacket
        *__restrict p;

      p=image->colormap;
      range=GetQuantumRange(depth);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
      for (i=0; i < (long) image->colors; i++)
      {
        if ((channel & RedChannel) != 0)
          p->red=ScaleAnyToQuantum(ScaleQuantumToAny(p->red,range),range);
        if ((channel & GreenChannel) != 0)
          p->green=ScaleAnyToQuantum(ScaleQuantumToAny(p->green,range),range);
        if ((channel & BlueChannel) != 0)
          p->blue=ScaleAnyToQuantum(ScaleQuantumToAny(p->blue,range),range);
        if ((channel & OpacityChannel) != 0)
          p->opacity=ScaleAnyToQuantum(ScaleQuantumToAny(p->opacity,range),
            range);
        p++;
      }
    }
  image->depth=depth;
  return(status);
}
