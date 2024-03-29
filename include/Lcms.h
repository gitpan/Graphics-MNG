//
//  Little cms
//  Copyright (C) 1998-2000 Marti Maria
//
// THIS SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
// EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// IN NO EVENT SHALL MARTI MARIA BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
// INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
// OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
// LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// Version 1.07m

#ifndef __cms_H

// ********** Configuration toggles ****************************************

//   Optimization mode.
//
// Note that USE_ASSEMBLER Is fastest by far, but it is limited to Pentium.
// USE_FLOAT are the generic floating-point routines. USE_C should work on
// virtually any machine.

// #define USE_FLOAT        1
// #define USE_C            1
#define USE_ASSEMBLER    1

// 3D interpolation method - Tetrahedral is slightly faster, but not always
// is proper. So, only trilinear assembler routine is provided. For now, is
// faster to use the trilinear (more accurated) assembler that the C-only
// Tetrahedral implementation. Also, tetrahedral algorithm seems it was
// covered by a Sakamoto's patent, now expired. Code is present here only
// for informational purposes.

#define USE_TRILINEAR       1
// #define USE_TETRAHEDRAL     1


// Define this if you are using this package as a DLL.
// The building of DLL is for now only available on
// Borland compilers, doesn't work on VC++, however VC++ can access the DLL.

// #define LCMS_DLL     1


// Uncomment if you are trying the engine in a non-windows environment
// like linux or SGI

// #define NON_WINDOWS  1


// Uncomment this one if you are using big endian machines
// Define if your processor stores words with the most significant
// byte first (like Motorola and SPARC, unlike Intel and VAX).
// (only meaningfull when NON_WINDOWS is used)

// #define USE_BIG_ENDIAN   1

// Uncomment this one if your compiler/machine does support the new
// "long long" type This will speedup fixed point math. (USE_C only)

// #define USE_INT64        1


// In conversion 16 -> 8 bps, it should be / 257, but doesn't matter
// anyway if I just >> 8, and the speed gain is big. This toggle forces
// the / 257 method, at a div per component cost. Don't touch unless
// you really know what are you doing!

// #define USE_257_FOR_16_TO_8_CONVERSION 1

// ********** End of configuration toggles ******************************

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


#ifdef NON_WINDOWS

// Non windows environments. Also avoid indentation on includes.

#undef LCMS_DLL

#ifdef  USE_ASSEMBLER
#  undef  USE_ASSEMBLER
#  define USE_C               1
#endif

#ifdef __sgi__
#   define USE_BIG_ENDIAN      1
#endif

#ifdef __sgi
#   define USE_BIG_ENDIAN      1
#endif

#ifdef macintosh
#   define USE_BIG_ENDIAN      1
#endif


#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#  include <sys/types.h>
#  define USE_INT64           1
#  undef  LONGLONG
#  define LONGLONG            u_int64_t
#endif


#include <memory.h>
#include <string.h>

typedef unsigned char BYTE, *LPBYTE;
typedef unsigned short WORD, *LPWORD;
typedef unsigned long DWORD, *LPDWORD; // DPM CHANGED THIS
typedef int BOOL;
typedef void *HANDLE, *LPVOID;
typedef char *LPSTR;

#define ZeroMemory(p,l)     memset((p),0,(l))
#define CopyMemory(d,s,l)   memcpy((d),(s),(l))
#define FAR
#define CONST const
#define FALSE 0
#define TRUE  1
#define LOWORD(l)    ((WORD)(l))
#define HIWORD(l)    ((WORD)((DWORD)(l) >> 16))
#define MAX_PATH     (256)
#define cdecl
#pragma pack(1)

#else

// Win32 stuff

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <pshpack1.h>

#ifdef  USE_INT64
#  ifndef LONGLONG
#    define LONGLONG __int64
#  endif
#endif

#endif


#include "icc34.h"          // ICC header file

// Plus some additions

#define lcmsSignature            ((icSignature)           0x6c636d73L)
#define icSigLuvKData            ((icColorSpaceSignature) 0x4C75764BL)  // 'LuvK'
#define icSigChromaticityTag     ((icTagSignature)        0x6368726dL)  // As per Addendum 2 to Spec. ICC.1:1998-09
#define icSigChromaticityType    ((icTagTypeSignature)    0x6368726dL)


#ifdef __cplusplus
extern "C" {
#endif

// Calling convention

#ifdef NON_WINDOWS
#  define LCMSEXPORT
#  define LCMSAPI
#else
# ifdef LCMS_DLL
#   ifdef __BORLANDC__
#      define LCMSEXPORT __stdcall _export
#      define LCMSAPI
#   else
       // VC++
#       define LCMSEXPORT  _stdcall
#       define LCMSAPI     __declspec(dllimport)
#   endif
# else
#       define LCMSEXPORT cdecl
#       define LCMSAPI
# endif
#endif


#ifdef __BORLANDC__

#      define ASM     asm
#      define RET(v)  return(v)
#else
      // VC++
#      define ASM     __asm
#      define RET(v)  return
#endif


// ********** Little cms API ***************************************************

typedef HANDLE cmsHPROFILE;        // Opaque typedefs to hide internals
typedef HANDLE cmsHTRANSFORM;

// Format of pixel is defined by one integer, using bits as follows
//
//            TTTTT -- P X S EEE CCCC BBB
//
//            T: Pixeltype
//            P: Planar? 0=Chunky, 1=Planar
//            X: swap 16 bps endianess?
//            S: Do swap? ie, BGR, KYMC
//            E: Extra samples
//            C: Channels (Samples per pixel)
//            B: Bytes per sample
//            -: Unused (reserved)


#define COLORSPACE_SH(s)       ((s) << 16)
#define PLANAR_SH(p)           ((p) << 12)
#define ENDIAN16_SH(e)         ((e) << 11)
#define DOSWAP_SH(e)           ((e) << 10)
#define EXTRA_SH(e)            ((e) << 7)
#define CHANNELS_SH(c)         ((c) << 3)
#define BYTES_SH(b)            (b)

// Pixel types

#define PT_ANY       0    // Don't check colorspace

                           // 1 & 2 are reserved
#define PT_GRAY      3
#define PT_RGB       4
#define PT_CMY       5
#define PT_CMYK      6
#define PT_YCbCr     7
#define PT_YUV       8      // Lu'v'
#define PT_XYZ       9
#define PT_Lab       10
#define PT_YUVK      11     // Lu'v'K
#define PT_HSV       12
#define PT_HLS       13
#define PT_Yxy       14


#define NOCOLORSPACECHECK(x)    ((x) & 0xFFFF)

// Some (not all!) representations

#ifndef TYPE_RGB_8

#define TYPE_GRAY_8            (COLORSPACE_SH(PT_GRAY)|CHANNELS_SH(1)|BYTES_SH(1))
#define TYPE_GRAY_16           (COLORSPACE_SH(PT_GRAY)|CHANNELS_SH(1)|BYTES_SH(2))
#define TYPE_GRAY_16_SE        (COLORSPACE_SH(PT_GRAY)|CHANNELS_SH(1)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_GRAYA_8           (COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(1))
#define TYPE_GRAYA_16          (COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(2))
#define TYPE_GRAYA_16_SE       (COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_GRAYA_8_PLANAR    (COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_GRAYA_16_PLANAR   (COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(2)|PLANAR_SH(1))

#define TYPE_RGB_8             (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_RGB_8_PLANAR      (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_BGR_8             (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_BGR_8_PLANAR      (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|PLANAR_SH(1))
#define TYPE_RGB_16            (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_RGB_16_PLANAR     (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_RGB_16_SE         (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_BGR_16            (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_BGR_16_PLANAR     (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1)|PLANAR_SH(1))
#define TYPE_BGR_16_SE         (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))

#define TYPE_RGBA_8            (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_RGBA_8_PLANAR     (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_RGBA_16           (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_RGBA_16_PLANAR    (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_RGBA_16_SE        (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_ABGR_8            (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_ABGR_16           (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_ABGR_16_PLANAR    (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1)|PLANAR_SH(1))
#define TYPE_ABGR_16_SE        (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))

#define TYPE_CMY_8             (COLORSPACE_SH(PT_CMY)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_CMY_8_PLANAR      (COLORSPACE_SH(PT_CMY)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_CMY_16            (COLORSPACE_SH(PT_CMY)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_CMY_16_PLANAR     (COLORSPACE_SH(PT_CMY)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_CMY_16_SE         (COLORSPACE_SH(PT_CMY)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))

#define TYPE_CMYK_8            (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(1))
#define TYPE_CMYK_8_PLANAR     (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_CMYK_16           (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(2))
#define TYPE_CMYK_16_PLANAR    (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_CMYK_16_SE        (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(2)|ENDIAN16_SH(1))

#define TYPE_KYMC_8            (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC_16           (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC_16_SE        (COLORSPACE_SH(PT_CMYK)|CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))

// HiFi separations, Thanks to Steven Greaves for providing the code,
// the colorspace is not checked

#define TYPE_CMYKcm_8          (CHANNELS_SH(6)|BYTES_SH(1))
#define TYPE_CMYKcm_8_PLANAR   (CHANNELS_SH(6)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_CMYKcm_16         (CHANNELS_SH(6)|BYTES_SH(2))
#define TYPE_CMYKcm_16_PLANAR  (CHANNELS_SH(6)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_CMYKcm_16_SE      (CHANNELS_SH(6)|BYTES_SH(2)|ENDIAN16_SH(1))

// Separations with more than 6 channels aren't very standarized,
// Except most start with CMYK and add other colors, so I just used
// then total number of channels after CMYK i.e CMYK8_8

#define TYPE_CMYK7_8           (CHANNELS_SH(7)|BYTES_SH(1))
#define TYPE_CMYK7_16          (CHANNELS_SH(7)|BYTES_SH(2))
#define TYPE_CMYK7_16_SE       (CHANNELS_SH(7)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_KYMC7_8           (CHANNELS_SH(7)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC7_16          (CHANNELS_SH(7)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC7_16_SE       (CHANNELS_SH(7)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))
#define TYPE_CMYK8_8           (CHANNELS_SH(8)|BYTES_SH(1))
#define TYPE_CMYK8_16          (CHANNELS_SH(8)|BYTES_SH(2))
#define TYPE_CMYK8_16_SE       (CHANNELS_SH(8)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_KYMC8_8           (CHANNELS_SH(8)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC8_16          (CHANNELS_SH(8)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC8_16_SE       (CHANNELS_SH(8)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))
#define TYPE_CMYK9_8           (CHANNELS_SH(9)|BYTES_SH(1))
#define TYPE_CMYK9_16          (CHANNELS_SH(9)|BYTES_SH(2))
#define TYPE_CMYK9_16_SE       (CHANNELS_SH(9)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_KYMC9_8           (CHANNELS_SH(9)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC9_16          (CHANNELS_SH(9)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC9_16_SE       (CHANNELS_SH(9)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))
#define TYPE_CMYK10_8          (CHANNELS_SH(10)|BYTES_SH(1))
#define TYPE_CMYK10_16         (CHANNELS_SH(10)|BYTES_SH(2))
#define TYPE_CMYK10_16_SE      (CHANNELS_SH(10)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_KYMC10_8          (CHANNELS_SH(10)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC10_16         (CHANNELS_SH(10)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC10_16_SE      (CHANNELS_SH(10)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))
#define TYPE_CMYK11_8          (CHANNELS_SH(11)|BYTES_SH(1))
#define TYPE_CMYK11_16         (CHANNELS_SH(11)|BYTES_SH(2))
#define TYPE_CMYK11_16_SE      (CHANNELS_SH(11)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_KYMC11_8          (CHANNELS_SH(11)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC11_16         (CHANNELS_SH(11)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC11_16_SE      (CHANNELS_SH(11)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))
#define TYPE_CMYK12_8          (CHANNELS_SH(12)|BYTES_SH(1))
#define TYPE_CMYK12_16         (CHANNELS_SH(12)|BYTES_SH(2))
#define TYPE_CMYK12_16_SE      (CHANNELS_SH(12)|BYTES_SH(2)|ENDIAN16_SH(1))
#define TYPE_KYMC12_8          (CHANNELS_SH(12)|BYTES_SH(1)|DOSWAP_SH(1))
#define TYPE_KYMC12_16         (CHANNELS_SH(12)|BYTES_SH(2)|DOSWAP_SH(1))
#define TYPE_KYMC12_16_SE      (CHANNELS_SH(12)|BYTES_SH(2)|DOSWAP_SH(1)|ENDIAN16_SH(1))

// Colorimetric

#define TYPE_XYZ_16            (COLORSPACE_SH(PT_XYZ)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_Lab_8             (COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_Lab_16            (COLORSPACE_SH(PT_Lab)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_Yxy_16            (COLORSPACE_SH(PT_Yxy)|CHANNELS_SH(3)|BYTES_SH(2))


// YCbCr

#define TYPE_YCbCr_8           (COLORSPACE_SH(PT_YCbCr)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_YCbCr_8_PLANAR    (COLORSPACE_SH(PT_YCbCr)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_YCbCr_16          (COLORSPACE_SH(PT_YCbCr)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_YCbCr_16_PLANAR   (COLORSPACE_SH(PT_YCbCr)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_YCbCr_16_SE       (COLORSPACE_SH(PT_YCbCr)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))

// YUV

#define TYPE_YUV_8           (COLORSPACE_SH(PT_YUV)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_YUV_8_PLANAR    (COLORSPACE_SH(PT_YUV)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_YUV_16          (COLORSPACE_SH(PT_YUV)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_YUV_16_PLANAR   (COLORSPACE_SH(PT_YUV)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_YUV_16_SE       (COLORSPACE_SH(PT_YUV)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))

// HLS

#define TYPE_HLS_8           (COLORSPACE_SH(PT_HLS)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_HLS_8_PLANAR    (COLORSPACE_SH(PT_HLS)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_HLS_16          (COLORSPACE_SH(PT_HLS)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_HLS_16_PLANAR   (COLORSPACE_SH(PT_HLS)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_HLS_16_SE       (COLORSPACE_SH(PT_HLS)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))


// HSV

#define TYPE_HSV_8           (COLORSPACE_SH(PT_HSV)|CHANNELS_SH(3)|BYTES_SH(1))
#define TYPE_HSV_8_PLANAR    (COLORSPACE_SH(PT_HSV)|CHANNELS_SH(3)|BYTES_SH(1)|PLANAR_SH(1))
#define TYPE_HSV_16          (COLORSPACE_SH(PT_HSV)|CHANNELS_SH(3)|BYTES_SH(2))
#define TYPE_HSV_16_PLANAR   (COLORSPACE_SH(PT_HSV)|CHANNELS_SH(3)|BYTES_SH(2)|PLANAR_SH(1))
#define TYPE_HSV_16_SE       (COLORSPACE_SH(PT_HSV)|CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1))


#endif

// Gamma tables.

typedef struct {
              int  nEntries;
              WORD GammaTable[1];

              } GAMMATABLE, FAR* LPGAMMATABLE;

// Vectors

typedef struct {                // Float Vector
        double n[3];
        } VEC3, FAR* LPVEC3;


typedef struct {                // Matrix
        VEC3 v[3];
        } MAT3, FAR* LPMAT3;


// CIE values

typedef struct {
               double X;
               double Y;
               double Z;

               } cmsCIEXYZ, FAR* LPcmsCIEXYZ;

typedef struct {
               double x;
               double y;
               double Y;

               } cmsCIExyY, FAR* LPcmsCIExyY;

// Primaries

typedef struct {

              cmsCIEXYZ  Red;
              cmsCIEXYZ  Green;
              cmsCIEXYZ  Blue;

              } cmsCIEXYZTRIPLE, FAR* LPcmsCIEXYZTRIPLE;


typedef struct {

              cmsCIExyY  Red;
              cmsCIExyY  Green;
              cmsCIExyY  Blue;

              } cmsCIExyYTRIPLE, FAR* LPcmsCIExyYTRIPLE;




// Input/Output

LCMSAPI cmsHPROFILE   LCMSEXPORT cmsOpenProfileFromFile(const char *ICCProfile, const char *sAccess);
LCMSAPI cmsHPROFILE   LCMSEXPORT cmsOpenProfileFromMem(LPVOID MemPtr, DWORD dwSize);

LCMSAPI cmsHPROFILE   LCMSEXPORT cmsCreateRGBProfile(LPcmsCIExyY WhitePoint,
                                        LPcmsCIExyYTRIPLE Primaries,
                                        LPGAMMATABLE TransferFunction[3]);

LCMSAPI BOOL          LCMSEXPORT cmsCloseProfile(cmsHPROFILE hProfile);

// Utils

LCMSAPI void          LCMSEXPORT cmsXYZ2xyY(LPcmsCIExyY Dest, const LPcmsCIEXYZ Source);
LCMSAPI void          LCMSEXPORT cmsxyY2XYZ(LPcmsCIEXYZ Dest, const LPcmsCIExyY Source);
LCMSAPI BOOL          LCMSEXPORT cmsWhitePointFromTemp(int TempK, LPcmsCIExyY WhitePoint);
LCMSAPI BOOL          LCMSEXPORT cmsAdaptToIlluminant(LPcmsCIEXYZ Result,
                                                        LPcmsCIExyY SourceWhitePt,
                                                        LPcmsCIExyY Illuminant,
                                                        LPcmsCIEXYZ Value);

LCMSAPI BOOL          LCMSEXPORT cmsBuildRGB2XYZtransferMatrix(LPMAT3 r,
                                                        LPcmsCIExyY WhitePoint,
                                                        LPcmsCIExyYTRIPLE Primaries);

// Gamma

LCMSAPI LPGAMMATABLE  LCMSEXPORT cmsBuildGamma(int nEntries, double Gamma);
LCMSAPI LPGAMMATABLE  LCMSEXPORT cmsAllocGamma(int nEntries);
LCMSAPI void          LCMSEXPORT cmsFreeGamma(LPGAMMATABLE Gamma);
LCMSAPI LPGAMMATABLE  LCMSEXPORT cmsReverseGamma(int nResultSamples, LPGAMMATABLE InGamma);
LCMSAPI LPGAMMATABLE  LCMSEXPORT cmsJoinGamma(LPGAMMATABLE InGamma,  LPGAMMATABLE OutGamma);


// Access to Profile data.

LCMSAPI BOOL          LCMSEXPORT cmsTakeMediaWhitePoint(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile);
LCMSAPI BOOL          LCMSEXPORT cmsTakeMediaBlackPoint(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile);
LCMSAPI BOOL          LCMSEXPORT cmsTakeIluminant(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile);
LCMSAPI BOOL          LCMSEXPORT cmsTakeColorants(LPcmsCIEXYZTRIPLE Dest, cmsHPROFILE hProfile);
LCMSAPI const char*   LCMSEXPORT cmsTakeProductName(cmsHPROFILE hProfile);
LCMSAPI const char*   LCMSEXPORT cmsTakeProductDesc(cmsHPROFILE hProfile);
LCMSAPI const char*   LCMSEXPORT cmsTakeProductInfo(cmsHPROFILE hProfile);
LCMSAPI BOOL          LCMSEXPORT cmsIsTag(cmsHPROFILE hProfile, icTagSignature sig);
LCMSAPI int           LCMSEXPORT cmsTakeRenderingIntent(cmsHPROFILE hProfile);

#define LCMS_USED_AS_INPUT      0
#define LCMS_USED_AS_OUTPUT     1
#define LCMS_USED_AS_PROOF      2

LCMSAPI BOOL          LCMSEXPORT cmsIsIntentSupported(cmsHPROFILE hProfile, int Intent, int UsedDirection);

LCMSAPI icColorSpaceSignature   LCMSEXPORT cmsGetPCS(cmsHPROFILE hProfile);
LCMSAPI icColorSpaceSignature   LCMSEXPORT cmsGetColorSpace(cmsHPROFILE hProfile);
LCMSAPI icProfileClassSignature LCMSEXPORT cmsGetDeviceClass(cmsHPROFILE hProfile);


// Intents

#define INTENT_PERCEPTUAL                 0
#define INTENT_RELATIVE_COLORIMETRIC      1
#define INTENT_SATURATION                 2
#define INTENT_ABSOLUTE_COLORIMETRIC      3

// Flags

#define cmsFLAGS_MATRIXINPUT       0x0001
#define cmsFLAGS_MATRIXOUTPUT      0x0002
#define cmsFLAGS_MATRIXONLY        (cmsFLAGS_MATRIXINPUT|cmsFLAGS_MATRIXOUTPUT)
#define cmsFLAGS_NOTPRECALC        0x0100
#define cmsFLAGS_NULLTRANSFORM     0x0200        // Don't transform anyway
#define cmsFLAGS_GAMUTCHECK        0x1000        // Out of Gamut alarm color

// Matches black and white on precalculated transforms

#define cmsFLAGS_WHITEBLACKCOMPENSATION 0x2000


LCMSAPI cmsHTRANSFORM LCMSEXPORT cmsCreateTransform(cmsHPROFILE Input,
                                               DWORD InputFormat,
                                               cmsHPROFILE Output,
                                               DWORD OutputFormat,
                                               int Intent,
                                               DWORD dwFlags);

LCMSAPI cmsHTRANSFORM LCMSEXPORT cmsCreateProofingTransform(cmsHPROFILE Input,
                                               DWORD InputFormat,
                                               cmsHPROFILE Output,
                                               DWORD OutputFormat,
                                               cmsHPROFILE Proofing,
                                               int Intent,
                                               int ProofingIntent,
                                               DWORD dwFlags);

LCMSAPI void         LCMSEXPORT cmsDeleteTransform(cmsHTRANSFORM hTransform);

LCMSAPI void         LCMSEXPORT cmsDoTransform(cmsHTRANSFORM Transform,
                                                 LPVOID InputBuffer,
                                                 LPVOID OutputBuffer,
                                                 unsigned int Size);


LCMSAPI void         LCMSEXPORT cmsSetAlarmCodes(int r, int g, int b);
LCMSAPI void         LCMSEXPORT cmsGetAlarmCodes(int *r, int *g, int *b);

// Error handling

#define LCMS_ERROR_ABORT    0
#define LCMS_ERROR_SHOW     1
#define LCMS_ERROR_IGNORE   2

LCMSAPI void LCMSEXPORT cmsErrorAction(int nAction);

#define LCMS_ERRC_WARNING        0x1000
#define LCMS_ERRC_RECOVERABLE    0x2000
#define LCMS_ERRC_ABORTED        0x3000

void cdecl cmsSignalError(int ErrorCode, const char *ErrorText, ...);

// ***************************************************************************
// End of Little cms API From here functions are private
// You can use them only if using static libraries, and at your own risk of
// be stripped or changed at futures releases.


// Alignment handling (needed in ReadLUT16 and ReadLUT8)

typedef struct {
        icS15Fixed16Number a;
        icUInt16Number     b;

       } _cmsTestAlign16;

#define SIZEOF_UINT16_ALIGNED (sizeof(_cmsTestAlign16) - sizeof(icS15Fixed16Number))

typedef struct {
        icS15Fixed16Number a;
        icUInt8Number      b;

       } _cmsTestAlign8;

#define SIZEOF_UINT8_ALIGNED (sizeof(_cmsTestAlign8) - sizeof(icS15Fixed16Number))


// Fixed point

typedef long Fixed32;       // Fixed 15.16 whith sign


#define INT_TO_FIXED(x)         ((x)<<16)
#define DOUBLE_TO_FIXED(x)      ((long)((x)*65536.0+.5))
#define FIXED_TO_INT(x)         ((x)>>16)
#define FIXED_REST_TO_INT(x)    ((x)&0xFFFFU)
#define FIXED_TO_DOUBLE(x)      (((double)x)/65536.0)
#define ROUND_FIXED_TO_INT(x)   (((x)+0x8000)>>16)


Fixed32 cdecl FixedMul(Fixed32 a, Fixed32 b);
Fixed32 cdecl FixedDiv(Fixed32 a, Fixed32 b);
Fixed32 cdecl ToFixedDomain(int a);              // (a * 65536.0 / 65535.0)
int     cdecl FromFixedDomain(Fixed32 a);        // (a * 65535.0 + .5)
Fixed32 cdecl FixedLERP(Fixed32 a, Fixed32 l, Fixed32 h);
WORD    cdecl FixedScale(WORD a, Fixed32 s);

// Vector & Matrix operations. I'm using the notation frequently found in
// literature. Mostly 'Graphic Gems' samples. Not to be same routines.

// Vector members

#define VX      0
#define VY      1
#define VZ      2

typedef struct {                // Fixed 15.16 bits vector
        Fixed32 n[3];
        } WVEC3, FAR* LPWVEC3;

typedef struct {                // Matrix (Fixed 15.16)
        WVEC3 v[3];
        } WMAT3, FAR* LPWMAT3;



void cdecl VEC3init(LPVEC3 r, double x, double y, double z);   // double version
void cdecl VEC3initF(LPWVEC3 r, double x, double y, double z); // Fix32 version
void cdecl VEC3toFix(LPWVEC3 r, LPVEC3 v);
void cdecl VEC3scaleFix(LPWORD r, LPWVEC3 Scale);
void cdecl VEC3swap(LPVEC3 a, LPVEC3 b);
void cdecl VEC3divK(LPVEC3 r, LPVEC3 v, double d);
void cdecl VEC3perK(LPVEC3 r, LPVEC3 v, double d);
void cdecl VEC3minus(LPVEC3 r, LPVEC3 a, LPVEC3 b);
void cdecl VEC3perComp(LPVEC3 r, LPVEC3 a, LPVEC3 b);
BOOL cdecl VEC3equal(LPWVEC3 a, LPWVEC3 b, double Tolerance);

void cdecl MAT3identity(LPMAT3 a);
void cdecl MAT3per(LPMAT3 r, LPMAT3 a, LPMAT3 b);
void cdecl MAT3perK(LPMAT3 r, LPMAT3 v, double d);
int  cdecl MAT3inverse(LPMAT3 a, LPMAT3 b);
void cdecl MAT3eval(LPVEC3 r, LPMAT3 a, LPVEC3 v);
void cdecl MAT3toFix(LPWMAT3 r, LPMAT3 v);
void cdecl MAT3evalW(LPWVEC3 r, LPWMAT3 a, LPWVEC3 v);
BOOL cdecl MAT3isIdentity(LPWMAT3 a, double Tolerance);


// Is a table linear?

int  cdecl cmsIsLinear(WORD Table[], int nEntries);

// I hold this structures describing domain
// details mainly for optimization purposes.


typedef struct {              // Used on 16 bits interpolations

               int nSamples;       // Valid on all kinds of tables
               int nInputs;        // != 1 only in 3D interpolation
               int nOutputs;       // != 1 only in 3D interpolation

               WORD Domain;

               int a1, a2, a3, a4;     // Optimization for 3D LUT


               } L16PARAMS, *LPL16PARAMS;


void    cdecl cmsCalcL16Params(int nSamples, LPL16PARAMS p);
void    cdecl cmsCalcCLUT16Params(int nSamples, int InputChan, int OutputChan, LPL16PARAMS p);

WORD    cdecl cmsLinearInterpLUT16(WORD Value, WORD LutTable[], LPL16PARAMS p);

Fixed32 cdecl cmsLinearInterpFixed(WORD Value1, WORD LutTable[], LPL16PARAMS p);

WORD    cdecl cmsReverseLinearInterpLUT16(WORD Value,
                                       WORD LutTable[], LPL16PARAMS p);

void cdecl cmsTrilinearInterp16(WORD Input[],
                                WORD Output[],
                                WORD LutTable[],
                                LPL16PARAMS p);

void cdecl cmsTetrahedralInterp16(WORD Input[],
                                  WORD Output[],
                                  WORD LutTable[], LPL16PARAMS p);



// LUT handling

#define LUT_HASMATRIX       0x0001        // Do-op Flags
#define LUT_HASTL1          0x0002
#define LUT_HASTL2          0x0008
#define LUT_HAS3DGRID       0x0010

#define MAXCHANNELS  16            // Maximum number of channels

typedef struct {

               DWORD wFlags;
               WMAT3 Matrix;                    // 15fixed16 matrix

               unsigned int InputChan;
               unsigned int OutputChan;
               unsigned int InputEntries;
               unsigned int OutputEntries;
               unsigned int cLutPoints;

               // I'm encoding linear tables as
               // WORD (0..ffff) values

               LPWORD L1[MAXCHANNELS];          // First linearization
               LPWORD L2[MAXCHANNELS];          // Last linearization

               LPWORD T;                        // 3D CLUT

              // Parameters & Optimizations

               L16PARAMS In16params;
               L16PARAMS Out16params;
               L16PARAMS CLut16params;

               int Intent;                       // Accomplished intent

               } LUT, FAR* LPLUT;


// LUT handling

LPLUT cdecl cmsAllocLUT(void);
LPLUT cdecl cmsAllocLinearTable(LPLUT NewLUT, LPGAMMATABLE Tables[], int nTable);
LPLUT cdecl cmsAlloc3DGrid(LPLUT Lut, int clutPoints, int inputChan, int outputChan);
void  cdecl cmsFreeLUT(LPLUT Lut);
void  cdecl cmsEvalLUT(LPLUT Lut, WORD In[], WORD Out[]);
LPLUT cdecl cmsReadICCLut(cmsHPROFILE hProfile, icTagSignature sig);


// Gamma

LPGAMMATABLE cdecl cmsScaleGamma(LPGAMMATABLE Input, Fixed32 Factor);


// Shaper/Matrix handling

#define MATSHAPER_HASMATRIX        0x0001        // Do-ops flags
#define MATSHAPER_HASSHAPER        0x0002
#define MATSHAPER_INPUT            0x0004        // Behaviour
#define MATSHAPER_OUTPUT           0x0008
#define MATSHAPER_ALLSMELTED       (MATSHAPER_INPUT|MATSHAPER_OUTPUT)


typedef struct {
               DWORD dwFlags;

               WMAT3 Matrix;
               L16PARAMS p16;

               LPWORD L[3];

               } MATSHAPER, FAR* LPMATSHAPER;

LPMATSHAPER cdecl cmsAllocMatShaper(LPMAT3 matrix, LPGAMMATABLE Shaper[], DWORD Behaviour);
LPMATSHAPER cdecl cmsAllocMonoMatShaper(LPGAMMATABLE Tables[], DWORD Behaviour);
void        cdecl cmsFreeMatShaper(LPMATSHAPER MatShaper);
void        cdecl cmsEvalMatShaper(LPMATSHAPER MatShaper, WORD In[], WORD Out[]);

LPGAMMATABLE cdecl cmsReadICCGamma(cmsHPROFILE hProfile, icTagSignature sig);
BOOL         cdecl cmsReadICCMatrixRGB2XYZ(LPMAT3 r, cmsHPROFILE hProfile);

// White Point & Primary chromas handling

BOOL cdecl cmsAdaptMatrixToD50(LPMAT3 r, LPcmsCIExyY SourceWhitePt);
BOOL cdecl cmsAdaptMatrixFromD50(LPMAT3 r, LPcmsCIExyY DestWhitePt);

// Inter-PCS conversion routines. They assume D50 as white point.

void cdecl cmsXYZ2LabEncoded(WORD XYZ[3], WORD Lab[3]);
void cdecl cmsLab2XYZEncoded(WORD Lab[3], WORD XYZ[3]);


// These macros unpack format specifiers into integers

#define T_COLORSPACE(s)       (((s)>>16)&31)
#define T_PLANAR(p)           (((p)>>12)&1)
#define T_ENDIAN16(e)         (((e)>>11)&1)
#define T_DOSWAP(e)           (((e)>>10)&1)
#define T_EXTRA(e)            (((e)>>7)&7)
#define T_CHANNELS(c)         (((c)>>3)&15)
#define T_BYTES(b)            ((b)&7)


// Translate form our notation to ICC one.

LCMSAPI icColorSpaceSignature LCMSEXPORT _cmsICCcolorSpace(int OurNotation);

// Packing & Unpacking

typedef struct {

              DWORD InputFormat, OutputFormat;          // Formats
              DWORD StrideIn, StrideOut;                // Planar support

              } _cmsFIXINFO, FAR *LP_cmsFIXINFO;


typedef LPBYTE (* _cmsFIXFN)(register LP_cmsFIXINFO info,
                             register WORD ToUnroll[],
                             register LPBYTE Buffer);

_cmsFIXFN cdecl _cmsIdentifyInputFormat(DWORD dwInput);
_cmsFIXFN cdecl _cmsIdentifyOutputFormat(DWORD dwOutput);

// Conversion

#define XYZRel       0
#define LabRel       1
#define XYZAbs       2
#define LabAbs       3


typedef void (* _cmsADJFN)(WORD In[], WORD Out[], LPWVEC3 a, LPWVEC3 b);

int cdecl cmsChooseCnvrt(int Absolute,
                 int Phase1, LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,

                 _cmsADJFN *fn1,
                 LPWVEC3 wa, LPWVEC3 wb);



// Clamping & Gamut handling

BOOL cdecl   _cmsEndPointsBySpace(icColorSpaceSignature Space,
                            WORD **White, WORD **Black, int *nOutputs);

WORD * cdecl _cmsWhiteBySpace(icColorSpaceSignature Space);

WORD cdecl Clamp_XYZ(int in);
WORD cdecl Clamp_RGB(int in);

WORD cdecl Clamp_L(Fixed32 in);
WORD cdecl Clamp_ab(Fixed32 in);


// These are two VITAL macros, from converting between 8 and 16 bit
// representation. For commodity, I'm always TRUNCATING values. Rounding
// has bad side effects, as a 0xFFFF is converted to 0x10000 and a '00' is
// returned.

#define RGB_8_TO_16(rgb) (WORD) ((((WORD) (rgb)) << 8)|(rgb)) // * 257

#ifdef USE_257_FOR_16_TO_8_CONVERSION
#   define RGB_16_TO_8(rgb) (BYTE) (((WORD) (rgb)) / 257)
#else
#   define RGB_16_TO_8(rgb) (BYTE) (((WORD) (rgb)) >> 8)
#endif


#define __cms_H

#ifdef __cplusplus
}
#endif

#ifdef NON_WINDOWS
#pragma pack()
#else
#include <poppack.h>
#endif
#endif

