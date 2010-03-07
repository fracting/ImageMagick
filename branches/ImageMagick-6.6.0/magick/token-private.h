/*
  Copyright 1999-2010 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.
  obtain a copy of the License at

    http://www.imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private token methods.
*/
#ifndef _MAGICKCORE_TOKEN_PRIVATE_H
#define _MAGICKCORE_TOKEN_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef EILSEQ
  #define EILSEQ  ENOENT
#endif

#define MaxMultibyteCodes  6

typedef struct
{
  long
    code_mask,
    code_value,
    utf_mask,
    utf_value;
} UTFInfo;

static UTFInfo
  utf_info[MaxMultibyteCodes] =
  {
    { 0x80, 0x00, 0x000007f, 0x0000000 },  /* 1 byte sequence */
    { 0xE0, 0xC0, 0x00007ff, 0x0000080 },  /* 2 byte sequence */
    { 0xF0, 0xE0, 0x000ffff, 0x0000800 },  /* 3 byte sequence */
    { 0xF8, 0xF0, 0x01fffff, 0x0010000 },  /* 4 byte sequence */
    { 0xFC, 0xF8, 0x03fffff, 0x0200000 },  /* 5 byte sequence */
    { 0xFE, 0xFC, 0x7ffffff, 0x4000000 },  /* 6 byte sequence */
  };

static inline long GetNextUTFCode(const char *text,size_t *octets)
{
  long
    code;

  register long
    i;

  register long
    c,
    unicode;

  *octets=0;
  if (text == (const char *) NULL)
    {
      errno=EINVAL;
      return(-1);
    }
  code=(long) (*text++) & 0xff;
  unicode=code;
  for (i=0; i < MaxMultibyteCodes; i++)
  {
    if ((code & utf_info[i].code_mask) == utf_info[i].code_value)
      {
        unicode&=utf_info[i].utf_mask;
        if (unicode < utf_info[i].utf_value)
          {
            errno=EILSEQ;
            return(-1);
          }
        *octets=(size_t) (i+1);
        return(unicode);
      }
    c=(long) (*text++ ^ 0x80) & 0xff;
    if ((c & 0xc0) != 0)
      {
        errno=EILSEQ;
        return(-1);
      }
    unicode=(unicode << 6) | c;
  }
  errno=EILSEQ;
  return(-1);
}

static long GetUTFCode(const char *text)
{
  size_t
    octets;

  return(GetNextUTFCode(text,&octets));
}

static size_t GetUTFOctets(const char *text)
{
  size_t
    octets;

  (void) GetNextUTFCode(text,&octets);
  return(octets);
}

static inline MagickBooleanType IsUTFSpace(long code)
{
  if (((code >= 0x0009) && (code <= 0x000d)) || (code == 0x0020) ||
      (code == 0x0085) || (code == 0x00a0) || (code == 0x1680) ||
      (code == 0x180e) || ((code >= 0x2000) && (code <= 0x200a)) ||
      (code == 0x2028) || (code == 0x2029) || (code == 0x202f) ||
      (code == 0x205f) || (code == 0x3000))
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsUTFValid(long code)
{
  long
    mask;

  mask=(long) 0x7fffffff;
  if (((code & ~mask) != 0) && ((code < 0xd800) || (code > 0xdfff)) &&
      (code != 0xfffe) && (code != 0xffff))
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsUTFAscii(long code)
{
  long
    mask;

  mask=(long) 0x7f;
  if ((code & ~mask) != 0)
    return(MagickFalse);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
