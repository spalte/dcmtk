/*
 *  Copyright (C) 2015-2023, Open Connections GmbH
 *
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
 *  Module:  dcmseg
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Class representing a Segmentation object
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmfg/concatenationcreator.h"
#include "dcmtk/dcmfg/fgderimg.h"
#include "dcmtk/dcmfg/fgfact.h"
#include "dcmtk/dcmfg/fgfracon.h"
#include "dcmtk/dcmfg/fgplanor.h"
#include "dcmtk/dcmfg/fgplanpo.h"
#include "dcmtk/dcmfg/fgseg.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"
#include "dcmtk/dcmseg/segutils.h"

// Explicit template instantiation
template class DCMTK_DCMSEG_EXPORT DcmSegmentation<Uint8>;
template class DCMTK_DCMSEG_EXPORT DcmSegmentation<Uint16>;

// default constructor (protected, instance creation via create() function)
template <typename BitsAlloc>
DcmSegmentation<BitsAlloc>::DcmSegmentation()
    : DcmSegmentation<BitsAlloc>::IODImage(OFin_place<IODImagePixelModule<BitsAlloc> >)
    , m_SegmentationSeries(DcmSegmentation<BitsAlloc>::IODImage::getData(), DcmSegmentation<BitsAlloc>::IODImage::getRules())
    , m_EnhancedGeneralEquipmentModule(DcmSegmentation<BitsAlloc>::IODImage::getData(), DcmSegmentation<BitsAlloc>::IODImage::getRules())
    , m_FG(DcmSegmentation<BitsAlloc>::IODImage::getData(), DcmSegmentation<BitsAlloc>::IODImage::getRules())
    , m_DimensionModule(DcmSegmentation<BitsAlloc>::IODImage::getData(), DcmSegmentation<BitsAlloc>::IODImage::getRules())
    , m_Frames()
    , m_ImageType("DERIVED\\PRIMARY")
    , m_ContentIdentificationMacro()
    , m_SegmentationType(DcmSegTypes::ST_BINARY)
    , m_SegmentationFractionalType(DcmSegTypes::SFT_OCCUPANCY)
    , m_MaximumFractionalValue(DCM_MaximumFractionalValue)
    , m_Segments()
    , m_FGInterface()
{
    DcmSegmentation<BitsAlloc>::initIODRules();
}

template <typename BitsAlloc>
void DcmSegmentation<BitsAlloc>::initIODRules()
{
    // ------------ Segmentation Image Module -------------

    // Partly overrides rules from General Image Module
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(new IODRule(DCM_ImageType, "2", "1", "SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(new IODRule(DCM_SegmentationType, "1", "1", "SegmentationImageModule", DcmIODTypes::IE_IMAGE),
                        OFTrue);
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(
        new IODRule(DCM_SegmentationFractionalType, "1", "1C", "SegmentationImageModule", DcmIODTypes::IE_IMAGE),
        OFTrue);
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(
        new IODRule(DCM_MaximumFractionalValue, "1", "1C", "SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);

    // Re-use General Image Module instead of Segmentation Image Module
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(new IODRule(DCM_LossyImageCompression, "1", "1", "GeneralImageModule", DcmIODTypes::IE_IMAGE),
                        OFTrue);
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(
        new IODRule(DCM_LossyImageCompressionMethod, "1-n", "1C", "GeneralImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(
        new IODRule(DCM_LossyImageCompressionRatio, "1-n", "1C", "GeneralImageModule", DcmIODTypes::IE_IMAGE), OFTrue);

    // Override rule from General Series Module
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(new IODRule(DCM_ReferencedPerformedProcedureStepSequence,
                                    "1",
                                    "1C",
                                    "SegmentationSeriesModule",
                                    DcmIODTypes::IE_SERIES),
                        OFTrue);
    typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->addRule(new IODRule(DCM_SeriesNumber, "1", "1", "SegmentationSeriesModule", DcmIODTypes::IE_SERIES),
                        OFTrue);

    // Instance Number is also used within Content Identification Macro, disable it there
    m_ContentIdentificationMacro.getIODRules().deleteRule(DCM_InstanceNumber);
}

template <typename BitsAlloc>
DcmSegmentation<BitsAlloc>::~DcmSegmentation()
{
    clearData();
}

// static method for loading segmentation objects
template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::loadFile(const OFString& filename, DcmSegmentation*& segmentation)
{
    DcmFileFormat dcmff;
    DcmDataset* dataset = NULL;
    OFCondition result  = loadFile(dcmff, filename, dataset);
    if (result.bad())
        return result;

    return loadDataset(*dataset, segmentation);
}

// static method for loading segmentation objects
template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::loadDataset(DcmDataset& dataset, DcmSegmentation*& segmentation)
{
    segmentation       = NULL;
    OFCondition result = DcmSegmentation<BitsAlloc>::decompress(dataset);
    if (result.bad())
        return result;

    DcmSegmentation* temp = new DcmSegmentation();
    if (temp == NULL)
    {
        return EC_MemoryExhausted;
    }

    result = temp->read(dataset);
    if (result.good())
    {
        segmentation = temp;
    }
    else
    {
        delete segmentation;
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::loadConcatenation(ConcatenationLoader& cl,
                                               const OFString& concatenationUID,
                                               DcmSegmentation*& segmentation)
{
    DcmDataset dset;
    segmentation = NULL;
    OFVector<DcmIODTypes::Frame*> frames;
    OFCondition result = cl.load(concatenationUID, &dset, frames);
    if (result.good())
    {
        segmentation = new DcmSegmentation();
        if (segmentation)
        {
            result = segmentation->readWithoutPixelData(dset);
            if (result.good())
            {
                segmentation->m_Frames = frames;
            }
        }
        else
        {
            result = EC_MemoryExhausted;
        }
    }
    if (result.bad())
    {
        DcmIODUtil::freeContainer(frames);
        delete segmentation;
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::createBinarySegmentation(DcmSegmentation*& segmentation,
                                                      const Uint16 rows,
                                                      const Uint16 columns,
                                                      const IODGeneralEquipmentModule::EquipmentInfo& equipmentInfo,
                                                      const ContentIdentificationMacro& contentIdentification)
{

    OFCondition result = createCommon(segmentation, rows, columns, equipmentInfo, contentIdentification);
    if (result.bad())
        return result;

    segmentation->m_SegmentationType = DcmSegTypes::ST_BINARY;

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::createFractionalSegmentation(DcmSegmentation*& segmentation,
                                                          const Uint16 rows,
                                                          const Uint16 columns,
                                                          const DcmSegTypes::E_SegmentationFractionalType fractType,
                                                          const Uint16& maxFractionalValue,
                                                          const IODGeneralEquipmentModule::EquipmentInfo& equipmentInfo,
                                                          const ContentIdentificationMacro& contentIdentification)
{
    OFCondition result = createCommon(segmentation, rows, columns, equipmentInfo, contentIdentification);
    if (result.bad())
        return result;

    segmentation->m_SegmentationType           = DcmSegTypes::ST_FRACTIONAL;
    segmentation->m_SegmentationFractionalType = fractType;
    segmentation->m_MaximumFractionalValue.putUint16(maxFractionalValue);

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::createCommon(DcmSegmentation*& segmentation,
                                          const Uint16 rows,
                                          const Uint16 columns,
                                          const IODGeneralEquipmentModule::EquipmentInfo& equipmentInfo,
                                          const ContentIdentificationMacro& contentIdentification)
{
    if ((rows == 0) || (columns == 0))
    {
        DCMSEG_ERROR("Segmentation must have at least 1 row and 1 column");
        return EC_IllegalParameter;
    }

    segmentation = new DcmSegmentation();
    if (segmentation == NULL)
        return EC_MemoryExhausted;

    segmentation->getImagePixel().setRows(rows);
    segmentation->getImagePixel().setColumns(columns);

    OFCondition result = segmentation->setContentIdentification(contentIdentification);
    if (result.good())
    {
        OFString tempstr;
        contentIdentification.getInstanceNumber(tempstr);
        result = segmentation->getGeneralImage().setInstanceNumber(tempstr);
        if (result.bad())
        {
            delete segmentation;
            segmentation = NULL;
            return EC_InvalidValue;
        }

        DcmIODUtil::setContentDateAndTimeNow(segmentation->getGeneralImage());
        result = segmentation->setEquipmentInfo(equipmentInfo, OFTrue /* check */);
    }

    if (result.bad())
    {
        delete segmentation;
        segmentation = NULL;
    }

    return result;
}

template <typename BitsAlloc>
FGDerivationImage*
DcmSegmentation<BitsAlloc>::createDerivationImageFG(const OFVector<ImageSOPInstanceReferenceMacro>& derivationImages,
                                         const OFString& derivationDescription)
{
    CodeSequenceMacro derivationCode("113076", "DCM", "Segmentation");
    CodeSequenceMacro purpose("121322", "DCM", "Source Image for Image Processing Operation");
    return FGDerivationImage::createMinimal(derivationImages, derivationDescription, derivationCode, purpose);
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::read(DcmItem& dataset)
{

    OFCondition result = readWithoutPixelData(dataset);
    if (result.good())
        result = readFrames(dataset);
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::readWithoutPixelData(DcmItem& dataset)
{
    OFString sopClass;
    if (DcmIODUtil::checkSOPClass(&dataset, UID_SegmentationStorage, sopClass).bad())
    {
        DCMSEG_ERROR("Given file does not seem to be a segmentation storage object since SOP class is: " << sopClass);
        return IOD_EC_WrongSOPClass;
    }

    // Read attributes in base classes
    DcmSegmentation<BitsAlloc>::IODImage::read(dataset);

    // Read Segmentation Series Module
    m_SegmentationSeries.read(dataset);

    // Read Enhanced General Equipment (i.e. make sure all type 1 elements are
    // there, which is not checked in General Equipment Module being part of
    // DcmIODImage.
    m_EnhancedGeneralEquipmentModule.read(dataset);

    // Read functional groups module
    m_FG.read(dataset);

    // Read functional groups itself
    m_FGInterface.read(dataset);

    // Read dimension information
    m_DimensionModule.read(dataset);

    readSegmentationType(dataset);

    readSegments(dataset);

    readSegmentationFractionalType(dataset);

    m_ContentIdentificationMacro.read(dataset);

    // Read specific segmentation elements
    DcmIODUtil::getAndCheckElementFromDataset(
        dataset, m_MaximumFractionalValue, typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->getByTag(DCM_MaximumFractionalValue));

    return EC_Normal;
}

template <typename BitsAlloc>
void DcmSegmentation<BitsAlloc>::setCheckFGOnWrite(const OFBool doCheck)
{
    m_FGInterface.setCheckOnWrite(doCheck);
}

template <typename BitsAlloc>
OFBool DcmSegmentation<BitsAlloc>::getCheckFGOnWrite()
{
    return m_FGInterface.getCheckOnWrite();
}

template <typename BitsAlloc>
void DcmSegmentation<BitsAlloc>::setCheckDimensionsOnWrite(const OFBool doCheck)
{
    m_DimensionModule.setCheckOnWrite(doCheck);
}

template <typename BitsAlloc>
OFBool DcmSegmentation<BitsAlloc>::getCheckDimensionsOnWrite()
{
    return m_DimensionModule.getCheckOnWrite();
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeWithSeparatePixelData(DcmItem& dataset, Uint8*& pixData, size_t& pixDataLength)
{
    // FGInterface::write() will know whether it has to check FG structure
    // so we do not need to check FG structure here (OFFalse).
    if (!check(OFFalse))
    {
        return IOD_EC_InvalidObject;
    }

    OFCondition result;

    // -- Set constant default values written by external modules --
    typename DcmSegmentation<BitsAlloc>::IODImage().getGeneralImage().setLossyImageCompression("00");
    typename DcmSegmentation<BitsAlloc>::IODImage().getGeneralImage().setImageType(m_ImageType);
    typename DcmSegmentation<BitsAlloc>::IODImage().getSOPCommon().setSOPClassUID(UID_SegmentationStorage);

    // -- Extra Study level data --

    // Enhanced Equipment Module
    if (result.good())
        result = m_EnhancedGeneralEquipmentModule.write(dataset);

    // -- Extra Series level data --

    // Write segmentation-specific series level attribute (Segmentation Series Module)
    if (result.good())
        result = m_SegmentationSeries.write(dataset);

    // -- Extra Image level data --

    // Write Multi-Frame Functional Groups Module
    if (result.good())
        result = writeMultiFrameFunctionalGroupsModule(dataset);

    // Write Multi-Frame Dimension Module
    if (result.good())
        result = writeMultiFrameDimensionModule(dataset);

    // Write segmentation image module and image pixel module
    if (result.good())
        result = writeSegmentationImageModule(dataset);

    // -- Write common multi frame image IOD attributes --

    // Patient Module
    // General Study Module
    // General Series Module
    // Frame of Reference Module
    // General Equipment Module
    // General Image Module
    // Multi-frame Functional Groups Module (except functional groups itself)
    // SOP Common Module
    // Common Instance Reference Module
    if (result.good())
        result = DcmSegmentation<BitsAlloc>::IODImage::write(dataset);

    // Write frame pixel data
    if (result.good())
    {
        Uint32 numFrames = DcmIODUtil::limitMaxFrames(
            m_Frames.size(), "More than 2147483647 frames provided, will only write 2147483647");
        Uint16 rows, cols;
        rows = cols = 0;
        getImagePixel().getRows(rows);
        getImagePixel().getColumns(cols);
        result = getTotalBytesRequired(rows, cols, numFrames, pixDataLength);
        if (result.bad())
            return result;

        pixData = new Uint8[pixDataLength];
        if (!pixData)
            return EC_MemoryExhausted;

        if (m_SegmentationType == DcmSegTypes::ST_BINARY)
            result = writeBinaryFrames(pixData, rows, cols, pixDataLength);
        else if (m_SegmentationType == DcmSegTypes::ST_FRACTIONAL)
            result = writeFractionalFrames(pixData);
        else
            result = SG_EC_UnknownSegmentationType;
        if (result.bad())
        {
            delete[] pixData;
        }
    }

    return result;
}

template <typename BitsAlloc>
FGInterface& DcmSegmentation<BitsAlloc>::getFunctionalGroups()
{
    return m_FGInterface;
}

template <typename BitsAlloc>
IODMultiFrameFGModule::ConcatenationInfo& DcmSegmentation<BitsAlloc>::getConcatenationInfo()
{
    return m_FG.getConcatenationInfo();
}

template <typename BitsAlloc>
size_t DcmSegmentation<BitsAlloc>::getNumberOfFrames()
{
    return m_FGInterface.getNumberOfFrames();
}

template <typename BitsAlloc>
size_t DcmSegmentation<BitsAlloc>::getNumberOfSegments()
{
    return m_Segments.size();
}

template <typename BitsAlloc>
IODGeneralEquipmentModule& DcmSegmentation<BitsAlloc>::getEquipment()
{
    return DcmSegmentation<BitsAlloc>::IODImage::getEquipment();
}

template <typename BitsAlloc>
IODSegmentationSeriesModule& DcmSegmentation<BitsAlloc>::getSegmentationSeriesModule()
{
    return m_SegmentationSeries;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::addSegment(DcmSegment<BitsAlloc>* seg, Uint16& segmentNumber)
{
    segmentNumber = 0;
    if (seg == NULL)
        return EC_IllegalParameter;

    if (m_Segments.size() >= DCM_SEG_MAX_SEGMENTS)
    {
        return SG_EC_MaxSegmentsReached;
    }

    // Casting is safe since we made sure number of segments fits into 16 bit
    segmentNumber = OFstatic_cast(Uint16, m_Segments.size() + 1);
    m_Segments.push_back(seg);
    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::addFrame(Uint8* pixData)
{
    if (m_Frames.size() >= DCM_SEG_MAX_FRAMES)
        return SG_EC_MaxFramesReached;

    OFCondition result;
    Uint16 rows = 0;
    Uint16 cols = 0;
    if (getImagePixel().getRows(rows).good() && getImagePixel().getColumns(cols).good())
    {
        DcmIODTypes::Frame* frame = NULL;
        if (m_SegmentationType == DcmSegTypes::ST_BINARY)
        {
            frame = DcmSegUtils::packBinaryFrame(pixData, rows, cols);
            if (!frame)
            {
                result = IOD_EC_CannotInsertFrame;
            }
        }
        else // fractional or labelmap
        {
            Uint16 bits_allocated = 1;
            result = getImagePixel().getBitsAllocated(bits_allocated);
            frame = new DcmIODTypes::Frame();
            if (frame)
            {
                frame->length  = rows * cols;
                frame->pixData = new Uint8[frame->length * (bits_allocated / 8)];
                if (frame->pixData)
                {
                    memcpy(frame->pixData, pixData, frame->length);
                }
                else
                {
                    delete frame;
                    result = EC_MemoryExhausted;
                }
            }
            else
                result = EC_MemoryExhausted;
        }
        if (result.good())
        {
            m_Frames.push_back(frame);
        }
    }
    else
    {
        DCMSEG_ERROR("Cannot add frame since rows and/or columns are unknown");
        result = IOD_EC_CannotInsertFrame;
    }
    return result;
}

template <typename BitsAlloc>
SOPInstanceReferenceMacro& DcmSegmentation<BitsAlloc>::getReferencedPPS()
{
    return typename DcmSegmentation<BitsAlloc>::IODImage().getSeries().getReferencedPPS();
}

template <typename BitsAlloc>
const DcmIODTypes::Frame* DcmSegmentation<BitsAlloc>::getFrame(const size_t& frameNo)
{
    if (frameNo > m_Frames.size() - 1)
    {
        return NULL;
    }

    return m_Frames[frameNo];
}

template <typename BitsAlloc>
void DcmSegmentation<BitsAlloc>::getFramesForSegment(const size_t& segmentNumber, OFVector<size_t>& frameNumbers)
{
    size_t numFrames = getNumberOfFrames();
    for (size_t count = 0; count < numFrames; count++)
    {
        FGSegmentation* fg = OFstatic_cast(
            FGSegmentation*, m_FGInterface.get(OFstatic_cast(Uint32, count), DcmFGTypes::EFG_SEGMENTATION));
        if (fg == NULL)
        {
            DCMSEG_ERROR("Cannot get segmentation functional group for frame " << count);
            return;
        }
        Uint16 refSeg;
        if (fg->getReferencedSegmentNumber(refSeg).good())
        {
            if (OFstatic_cast(size_t, refSeg) == segmentNumber)
            {
                frameNumbers.push_back(count);
            }
        }
    }
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::addForAllFrames(const FGBase& group)
{
    return m_FGInterface.addShared(group);
}

template <typename BitsAlloc>
OFCondition
DcmSegmentation<BitsAlloc>::addFrame(Uint8* pixData, const Uint16 segmentNumber, const OFVector<FGBase*>& perFrameInformation)
{
    if (m_Frames.size() >= DCM_SEG_MAX_FRAMES)
        return SG_EC_MaxFramesReached;

    Uint32 frameNo = OFstatic_cast(Uint32, m_Frames.size()); // will be the index of the frame (counted from 0)
    OFCondition result;

    // Check input parameters
    if (pixData == NULL)
    {
        DCMSEG_ERROR("No pixel data provided or zero length");
        result = EC_IllegalParameter;
    }
    if (segmentNumber > m_Segments.size())
    {
        DCMSEG_ERROR("Cannot add frame: Segment with given number " << segmentNumber << " does not exist");
        result = SG_EC_NoSuchSegment;
    }
    if (result.bad())
        return result;

    OFVector<FGBase*>::const_iterator it = perFrameInformation.begin();
    while (it != perFrameInformation.end())
    {
        result = (*it)->check();
        if (result.bad())
        {
            DCMSEG_ERROR("Could not add new frame since functional group of type: "
                         << (*it)->getType() << " is invalid: " << result.text());
            break;
        }
        result = m_FGInterface.addPerFrame(frameNo, *(*it));
        if (result.bad())
        {
            DCMSEG_ERROR("Could not add new frame since functional group of type " << (*it)->getType() << ": "
                                                                                   << result.text());
            break;
        }
        it++;
    }

    // Now also add Segmentation Functional Group
    if (result.good())
    {
        FGSegmentation seg;
        result = seg.setReferencedSegmentNumber(segmentNumber);
        if (result.good())
        {
            result = m_FGInterface.addPerFrame(frameNo, seg);
        }
        else
        {
            DCMSEG_ERROR("Could not add new frame, invalid segment number " << segmentNumber << ": " << result.text());
        }
    }

    // Insert pixel data
    if (result.good())
    {
        result = addFrame(pixData);
    }

    // Cleanup any per-frame groups that might have been inserted and return
    if (result.bad())
    {
        for (OFVector<FGBase*>::const_iterator it2 = perFrameInformation.begin(); it2 != perFrameInformation.end();
             it2++)
        {
            m_FGInterface.deletePerFrame(frameNo, (*it2)->getType());
        }
    }

    return result;
}

template <typename BitsAlloc>
ContentIdentificationMacro& DcmSegmentation<BitsAlloc>::getContentIdentification()
{
    return m_ContentIdentificationMacro;
}

template <typename BitsAlloc>
IODMultiframeDimensionModule& DcmSegmentation<BitsAlloc>::getDimensions()
{
    return m_DimensionModule;
}

template <typename BitsAlloc>
OFCondition
DcmSegmentation<BitsAlloc>::setLossyImageCompressionFlag(const OFString& ratios, const OFString& methods, const OFBool checkValues)
{
    OFCondition result = typename DcmSegmentation<BitsAlloc>::IODImage().getGeneralImage().setLossyImageCompression("01");
    if (result.good() || !checkValues)
        result = typename DcmSegmentation<BitsAlloc>::IODImage().getGeneralImage().setLossyImageCompressionMethod(methods);
    if (result.good() || !checkValues)
        result = typename DcmSegmentation<BitsAlloc>::IODImage().getGeneralImage().setLossyImageCompressionRatio(ratios);

    if (checkValues)
        return result;
    else
        return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::saveFile(const OFString& filename, const E_TransferSyntax writeXfer)
{
    if ((writeXfer != EXS_LittleEndianExplicit) && (writeXfer != EXS_BigEndianExplicit)
        && (writeXfer != EXS_LittleEndianImplicit)
#ifdef WITH_ZLIB
        && (writeXfer != EXS_DeflatedLittleEndianExplicit)
#endif
    )
    {
        DcmXfer ts(writeXfer);
#ifdef WITH_ZLIB
        DCMSEG_ERROR("Cannot write transfer syntax: " << ts.getXferName()
                                                      << ": Can only write uncompressed or Deflated)");
#else
        if (writeXfer == EXS_DeflatedLittleEndianExplicit)
        {
            DCMSEG_ERROR("Cannot write transfer syntax: "
                         << ts.getXferName() << ": Deflate (ZLIB) support disabled, can only write uncompressed");
        }
#endif
        return EC_CannotChangeRepresentation;
    }

    DcmFileFormat dcmff;
    OFCondition result = writeDataset(*(dcmff.getDataset()));
    if (result.good())
    {
        result = dcmff.saveFile(filename.c_str(), writeXfer);
    }
    if (result.bad())
    {
        DCMSEG_ERROR("Cannot save segmentation document to file " << filename << ": " << result.text());
    }

    return result;
}

/* -- Setter for DICOM attributes -- */
template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::setEquipmentInfo(const IODGeneralEquipmentModule::EquipmentInfo& equipmentInfo,
                                              const OFBool checkValue)
{

    if (checkValue)
    {
        if (equipmentInfo.m_Manufacturer.empty() || equipmentInfo.m_ManufacturerModelName.empty()
            || equipmentInfo.m_DeviceSerialNumber.empty() || equipmentInfo.m_SoftwareVersions.empty())
        {
            return EC_InvalidValue;
        }
    }

    OFCondition result = getEquipment().setManufacturer(equipmentInfo.m_Manufacturer, checkValue);
    if (result.good())
        result = getEquipment().setManufacturerModelName(equipmentInfo.m_ManufacturerModelName, checkValue);
    if (result.good())
        result = getEquipment().setDeviceSerialNumber(equipmentInfo.m_DeviceSerialNumber, checkValue);
    if (result.good())
        result = getEquipment().setSoftwareVersions(equipmentInfo.m_SoftwareVersions, checkValue);

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::setContentIdentification(const ContentIdentificationMacro& contentIdentification,
                                                      const OFBool checkValue)
{
    // Instance Number and Content Label must be filled out, rest can be empty
    OFCondition result;
    if (checkValue)
    {
        result = OFconst_cast(ContentIdentificationMacro*, &contentIdentification)->check();
    }
    if (result.bad())
        return result;

    m_ContentIdentificationMacro = contentIdentification;

    return result;
}

/* -- Getter for DICOM attributes -- */

template <typename BitsAlloc>
DcmSegment<BitsAlloc>* DcmSegmentation<BitsAlloc>::getSegment(const size_t segmentNumber)
{
    // check for invalid index
    if ((segmentNumber == 0) || (segmentNumber > m_Segments.size()))
    {
        return NULL;
    }

    // logical segment numbering starts with 1, so subtract 1 for vector index
    return m_Segments[segmentNumber - 1];
}

template <typename BitsAlloc>
OFBool DcmSegmentation<BitsAlloc>::getSegmentNumber(const DcmSegment<BitsAlloc>* segment, size_t& segmentNumber)
{
    segmentNumber = 0;
    size_t max    = m_Segments.size();
    for (size_t count = 0; count < max; count++)
    {
        if (m_Segments.at(count) == segment)
        {
            segmentNumber = OFstatic_cast(Uint16, count + 1);
            return OFTrue;
        }
    }
    // not found
    return OFFalse;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::getModality(OFString& value, const long signed int pos) const
{
    (void)pos;
    // Fixed for Segmentations to value "SEG"
    value = "SEG";
    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::importFromSourceImage(const OFString& filename, const bool takeOverCharset)
{
    DcmFileFormat dcmff;
    OFCondition result = dcmff.loadFile(filename);
    if (result.good())
    {
        return importFromSourceImage(*(dcmff.getDataset()), takeOverCharset);
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::importFromSourceImage(DcmItem& dataset, const bool takeOverCharset)
{
    OFString FoR;
    dataset.findAndGetOFStringArray(DCM_FrameOfReferenceUID, FoR);
    return DcmIODCommon::importHierarchy(dataset,
                                         OFTrue,       // Patient
                                         OFTrue,       // Study
                                         !FoR.empty(), // Frame of Reference
                                         OFFalse,      // Series
                                         takeOverCharset);
}

/* protected functions */

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeSegments(DcmItem& item)
{
    OFCondition result;
    DcmIODUtil::writeSubSequence<OFVector<DcmSegment<BitsAlloc>*> >(
        result, DCM_SegmentSequence, m_Segments, item, "1-n", "1", "SegmentationImageModule");
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::readSegments(DcmItem& item)
{
    return DcmIODUtil::readSubSequence<OFVector<DcmSegment<BitsAlloc>*> >(
        item, DCM_SegmentSequence, m_Segments, "1-n", "1", "SegmentationImageModule");
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::readFrames(DcmItem& dataset)
{
    OFCondition result;
    Uint16 allocated, stored, high, spp, pixelRep, rows, cols;
    Uint32 numberOfFrames = 0;
    allocated = stored = high = spp = rows = cols = 0;
    pixelRep                                      = 2; // invalid value for this attribute
    OFString colorModel;

    /* check the typical image pixel attributes and get correct(ed) values */
    result = getAndCheckImagePixelAttributes(
        dataset, allocated, stored, high, spp, pixelRep, rows, cols, numberOfFrames, colorModel);
    if (result.bad())
        return result;

    /* Check length of pixel data element */
    DcmElement* pixelData = NULL;
    if (dataset.findAndGetElement(DCM_PixelData, pixelData).bad())
        return IOD_EC_InvalidPixelData;
    if (!checkPixDataLength(pixelData, rows, cols, numberOfFrames))
        return IOD_EC_InvalidPixelData;

    /* Get pixel data values */
    Uint8* pixels = NULL;
    result        = pixelData->getUint8Array(pixels);
    if (result.bad())
    {
        DCMSEG_ERROR("Cannot read pixel data");
        return result;
    }

    /* Read all frames into dedicated data structure */
    size_t pixelsPerFrame = OFstatic_cast(size_t, rows) * cols;
    if (m_SegmentationType == DcmSegTypes::ST_BINARY)
    {
        result = DcmIODUtil::extractBinaryFrames(pixels, numberOfFrames, pixelsPerFrame, m_Frames);
        if (result.bad())
        {
            return result;
        }
    }
    else if (m_SegmentationType == DcmSegTypes::ST_FRACTIONAL)
    {
        for (size_t count = 0; count < numberOfFrames; count++)
        {
            DcmIODTypes::Frame* frame = new DcmIODTypes::Frame();
            if (!frame)
                return EC_MemoryExhausted;
            frame->length  = pixelsPerFrame;
            frame->pixData = new Uint8[pixelsPerFrame];
            if (!frame->pixData)
            {
                delete frame;
                return EC_MemoryExhausted;
            }
            memcpy(frame->pixData, pixels + count * pixelsPerFrame, pixelsPerFrame);
            m_Frames.push_back(frame);
        }
    }

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::getAndCheckImagePixelAttributes(DcmItem& dataset,
                                                             Uint16& allocated,
                                                             Uint16& stored,
                                                             Uint16& high,
                                                             Uint16& spp,
                                                             Uint16& pixelRep,
                                                             Uint16& rows,
                                                             Uint16& cols,
                                                             Uint32& numberOfFrames,
                                                             OFString& colorModel)
{
    OFBool fail = OFFalse;
    dataset.findAndGetUint16(DCM_BitsAllocated, allocated);
    dataset.findAndGetUint16(DCM_BitsStored, stored);
    dataset.findAndGetUint16(DCM_HighBit, high);
    dataset.findAndGetUint16(DCM_PixelRepresentation, pixelRep);
    dataset.findAndGetUint16(DCM_SamplesPerPixel, spp);
    dataset.findAndGetOFStringArray(DCM_PhotometricInterpretation, colorModel);

    /* Rows and Columns */
    OFCondition result = getImagePixel().getRows(rows);
    if (result.good())
        result = getImagePixel().getColumns(cols);
    if (result.bad())
    {
        DCMSEG_ERROR("Cannot find Rows or Columns in dataset");
        fail = OFTrue;
    }

    /* Number of Frames */
    Sint32 numFrames = 0;
    result           = m_FG.getNumberOfFrames(numFrames);
    if (result.bad())
    {
        DCMSEG_ERROR("Number of Frames not set");
        fail = OFTrue;
    }
    else
    {
        if (numFrames < 0)
        {
            DCMSEG_ERROR("Number of Frames must be greater than 0");
            fail = OFTrue;
        }
        else
        {
            numberOfFrames = OFstatic_cast(Uint32, numFrames);
        }
    }

    Uint16 depth = 0;
    if (m_SegmentationType == DcmSegTypes::ST_BINARY)
        depth = 1;
    else
        depth = 8;

    if (allocated != depth)
    {
        DCMSEG_WARN("Bits Allocated is not set correctly ("
                    << allocated << ", ignored), assuming value " << depth << " as required for "
                    << DcmSegTypes::segtype2OFString(m_SegmentationType) << " segmentation objects");
        allocated = depth;
    }
    if (stored != depth)
    {
        DCMSEG_WARN("Bits Stored is not set correctly ("
                    << stored << ", ignored), assuming value " << depth << " as required for "
                    << DcmSegTypes::segtype2OFString(m_SegmentationType) << " segmentation objects");
        stored = depth;
    }
    if (high != depth - 1)
    {
        DCMSEG_WARN("High Bit is not set correctly ("
                    << high << ", ignored), assuming value " << depth - 1 << " as required for "
                    << DcmSegTypes::segtype2OFString(m_SegmentationType) << " segmentation objects");
        high = depth - 1;
    }
    if (spp != 1)
    {
        DCMSEG_WARN("Samples per Pixel is not set correctly ("
                    << spp << ", ignored), assuming value 1 as required for segmentation objects");
        spp = 1;
    }
    if (pixelRep != 0)
    {
        DCMSEG_WARN("Pixel Representation is not set correctly ("
                    << pixelRep << ", ignored), assuming value 0 as required for segmentation objects");
        pixelRep = 0;
    }
    if (colorModel != "MONOCHROME2")
    {
        DCMSEG_WARN("Photometric Interpretation is not set correctly (ignored), assuming value MONOCHROME2 as required "
                    "for segmentation objects");
        colorModel = "MONOCHROME2";
    }
    if (rows == 0)
    {
        DCMSEG_ERROR("Rows is not set correctly (0)");
        fail = OFTrue;
    }
    if (cols == 0)
    {
        DCMSEG_ERROR("Columns is not set correctly (0)");
        fail = OFTrue;
    }

    if (fail)
        return EC_InvalidValue;

    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeDataset(DcmItem& dataset)
{
    Uint8* pixData = NULL;
    size_t pixDataLength;
    OFCondition result = writeWithSeparatePixelData(dataset, pixData, pixDataLength);
    if (result.good())
    {
        // Check whether pixel data length exceeds maximum number of bytes for uncompressed pixel data,
        // enforced by length field of Pixel Data attribute VR OB/OW if written in explicit VR transfer syntax.
        if (pixDataLength <= 4294967294UL)
        {
            result
                = dataset.putAndInsertUint8Array(DCM_PixelData, pixData, OFstatic_cast(unsigned long, pixDataLength));
        }
        else
        {
            result = FG_EC_PixelDataTooLarge;
        }
        delete[] pixData;
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeConcatenation(ConcatenationCreator& cc)
{
    Uint8* pixData       = NULL;
    size_t pixDataLength = 0;
    DcmItem* item        = new DcmItem();
    if (!item)
        return EC_MemoryExhausted;
    OFCondition result = writeWithSeparatePixelData(*item, pixData, pixDataLength);
    if (result.good())
    {
        result = cc.setCfgInput(item, pixData, pixDataLength, OFTrue /* transfer ownership */);
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeMultiFrameFunctionalGroupsModule(DcmItem& dataset)
{
    Uint32 numFrames = DcmIODUtil::limitMaxFrames(
        m_Frames.size(), "More than 2147483647 frames provided, limiting Number of Frames to 2147483647");
    m_FG.setNumberOfFrames(numFrames);
    OFCondition result = m_FG.write(dataset);
    if (result.good())
        m_FGInterface.write(dataset);
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeMultiFrameDimensionModule(DcmItem& dataset)
{
    return m_DimensionModule.write(dataset);
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeFractionalFrames(Uint8* pixData)
{
    OFVector<DcmIODTypes::Frame*>::iterator it = m_Frames.begin();
    // Just copy bytes for each frame as is
    for (size_t count = 0; it != m_Frames.end(); count++)
    {
        memcpy(pixData + count * (*it)->length, (*it)->pixData, (*it)->length);
        it++;
    }
    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeBinaryFrames(Uint8* pixData, Uint16 rows, Uint16 cols, const size_t pixDataLength)
{
    // Holds the pixels for all frames. Each bit represents a pixel which is either
    // 1 (part of segment) or 0 (not part of segment. All frames are directly
    // concatenated, i.e. there are no unused bits between the frames.
    memset(pixData, 0, pixDataLength);

    // Fill Pixel Data Element
    concatFrames(m_Frames, pixData, rows * cols);
    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::writeSegmentationImageModule(DcmItem& dataset)
{
    dataset.putAndInsertOFStringArray(DCM_ImageType, "DERIVED\\PRIMARY");

    OFCondition result = m_ContentIdentificationMacro.write(dataset);

    /* Write hardcoded values */
    if (result.good())
    {
        getImagePixel().setSamplesPerPixel(1);
        getImagePixel().setPhotometricInterpretation("MONOCHROME2");
        getImagePixel().setPixelRepresentation(0);

        /* Write Bits Allocated/Stored, High Bit, Segmentation Fractional Type,
         * Segmentation Type, Maximum Fractional Value
         */
        switch (m_SegmentationType)
        {
            case DcmSegTypes::ST_BINARY:
            {
                getImagePixel().setBitsAllocated(1);
                getImagePixel().setBitsStored(1);
                getImagePixel().setHighBit(0);
                dataset.putAndInsertOFStringArray(DCM_SegmentationType, "BINARY");
                break;
            }
            case DcmSegTypes::ST_FRACTIONAL:
            {
                getImagePixel().setBitsAllocated(8);
                getImagePixel().setBitsStored(8);
                getImagePixel().setHighBit(7);
                dataset.putAndInsertOFStringArray(DCM_SegmentationType, "FRACTIONAL");
                if (m_SegmentationFractionalType == DcmSegTypes::SFT_OCCUPANCY)
                {
                    dataset.putAndInsertOFStringArray(DCM_SegmentationFractionalType, "OCCUPANCY");
                }
                else
                {
                    dataset.putAndInsertOFStringArray(DCM_SegmentationFractionalType, "PROBABILITY");
                }
                // Maximum Fractional Value: Attribute is type 1C but "required if .. FRACTIONAL", i.e. write type 1
                DcmIODUtil::copyElementToDataset(
                    result, dataset, m_MaximumFractionalValue, "1", "1", "SegmentationImageModule");
                break;
            }
            case DcmSegTypes::ST_UNKNOWN:
            {
                DCMSEG_ERROR("Internal error, segmentation type not set");
                result = EC_InternalError;
                break;
            }
        }
    }

    /* Write segments */
    OFVector<DcmItem*> segmentItems;
    if (result.good())
    {
        typename OFVector<DcmSegment<BitsAlloc>*>::iterator it = m_Segments.begin();
        dataset.findAndDeleteElement(DCM_SegmentSequence);
        for (Uint16 itemCount = 0; (it != m_Segments.end()) && result.good(); itemCount++)
        {
            DcmItem* segmentItem = NULL;
            dataset.findOrCreateSequenceItem(DCM_SegmentSequence, segmentItem, itemCount);
            if (segmentItem)
            {
                result = (*it)->write(*segmentItem);
                /* Insert segment number for the segment, starting from 1 and increasing monotonically. */
                if (result.good())
                {
                    segmentItem->putAndInsertUint16(DCM_SegmentNumber, itemCount + 1);
                }
            }
            else
            {
                DCMIOD_ERROR("Cannot create/get item in Segment Sequence (internal error)");
                result = EC_InternalError;
            }
            it++;
        }
    }

    return result;
}

// -- private helpers --

template <typename BitsAlloc>
void DcmSegmentation<BitsAlloc>::clearData()
{
    DcmSegmentation<BitsAlloc>::IODImage::clearData();
    m_FG.clearData();
    m_FGInterface.clear();
    DcmIODUtil::freeContainer(m_Frames);
    DcmIODUtil::freeContainer(m_Segments);
    m_MaximumFractionalValue.clear();
    m_SegmentationFractionalType = DcmSegTypes::SFT_UNKNOWN;
    m_SegmentationType           = DcmSegTypes::ST_UNKNOWN;
}

template <typename BitsAlloc>
OFBool DcmSegmentation<BitsAlloc>::checkPixDataLength(DcmElement* pixelData,
                                           const Uint16 rows,
                                           const Uint16 cols,
                                           const Uint32& numberOfFrames)
{
    // Get actual length of pixel data in bytes
    size_t length = pixelData->getLengthField();

    // Find out how many bytes are needed
    size_t bytesRequired = 0;
    OFCondition result   = getTotalBytesRequired(rows, cols, numberOfFrames, bytesRequired);
    if (result.bad())
        return OFFalse;
    // Length found in Pixel Data element is always even
    if (bytesRequired % 2 == 1)
        bytesRequired++;
    /* Compare expected and actual length */
    if (length < bytesRequired)
    {
        DCMSEG_ERROR("Not enough bytes found in Pixel Data element. Found " << length << " bytes but " << bytesRequired
                                                                            << " bytes expected");
        return OFFalse;
    }
    else if (length > bytesRequired)
    {
        DCMSEG_WARN("Too many bytes found in Pixel Data element. Found " << length << " bytes but " << bytesRequired
                                                                         << " bytes expected");
        return OFTrue;
    }
    else
    {
        DCMSEG_TRACE("Found " << length << " bytes in Pixel Data element as expected");
    }
    return OFTrue;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::getTotalBytesRequired(const Uint16& rows,
                                                   const Uint16& cols,
                                                   const Uint32& numberOfFrames,
                                                   size_t& bytesRequired)
{
    OFBool ok = OFStandard::safeMult(OFstatic_cast(size_t, rows), OFstatic_cast(size_t, cols), bytesRequired);
    if (ok)
        OFStandard::safeMult(bytesRequired, OFstatic_cast(size_t, numberOfFrames), bytesRequired);
    if (!ok)
    {
        DCMSEG_ERROR("Cannot compute number of bytes required for Pixel Data since size_t type is too small");
        return EC_TooManyBytesRequested;
    }

    /* for binary, we only need one bit per pixel */
    size_t remainder = 0;
    if (m_SegmentationType == DcmSegTypes::ST_BINARY)
    {
        // check whether the 1-bit pixels exactly fit into bytes
        remainder = bytesRequired % 8;
        // number of bytes that work on an exact fit
        bytesRequired = bytesRequired / 8;
        // add one byte if we have a remainder
        if (remainder > 0)
        {
            bytesRequired++;
        }
    }
    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::loadFile(DcmFileFormat& dcmff, const OFString& filename, DcmDataset*& dset)
{
    dset               = NULL;
    OFCondition result = dcmff.loadFile(filename.c_str());
    if (result.bad())
    {
        DCMSEG_ERROR("Could not load file " << filename << ": " << result.text());
        return result;
    }
    dset = dcmff.getDataset();
    if (dset == NULL)
    {
        DCMSEG_ERROR("Could not load file " << filename << ": No dataset");
        return IOD_EC_InvalidObject;
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::readSegmentationFractionalType(DcmItem& item)
{
    m_SegmentationFractionalType = DcmSegTypes::SFT_UNKNOWN;
    if (!item.tagExists(DCM_SegmentationFractionalType))
    {
        return EC_TagNotFound;
    }
    DcmCodeString element(DCM_SegmentationFractionalType);
    OFCondition result = DcmIODUtil::getAndCheckElementFromDataset(
        item, element, typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->getByTag(DCM_SegmentationFractionalType));
    OFString str;
    if (result.good())
    {
        element.getOFStringArray(str);
        m_SegmentationFractionalType = DcmSegTypes::OFString2FractionalType(str);
    }

    if (m_SegmentationFractionalType == DcmSegTypes::SFT_UNKNOWN)
    {
        DCMSEG_ERROR("Invalid value for attribute Segmentation Fractional Type: " << str);
        return EC_InvalidValue;
    }
    else
        return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::readSegmentationType(DcmItem& item)
{
    m_SegmentationType = DcmSegTypes::ST_UNKNOWN;
    if (!item.tagExists(DCM_SegmentationType))
    {
        return EC_TagNotFound;
    }

    DcmCodeString element(DCM_SegmentationType);
    OFCondition result
        = DcmIODUtil::getAndCheckElementFromDataset(item, element, typename DcmSegmentation<BitsAlloc>::IODImage().getRules()->getByTag(DCM_SegmentationType));
    OFString str;
    if (result.good())
    {
        element.getOFStringArray(str);
        m_SegmentationType = DcmSegTypes::OFString2Segtype(str);
    }

    if (m_SegmentationType == DcmSegTypes::ST_UNKNOWN)
    {
        DCMSEG_ERROR("Invalid value for attribute Segmentation Type: " << str);
        result = EC_InvalidValue;
    }

    return result;
}

// protected override of public base class function

template <typename BitsAlloc>
IODImagePixelModule<Uint8>& DcmSegmentation<BitsAlloc>::getImagePixel()
{
    return *OFget<IODImagePixelModule<Uint8> >(&DcmSegmentation<BitsAlloc>::IODImage::getImagePixel());
}

template <typename BitsAlloc>
OFBool DcmSegmentation<BitsAlloc>::check(const OFBool checkFGStructure)
{
    if (m_Frames.size() == 0)
    {
        DCMSEG_ERROR("No frame data available");
        return OFFalse;
    }
    if (m_Segments.size() == 0)
    {
        DCMSEG_ERROR("No segments defined");
        return OFFalse;
    }
    if (m_Segments.size() > DCM_SEG_MAX_SEGMENTS)
    {
        DCMSEG_ERROR("Too many segments defined");
        return OFFalse;
    }
    if (m_Segments.size() > m_Frames.size())
    {
        DCMSEG_ERROR("More segments than frames defined");
        return OFFalse;
    }

    if (checkFGStructure)
    {
        if (!m_FGInterface.check())
            return OFFalse;
    }

    // Check rules around Frame of Reference

    // 1. If Derivation Image FG is not present, Frame of Reference is required.
    OFBool frameOfRefRequired = OFFalse;
    FGBase* group             = m_FGInterface.get(OFstatic_cast(Uint32, 0), DcmFGTypes::EFG_DERIVATIONIMAGE);
    if (group == NULL)
    {
        // Derivation Image FG is not present, FoR is required
        frameOfRefRequired = OFTrue;
    }
    else
    {
        // Derivation Image FG present, Frame of Reference is not required
        frameOfRefRequired = OFFalse;
    }
    OFString frameOfRef;
    typename DcmSegmentation<BitsAlloc>::IODImage().getFrameOfReference().getFrameOfReferenceUID(frameOfRef);
    if (frameOfRefRequired && frameOfRef.empty())
    {
        DCMSEG_ERROR("Frame of Reference UID is not set for Segmentation but is required");
        return OFFalse;
    }

    // 2. When a Frame of Reference UID is present the segment shall be specified
    // within that coordinate system, using the Pixel Measures, Plane Position
    // (Patient) and Plane Orientation (Patient) Functional Groups.
    if (!frameOfRef.empty())
    {
        // Check that each of above FGs is present. We do not check this for
        // all frames since if it exists for one frame it must exist for all others.
        // This is a general rule and applies for all FGs, so it is not checked here.
        group = m_FGInterface.get(OFstatic_cast(Uint32, 0), DcmFGTypes::EFG_PIXELMEASURES);
        if (!group)
        {
            DCMSEG_ERROR("Frame of Reference UID is present but Pixel Measures FG is missing");
            return OFFalse;
        }
        group = m_FGInterface.get(OFstatic_cast(Uint32, 0), DcmFGTypes::EFG_PLANEPOSPATIENT);
        if (!group)
        {
            DCMSEG_ERROR("Frame of Reference UID is present but Plane Position (Patient) FG is missing");
            return OFFalse;
        }
        group = m_FGInterface.get(OFstatic_cast(Uint32, 0), DcmFGTypes::EFG_PLANEORIENTPATIENT);
        if (!group)
        {
            DCMSEG_ERROR("Frame of Reference UID is present but Plane Orientation (Patient) FG is missing");
            return OFFalse;
        }
    }
    // Another condition cannot be checked since we do not have access to the
    // datasets of the source images:
    // 3. If FoR is present but not the same in images this segmentation applies to,
    // (those in Derivation Image FG), each pixel of the segmentation shall
    // correspond to a pixel in a referenced image (i.e. they must share the same
    // size and resolution).

    return OFTrue;
}

template <typename BitsAlloc>
OFCondition DcmSegmentation<BitsAlloc>::decompress(DcmDataset& dset)
{
    DcmXfer xfer = dset.getOriginalXfer();
    OFCondition result;
    // If the original transfer syntax could have been lossy, print warning
    if (dset.hasRepresentation(EXS_LittleEndianExplicit, NULL))
    {
        if (xfer.isEncapsulated() && (xfer != EXS_RLELossless) && (xfer != EXS_DeflatedLittleEndianExplicit))
        {
            DCMSEG_WARN("Dataset has been compressed using a (possibly) lossy compression scheme (ignored)");
        }
    }
    // If the original transfer is encapsulated and we do not already have an uncompressed version, decompress or reject
    // the file
    else if (xfer.isEncapsulated())
    {
        // RLE compression is fine (truly lossless). Deflated is handled internally by DCMTK.
        if (xfer == EXS_RLELossless)
        {
            DCMSEG_DEBUG("DICOM file is RLE-compressed, converting to uncompressed transfer syntax first");
            result = DcmIODUtil::decompress(dset);
        }
        else // We do not accept any transfer syntax that could be lossy compressed
        {
            DCMSEG_ERROR("Transfer syntax " << DcmXfer(xfer).getXferName()
                                            << " uses lossy compression, not supported for Segmentation objects!");
            result = IOD_EC_CannotDecompress;
        }
    }
    return result;
}

template <typename BitsAlloc>
void DcmSegmentation<BitsAlloc>::concatFrames(OFVector<DcmIODTypes::Frame*> frames, Uint8* pixData, const size_t bitsPerFrame)
{
    // Writing position within the pixData memory
    Uint8* writePos                               = pixData;
    OFVector<DcmIODTypes::Frame*>::iterator frame = frames.begin();
    Uint8 freeBits                                = 0;
    Uint8 firstByte                               = 0;
    // Iterate over frames and copy each to pixData memory
    for (size_t f = 0; frame != frames.end(); f++)
    {
        DCMSEG_DEBUG("Packing segmentation frame #" << f + 1 << "/" << frames.size());
        // Backup first byte of the destination since it may contain bits of the
        // previous frame; mask out those bits not belonging to previous frame.
        // This will potentially create some empty bits on the left of the byte,
        // that the current frame can use to store the its own first bits.
        firstByte = OFstatic_cast(unsigned char, (writePos[0] << freeBits)) >> freeBits;
        memcpy(writePos, (*frame)->pixData, (*frame)->length);
        // If the previous frame left over some unused bits, shift the current frame
        // that number of bits to the left, and restore the original bits of the
        // previous frame that are overwritten by the shifting operation.
        if (freeBits > 0)
        {
            DcmSegUtils::alignFrameOnBitPosition(writePos, (*frame)->length, 8 - freeBits);
            writePos[0] |= firstByte;
        }
        // Compute free bits left over from this frame in the previous byte written
        freeBits = (8 - (((f + 1) * bitsPerFrame) % 8)) % 8;
        // If we have free bits, the previous byte written to will be the first byte
        // we write to for the next frame. Otherwise start with a fresh destination
        // byte.
        if (freeBits > 0)
        {
            writePos = writePos + (*frame)->length - 1;
        }
        else
        {
            writePos = writePos + (*frame)->length;
        }
        // Next frame
        frame++;
    }
    // Through shifting we can have non-zero bits within the unused bits of the
    // last byte. Fill them with zeros (though not required by the standard).
    if (freeBits > 0)
    {
        *writePos = (OFstatic_cast(unsigned char, *writePos) >> freeBits) << freeBits;
    }
}
