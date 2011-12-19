/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              SSSSS  TTTTT  EEEEE   GGGG   AAA   N   N   OOO                 %
%              SS       T    E      G      A   A  NN  N  O   O                %
%               SSS     T    EEE    G  GG  AAAAA  N N N  O   O                %
%                 SS    T    E      G   G  A   A  N  NN  O   O                %
%              SSSSS    T    EEEEE   GGG   A   A  N   N   OOO                 %
%                                                                             %
%                                                                             %
%                       Write A Steganographic Image.                         %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2012 ImageMagick Studio LLC, a non-profit organization      %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/colormap.h"
#include "MagickCore/constitute.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/static.h"
#include "MagickCore/string_.h"
#include "MagickCore/module.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S T E G A N O I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadSTEGANOImage() reads a steganographic image hidden within another
%  image type.  It allocates the memory necessary for the new Image structure
%  and returns a pointer to the new image.
%
%  The format of the ReadSTEGANOImage method is:
%
%      Image *ReadSTEGANOImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline size_t MagickMin(const size_t x,
  const size_t y)
{
  if (x < y)
    return(x);
  return(y);
}

static Image *ReadSTEGANOImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define GetBit(alpha,i) MagickMin((((size_t) (alpha) >> (size_t) \
  (i)) & 0x01),16)
#define SetBit(i,set) SetPixelIndex(image,((set) != 0 ? \
  (size_t) GetPixelIndex(image,q) | (one << (size_t) (i)) : \
  (size_t) GetPixelIndex(image,q) & ~(one << (size_t) (i))),q)

  Image
    *image,
    *watermark;

  ImageInfo
    *read_info;

  int
    c;

  MagickBooleanType
    status;

  PixelInfo
    pixel;

  register Quantum
    *q;

  register ssize_t
    x;

  size_t
    depth,
    one;

  ssize_t
    i,
    j,
    k,
    y;

  /*
    Initialize Image structure.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  one=1;
  image=AcquireImage(image_info,exception);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  read_info=CloneImageInfo(image_info);
  SetImageInfoBlob(read_info,(void *) NULL,0);
  *read_info->magick='\0';
  watermark=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  if (watermark == (Image *) NULL)
    return((Image *) NULL);
  watermark->depth=MAGICKCORE_QUANTUM_DEPTH;
  if (AcquireImageColormap(image,MaxColormapSize,exception) == MagickFalse)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if (image_info->ping != MagickFalse)
    {
      (void) CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Get hidden watermark from low-order bits of image.
  */
  c=0;
  i=0;
  j=0;
  i=(ssize_t) (watermark->depth-1);
  depth=watermark->depth;
  for (k=image->offset; (i >= 0) && (j < (ssize_t) depth); i--)
  {
    for (y=0; (y < (ssize_t) image->rows) && (j < (ssize_t) depth); y++)
    {
      x=0;
      for ( ; (x < (ssize_t) image->columns) && (j < (ssize_t) depth); x++)
      {
        Quantum
          virtual_pixel[MaxPixelChannels];

        if ((k/(ssize_t) watermark->columns) >= (ssize_t) watermark->rows)
          break;
        (void) GetOneVirtualPixel(watermark,k % (ssize_t) watermark->columns,
          k/(ssize_t) watermark->columns,virtual_pixel,exception);
        pixel.red=virtual_pixel[RedPixelChannel];
        pixel.green=virtual_pixel[GreenPixelChannel];
        pixel.blue=virtual_pixel[BluePixelChannel];
        pixel.alpha=virtual_pixel[AlphaPixelChannel];
        q=GetAuthenticPixels(image,x,y,1,1,exception);
        if (q == (Quantum *) NULL)
          break;
        switch (c)
        {
          case 0:
          {
            SetBit(i,GetBit(pixel.red,j));
            break;
          }
          case 1:
          {
            SetBit(i,GetBit(pixel.green,j));
            break;
          }
          case 2:
          {
            SetBit(i,GetBit(pixel.blue,j));
            break;
          }
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
        c++;
        if (c == 3)
          c=0;
        k++;
        if (k == (ssize_t) (watermark->columns*watermark->columns))
          k=0;
        if (k == image->offset)
          j++;
      }
    }
    status=SetImageProgress(image,LoadImagesTag,(MagickOffsetType) i,depth);
    if (status == MagickFalse)
      break;
  }
  watermark=DestroyImage(watermark);
  (void) SyncImage(image,exception);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r S T E G A N O I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterSTEGANOImage() adds attributes for the STEGANO image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterSTEGANOImage method is:
%
%      size_t RegisterSTEGANOImage(void)
%
*/
ModuleExport size_t RegisterSTEGANOImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("STEGANO");
  entry->decoder=(DecodeImageHandler *) ReadSTEGANOImage;
  entry->format_type=ImplicitFormatType;
  entry->description=ConstantString("Steganographic image");
  entry->module=ConstantString("STEGANO");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S T E G A N O I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterSTEGANOImage() removes format registrations made by the
%  STEGANO module from the list of supported formats.
%
%  The format of the UnregisterSTEGANOImage method is:
%
%      UnregisterSTEGANOImage(void)
%
*/
ModuleExport void UnregisterSTEGANOImage(void)
{
  (void) UnregisterMagickInfo("STEGANO");
}
