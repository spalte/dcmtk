/*
 *
 *  Copyright (C) 2015-2019, Open Connections GmbH
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation are maintained by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module: dcmiod
 *
 *  Author: Michael Onken
 *
 *  Purpose: Class for managing common types needed by dcmiod module
 *
 */

#ifndef IODTYPES_H
#define IODTYPES_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmiod/ioddef.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/ofstd/ofcond.h"
#include <cstddef>

// ----------------------------------------------------------------------------
// Define the loggers for this module
// ----------------------------------------------------------------------------

extern DCMTK_DCMIOD_EXPORT OFLogger DCM_dcmiodLogger;

#define DCMIOD_TRACE(msg) OFLOG_TRACE(DCM_dcmiodLogger, msg)
#define DCMIOD_DEBUG(msg) OFLOG_DEBUG(DCM_dcmiodLogger, msg)
#define DCMIOD_INFO(msg) OFLOG_INFO(DCM_dcmiodLogger, msg)
#define DCMIOD_WARN(msg) OFLOG_WARN(DCM_dcmiodLogger, msg)
#define DCMIOD_ERROR(msg) OFLOG_ERROR(DCM_dcmiodLogger, msg)
#define DCMIOD_FATAL(msg) OFLOG_FATAL(DCM_dcmiodLogger, msg)

// ----------------------------------------------------------------------------
// Error constants
// ----------------------------------------------------------------------------

extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_WrongSOPClass;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_MissingAttribute;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_MissingSequenceData;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_InvalidDimensions;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_CannotInsertFrame;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_InvalidPixelData;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_InvalidObject;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_CannotDecompress;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_NoSuchRule;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_InvalidLaterality;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_InvalidElementValue;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_InvalidReference;
extern DCMTK_DCMIOD_EXPORT const OFConditionConst IOD_EC_ReferencesOmitted;

/** Class that wraps some constant definitions in the context of IODs
 */
class DCMTK_DCMIOD_EXPORT DcmIODTypes
{

public:

    struct FrameBase
    {
        /// Destructor
        FrameBase() {}
        virtual size_t getLength() = 0;
        virtual void* getPixelData() = 0;
        virtual Uint8 bytesPerPixel() = 0;
        virtual Uint8 getUint8AtIndex(size_t index) = 0;
        virtual Uint16 getUint16AtIndex(size_t index) = 0;
        virtual void setReleaseMemory(OFBool release) = 0;
        virtual OFString print() = 0;
        virtual ~FrameBase() {}
    };

    /** Struct representing a single frame
     */
    template<typename PixelType>
    struct Frame : public FrameBase
    {
        Frame() : pixData(NULL), length(0), releaseMemory(OFTrue) {}

        Frame(const size_t lengthInBytes) : pixData(NULL), length(lengthInBytes), releaseMemory(OFTrue)
        {
            pixData = new PixelType[lengthInBytes];
        }

        Frame(PixelType* pixelData, const size_t lengthInBytes) : pixData(pixelData), length(lengthInBytes), releaseMemory(OFTrue)
        {
        }

        Frame(const Frame& rhs)
        {
            delete[] pixData;
            pixData = new PixelType[rhs.length];
            memcpy(pixData, rhs.pixData, rhs.length);
            length = rhs.length;
            releaseMemory = rhs.releaseMemory;
        };

        /// Assignment operator
        Frame& operator=(const Frame& rhs)
        {
            if (this != &rhs)
            {
                delete[] pixData;
                pixData = new PixelType[rhs.length];
                memcpy(pixData, rhs.pixData, rhs.length);
                length = rhs.length;
                releaseMemory = rhs.releaseMemory;
            }
            return *this;
        }

        virtual void setReleaseMemory(OFBool release)
        {
            releaseMemory = release;
        }

        virtual size_t getLength()
        {
            return length;
        }

        virtual void* getPixelData()
        {
            return pixData;
        }

        virtual PixelType* getPixelDataTyped()
        {
            return pixData;
        }

        virtual Uint8 bytesPerPixel()
        {
            return sizeof(PixelType);
        }

        virtual Uint8 getUint8AtIndex(size_t index)
        {
            return static_cast<Uint8>(pixData[index]);
        }

        virtual Uint16 getUint16AtIndex(size_t index)
        {
            return static_cast<Uint16>(pixData[index]);
        }

        virtual OFString print()
        {
            OFStringStream ss;
            ss << "Frame with " << length + " bytes:\n";
            for (size_t i = 0; i < length; i++)
            {
                ss << STD_NAMESPACE hex << (Uint16)pixData[i] + " ";
            }
            ss << "\n";
            return ss.str().c_str();
        }

        /// Array for the pixel data bytes
        PixelType* pixData;
        /// Number of pixel data in bytes
        size_t length;
        // Denote whether to release memory in destructor
        OFBool releaseMemory;
        /// Destructor, frees memory
        ~Frame()
        {
            if (releaseMemory)
            {
                delete[] pixData;
                pixData = NULL;
            }
        }
    };

    /** IOD Information Entities (incomplete list, extended as needed)
     */
    enum IOD_IE
    {
        /// Undefined Information Entity (i.e.\ no value set)
        IE_UNDEFINED,
        /// Patient Entity
        IE_PATIENT,
        /// Study Entity
        IE_STUDY,
        /// Series Entity
        IE_SERIES,
        /// Frame of Reference Entity
        IE_FOR,
        /// Equipment Entity
        IE_EQUIPMENT,
        /// Image Entity
        IE_IMAGE,
        //// Meta Entity: Instance covering image, waveform, etc.
        IE_INSTANCE
    };

    /** Enumerated values for attribute "Laterality"
     */
    enum IOD_LATERALITY
    {
        /// Undefined (e.g.\ value not set)
        LATERALITY_UNDEFINED,
        /// Left body part
        LATERALITY_L,
        /// Right body part
        LATERALITY_R
    };

    /** Enhanced US Image Module: Image Type (first value)
     */
    enum IOD_ENHUSIMAGETYPE
    {
        /// Unknown
        IMAGETYPE_UNKNOWN,
        /// ORIGINAL
        IMAGETYPE_ORIGINAL,
        /// DERIVED
        IMAGETYPE_DERIVED
    };

private:
    /** Private undefined default constructor
     */
    DcmIODTypes() {};

    /** Private undefined destructor
     */
    ~DcmIODTypes() {};
};

#endif // IODTYPES_H
