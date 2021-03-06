//------------------------------------------------------------------------------------//
//                                                                                    //
//               _________        .__  __    __                                       //
//               \_   ___ \_______|__|/  |__/  |_  ___________                        //
//               /    \  \/\_  __ \  \   __\   __\/ __ \_  __ \                       //
//               \     \____|  | \/  ||  |  |  | \  ___/|  | \/                       //
//                \______  /|__|  |__||__|  |__|  \___  >__|                          //
//                       \/                           \/                              //
//                                                                                    //
//    Critter is provided under the MIT License(MIT)                                  //
//    Critter uses portions of other open source software.                            //
//    Please review the LICENSE file for further details.                             //
//                                                                                    //
//    Copyright(c) 2015 Matt Davidson                                                 //
//                                                                                    //
//    Permission is hereby granted, free of charge, to any person obtaining a copy    //
//    of this software and associated documentation files(the "Software"), to deal    //
//    in the Software without restriction, including without limitation the rights    //
//    to use, copy, modify, merge, publish, distribute, sublicense, and / or sell     //
//    copies of the Software, and to permit persons to whom the Software is           //
//    furnished to do so, subject to the following conditions :                       //
//                                                                                    //
//    1. Redistributions of source code must retain the above copyright notice,       //
//    this list of conditions and the following disclaimer.                           //
//    2. Redistributions in binary form must reproduce the above copyright notice,    //
//    this list of conditions and the following disclaimer in the                     //
//    documentation and / or other materials provided with the distribution.          //
//    3. Neither the name of the copyright holder nor the names of its                //
//    contributors may be used to endorse or promote products derived                 //
//    from this software without specific prior written permission.                   //
//                                                                                    //
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      //
//    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        //
//    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE      //
//    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          //
//    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   //
//    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN       //
//    THE SOFTWARE.                                                                   //
//                                                                                    //
//------------------------------------------------------------------------------------//
/* Based on code from the OGRE engine:
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2012 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include <CtrFreeImageCodec.h>
#include <CtrTextureImage.h>
#include <CtrStringUtilities.h>
#include <CtrLog.h>

#if IBL_USE_ASS_IMP_AND_FREEIMAGE


#define FREEIMAGE_LIB 
#include "../dependencies/FreeImage/source/FreeImage.h"

// freeimage 3.9.1~3.11.0 interoperability fix
#ifndef FREEIMAGE_COLORORDER
// we have freeimage 3.9.1, define these symbols in such way as 3.9.1 really work (do not use 3.11.0 definition, as color order was changed between these two versions on Apple systems)
#define FREEIMAGE_COLORORDER_BGR    0
#define FREEIMAGE_COLORORDER_RGB    1
#if defined(FREEIMAGE_BIGENDIAN)
#define FREEIMAGE_COLORORDER FREEIMAGE_COLORORDER_RGB
#else
#define FREEIMAGE_COLORORDER FREEIMAGE_COLORORDER_BGR
#endif
#endif

namespace Ctr 
{
    FreeImageCodec::RegisteredCodecList FreeImageCodec::msCodecList;
    //---------------------------------------------------------------------
    void FreeImageLoadErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) 
    {
        // Callback method as required by FreeImage to report problems
        const char* typeName = FreeImage_GetFormatFromFIF(fif);
        if (typeName)
        {
            LOG("FreeImage error: '" << message << "' when loading format " << typeName);
        }
        else
        {
            LOG("FreeImage error: '" << message << "'");
        }

    }
    //---------------------------------------------------------------------
    void FreeImageSaveErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) 
    {
        // Callback method as required by FreeImage to report problems
        throw(std::exception(message));
    }
    //---------------------------------------------------------------------
    void FreeImageCodec::startup(void)
    {
        FreeImage_Initialise(false);

        LOG("FreeImage version: " + std::string(FreeImage_GetVersion()));
        LOG(FreeImage_GetCopyrightMessage());

        // Register codecs
        std::ostringstream strExt;
        strExt << "Supported formats: ";
        bool first = true;
        for (int i = 0; i < FreeImage_GetFIFCount(); ++i)
        {

            // Skip DDS codec since FreeImage does not have the option 
            // to keep DXT data compressed, we'll use our own codec
            if ((FREE_IMAGE_FORMAT)i == FIF_DDS)
                continue;
            
            std::string exts(FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i));
            if (!first)
            {
                strExt << ",";
            }
            first = false;
            strExt << exts;
            
            // Pull off individual formats (separated by comma by FI)
            StringVector extsVector = StringUtil::split(exts, ",");
            for (auto v = extsVector.begin(); v != extsVector.end(); ++v)
            {
                // FreeImage 3.13 lists many formats twice: once under their own codec and
                // once under the "RAW" codec, which is listed last. Avoid letting the RAW override
                // the dedicated codec!
                if (!Codec::isCodecRegistered(*v))
                {
                    ImageCodec* codec = new FreeImageCodec(*v, i);
                    msCodecList.push_back(codec);
                    Codec::registerCodec(codec);
                }
            }
        }
        LOG(strExt.str());

        // Set error handler
        FreeImage_SetOutputMessage(FreeImageLoadErrorHandler);




    }
    //---------------------------------------------------------------------
    void FreeImageCodec::shutdown(void)
    {
        FreeImage_DeInitialise();

        for (auto i = msCodecList.begin(); i != msCodecList.end(); ++i)
        {
            Codec::unRegisterCodec(*i);
            delete *i;
        }
        msCodecList.clear();

    }
    //---------------------------------------------------------------------
    FreeImageCodec::FreeImageCodec(const std::string &type, unsigned int fiType):
        mType(type),
        mFreeImageType(fiType)
    { 
    }
    //---------------------------------------------------------------------
    FIBITMAP* FreeImageCodec::encode(MemoryDataStreamPtr& input, CodecDataPtr& pData) const
    {
        FIBITMAP* ret = 0;

        ImageData* pImgData = static_cast< ImageData * >(pData.get());
        PixelBox src(pImgData->width, pImgData->height, pImgData->depth, pImgData->format, input->getPtr());

        // The required format, which will adjust to the format
        // actually supported by FreeImage.
        PixelFormat requiredFormat = pImgData->format;

        // determine the settings
        FREE_IMAGE_TYPE imageType;
        PixelFormat determiningFormat = pImgData->format;

        switch (determiningFormat)
        {
        case PF_R5G6B5:
        case PF_B5G6R5:
        case PF_R8G8B8:
        case PF_B8G8R8:
        case PF_A8R8G8B8:
        case PF_X8R8G8B8:
        case PF_A8B8G8R8:
        case PF_X8B8G8R8:
        case PF_B8G8R8A8:
        case PF_R8G8B8A8:
        case PF_A4L4:
        case PF_BYTE_LA:
        case PF_R3G3B2:
        case PF_A4R4G4B4:
        case PF_A1R5G5B5:
        case PF_A2R10G10B10:
        case PF_A2B10G10R10:
            // I'd like to be able to use r/g/b masks to get FreeImage to load the data
            // in it's existing format, but that doesn't work, FreeImage needs to have
            // data in RGB[A] (big endian) and BGR[A] (little endian), always.
            //            if (PixelUtil::hasAlpha(determiningFormat))
        {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
            requiredFormat = PF_BYTE_RGBA;
#else
            requiredFormat = PF_BYTE_BGRA;
            imageType = FIT_BITMAP;
#endif
        }
        break;
        // fall through
        case PF_L8:
        case PF_A8:
            imageType = FIT_BITMAP;
            break;

        case PF_L16:
            imageType = FIT_UINT16;
            break;

        case PF_SHORT_GR:
            requiredFormat = PF_SHORT_RGB;
            // fall through
        case PF_SHORT_RGB:
            imageType = FIT_RGB16;
            break;

        case PF_SHORT_RGBA:
            imageType = FIT_RGBA16;
            break;

        case PF_FLOAT16_R:
            requiredFormat = PF_FLOAT32_R;
            // fall through
        case PF_FLOAT32_R:
            imageType = FIT_FLOAT;
            break;

        case PF_FLOAT16_GR:
        case PF_FLOAT16_RGB:
        case PF_FLOAT32_GR:
            requiredFormat = PF_FLOAT32_RGB;
            // fall through
        case PF_FLOAT32_RGB:
            imageType = FIT_RGBF;
            break;

        case PF_FLOAT16_RGBA:
            requiredFormat = PF_FLOAT32_RGBA;
            // fall through
        case PF_FLOAT32_RGBA:
            imageType = FIT_RGBAF;
            break;

        default:
            throw(std::exception("Invalid image format - FreeImageCodec::encode"));
        };

        // Check support for this image type & bit depth
        if (!FreeImage_FIFSupportsExportType((FREE_IMAGE_FORMAT)mFreeImageType, imageType) ||
            !FreeImage_FIFSupportsExportBPP((FREE_IMAGE_FORMAT)mFreeImageType, (int)PixelUtil::getNumElemBits(requiredFormat)))
        {
            // Ok, need to allocate a fallback
            // Only deal with RGBA -> RGB for now
            switch (requiredFormat)
            {
            case PF_BYTE_RGBA:
                requiredFormat = PF_BYTE_RGB;
                break;
            case PF_BYTE_BGRA:
                requiredFormat = PF_BYTE_BGR;
                break;
            default:
                break;
            };

        }

        bool conversionRequired = false;
        unsigned char* srcData = input->getPtr();

        // Check BPP
        unsigned bpp = static_cast<unsigned>(PixelUtil::getNumElemBits(requiredFormat));
        if (!FreeImage_FIFSupportsExportBPP((FREE_IMAGE_FORMAT)mFreeImageType, (int)bpp))
        {
            if (bpp == 32 && PixelUtil::hasAlpha(pImgData->format) && FreeImage_FIFSupportsExportBPP((FREE_IMAGE_FORMAT)mFreeImageType, 24))
            {
                // drop to 24 bit (lose alpha)
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
                requiredFormat = PF_BYTE_RGB;
#else
                requiredFormat = PF_BYTE_BGR;
#endif
                bpp = 24;
            }
            else if (bpp == 128 && PixelUtil::hasAlpha(pImgData->format) && FreeImage_FIFSupportsExportBPP((FREE_IMAGE_FORMAT)mFreeImageType, 96))
            {
                // drop to 96-bit floating point
                requiredFormat = PF_FLOAT32_RGB;
            }
        }

        PixelBox convBox(pImgData->width, pImgData->height, 1, requiredFormat);
        if (requiredFormat != pImgData->format)
        {
            conversionRequired = true;
            // Allocate memory
            convBox.data = (uint8_t*)malloc(sizeof(uint8_t)*convBox.getConsecutiveSize());
            // perform conversion and reassign source
            PixelBox newSrc(pImgData->width, pImgData->height, 1, pImgData->format, input->getPtr());
            PixelUtil::bulkPixelConversion(newSrc, convBox);
            srcData = static_cast<unsigned char*>(convBox.data);
        }


        ret = FreeImage_AllocateT(
            imageType,
            static_cast<int>(pImgData->width),
            static_cast<int>(pImgData->height),
            bpp);

        if (!ret)
        {
            if (conversionRequired)
                free(convBox.data);

            throw(std::exception("FreeImage_AllocateT failed - possibly out of memory. "));
        }

        if (requiredFormat == PF_L8 || requiredFormat == PF_A8)
        {
            // Must explicitly tell FreeImage that this is greyscale by setting
            // a "grey" palette (otherwise it will save as a normal RGB
            // palettized image).
            FIBITMAP *tmp = FreeImage_ConvertToGreyscale(ret);
            FreeImage_Unload(ret);
            ret = tmp;
        }
        else if (requiredFormat == PF_L16)
        {
            FIBITMAP* tmp = FreeImage_ConvertToType(ret, FIT_UINT16);
            FreeImage_Unload(ret);
            ret = tmp;
        }

        size_t dstPitch = FreeImage_GetPitch(ret);
        size_t srcPitch = pImgData->width * PixelUtil::getNumElemBytes(requiredFormat);


        // Copy data, invert scanlines and respect FreeImage pitch
        uint8_t* pSrc;
        uint8_t* pDst = FreeImage_GetBits(ret);
        for (size_t y = 0; y < pImgData->height; ++y)
        {
            pSrc = srcData + (pImgData->height - y - 1) * srcPitch;
            memcpy(pDst, pSrc, srcPitch);
            pDst += dstPitch;
        }

        if (conversionRequired)
        {
            // delete temporary conversion area
            free(convBox.data);
        }

        return ret;
    }
    //---------------------------------------------------------------------
    DataStreamPtr FreeImageCodec::code(MemoryDataStreamPtr& input, Codec::CodecDataPtr& pData) const
    {        
        FIBITMAP* fiBitmap = encode(input, pData);

        // open memory chunk allocated by FreeImage
        FIMEMORY* mem = FreeImage_OpenMemory();
        // write data into memory
        FreeImage_SaveToMemory((FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, mem);
        // Grab data information
        BYTE* data = nullptr;
        DWORD size;
        FreeImage_AcquireMemory(mem, &data, &size);
        // Copy data into our own buffer
        // Because we're asking MemoryDataStream to free this, must create in a compatible way
        BYTE* ourData = (BYTE*)malloc(size*sizeof(BYTE));
        memcpy(ourData, data, size);
        // Wrap data in stream, tell it to free on close 
        DataStreamPtr outstream(new MemoryDataStream(ourData, size, true));
        // Now free FreeImage memory buffers
        FreeImage_CloseMemory(mem);
        // Unload bitmap
        FreeImage_Unload(fiBitmap);

        return outstream;
    }
    //---------------------------------------------------------------------
    void FreeImageCodec::codeToFile(MemoryDataStreamPtr& input, 
                                    const std::string& outFileName, 
                                    Codec::CodecDataPtr& pData) const
    {
        FIBITMAP* fiBitmap = encode(input, pData);

        if ((FREE_IMAGE_FORMAT)mFreeImageType == FIF_TIFF)
        {
            FreeImage_Save((FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, outFileName.c_str(), TIFF_NONE);
        }
        else
        {
            FreeImage_Save((FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, outFileName.c_str());
        }
        FreeImage_Unload(fiBitmap);
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult FreeImageCodec::decode(DataStreamPtr& input) const
    {
        MemoryDataStream memStream(input, true);

        FIMEMORY* fiMem = 
            FreeImage_OpenMemory(memStream.getPtr(), static_cast<uint32_t>(memStream.size()));
        // TIFF - 18.
        FIBITMAP* fiBitmap = FreeImage_LoadFromMemory(
            (FREE_IMAGE_FORMAT)mFreeImageType, fiMem);
        if (!fiBitmap)
        {
            throw(std::exception("Error decoding image"));
        }


        ImageData* imgData = new ImageData();
        MemoryDataStreamPtr output;

        imgData->depth = 1; // only 2D formats handled by this codec
        imgData->width = FreeImage_GetWidth(fiBitmap);
        imgData->height = FreeImage_GetHeight(fiBitmap);
        imgData->num_mipmaps = 0; // no mipmaps in non-DDS 
        imgData->flags = 0;

        // Must derive format first, this may perform conversions
        
        FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(fiBitmap);
        FREE_IMAGE_COLOR_TYPE colourType = FreeImage_GetColorType(fiBitmap);
        unsigned bpp = FreeImage_GetBPP(fiBitmap);

        switch(imageType)
        {
        case FIT_UNKNOWN:
        case FIT_COMPLEX:
        case FIT_UINT32:
        case FIT_INT32:
        case FIT_DOUBLE:
        default:
            throw(std::exception("Unknown or unsupported image format - FreeImageCodec::decode"));
                
            break;
        case FIT_BITMAP:
            // Standard image type
            // Perform any colour conversions for greyscale
            if (colourType == FIC_MINISWHITE || colourType == FIC_MINISBLACK)
            {
                FIBITMAP* newBitmap = FreeImage_ConvertToGreyscale(fiBitmap);
                // free old bitmap and replace
                FreeImage_Unload(fiBitmap);
                fiBitmap = newBitmap;
                // get new formats
                bpp = FreeImage_GetBPP(fiBitmap);
            }
            // Perform any colour conversions for RGB
            else if (bpp < 8 || colourType == FIC_PALETTE || colourType == FIC_CMYK)
            {
                FIBITMAP* newBitmap =  nullptr;    
                if (FreeImage_IsTransparent(fiBitmap))
                {
                    // convert to 32 bit to preserve the transparency 
                    // (the alpha byte will be 0 if pixel is transparent)
                    newBitmap = FreeImage_ConvertTo32Bits(fiBitmap);
                }
                else
                {
                    // no transparency - only 3 bytes are needed
                    newBitmap = FreeImage_ConvertTo24Bits(fiBitmap);
                }

                // free old bitmap and replace
                FreeImage_Unload(fiBitmap);
                fiBitmap = newBitmap;
                // get new formats
                bpp = FreeImage_GetBPP(fiBitmap);
            }

            // by this stage, 8-bit is greyscale, 16/24/32 bit are RGB[A]
            switch(bpp)
            {
            case 8:
            case 16:
            case 24:
            {
                FIBITMAP* newBitmap = FreeImage_ConvertTo32Bits(fiBitmap);
                FreeImage_Unload(fiBitmap);
                fiBitmap = newBitmap;
            }
            case 32:
                imgData->format = PF_BYTE_RGBA;
                break;
                
                
            };
            break;
        case FIT_UINT16:
        case FIT_INT16:
            // 16-bit greyscale
            imgData->format = PF_L16;
            break;
        case FIT_FLOAT:
            // Single-component floating point data
            imgData->format = PF_FLOAT32_R;
            break;
        case FIT_RGB16:
            imgData->format = PF_SHORT_RGB;
            break;
        case FIT_RGBA16:
            imgData->format = PF_SHORT_RGBA;
            break;
        case FIT_RGBF:
            imgData->format = PF_FLOAT32_RGB;
            break;
        case FIT_RGBAF:
            imgData->format = PF_FLOAT32_RGBA;
            break;
            
            
        };

        unsigned char* srcData = FreeImage_GetBits(fiBitmap);
        unsigned srcPitch = FreeImage_GetPitch(fiBitmap);

        // Final data - invert image and trim pitch at the same time
        size_t dstPitch = imgData->width * PixelUtil::getNumElemBytes(imgData->format);
        imgData->size = dstPitch * imgData->height;
        // Bind output buffer
        output.reset(new MemoryDataStream(imgData->size));

        uint8_t* pSrc;
        uint8_t* pDst = output->getPtr();
        for (size_t y = 0; y < imgData->height; ++y)
        {
            pSrc = srcData + (imgData->height - y - 1) * srcPitch;
            memcpy(pDst, pSrc, dstPitch);
            pDst += dstPitch;
        }

        
        FreeImage_Unload(fiBitmap);
        FreeImage_CloseMemory(fiMem);


        DecodeResult ret;
        ret.first = output;
        ret.second = CodecDataPtr(imgData);
        return ret;

    }
    //---------------------------------------------------------------------    
    std::string FreeImageCodec::getType() const 
    {
        return mType;
    }
    //---------------------------------------------------------------------
    std::string FreeImageCodec::magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const
    {
        FIMEMORY* fiMem = 
            FreeImage_OpenMemory((BYTE*)magicNumberPtr, static_cast<uint32_t>(maxbytes));

        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(fiMem, (int)maxbytes);
        FreeImage_CloseMemory(fiMem);

        if (fif != FIF_UNKNOWN)
        {
            std::string ext(FreeImage_GetFormatFromFIF(fif));
            Ctr::stringToLower(ext);
            return ext;
        }
        else
        {
            return std::string();
        }
    }
}

#endif