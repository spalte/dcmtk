/*
 *
 *  Copyright (C) 2015-2022, Open Connections GmbH
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
 *  Purpose: Class representing a Segment from the Segment Ident. Sequence
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"
#include "dcmtk/dcmseg/segtypes.h"

// Explicit template instantiation
template class DCMTK_DCMSEG_EXPORT DcmSegment<Uint8>;
template class DCMTK_DCMSEG_EXPORT DcmSegment<Uint16>;

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::create(DcmSegment*& segment,
                                          const OFString& segmentLabel,
                                          const CodeSequenceMacro& segmentedPropertyCategory,
                                          const CodeSequenceMacro& segmentedPropertyType,
                                          const DcmSegTypes::E_SegmentAlgoType algoType,
                                          const OFString& algoName)

{
    segment = new DcmSegment();
    if (segment == NULL)
        return EC_MemoryExhausted;

    OFCondition result = segment->setSegmentLabel(segmentLabel, OFTrue /* check value */);

    if (result.good())
    {
        segment->m_SegmentDescription.getSegmentedPropertyCategoryCode() = segmentedPropertyCategory;
        result = segment->m_SegmentDescription.getSegmentedPropertyCategoryCode().check();
    }

    if (result.good())
    {
        segment->m_SegmentDescription.getSegmentedPropertyTypeCode() = segmentedPropertyType;
        result                                                       = segment->getSegmentedPropertyTypeCode().check();
    }

    if (result.good())
    {
        result = segment->setSegmentAlgorithm(algoType, algoName, OFTrue);
    }

    if (result.bad())
    {
        delete segment;
        segment = NULL;
    }

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::read(DcmItem& item, const OFBool clearOldData)
{
    if (clearOldData)
        clearData();

    m_SegmentDescription.read(item);
    DcmIODUtil::getAndCheckElementFromDataset(item, m_SegmentAlgorithmName, m_Rules.getByTag(DCM_SegmentAlgorithmName));

    DcmIODUtil::readSingleItem<AlgorithmIdentificationMacro>(item,
                                                             DCM_SegmentationAlgorithmIdentificationSequence,
                                                             m_SegmentationAlgorithmIdentification,
                                                             "3",
                                                             "Segmentation Image Module");

    DcmIODUtil::getAndCheckElementFromDataset(
        item, m_RecommendedDisplayGrayscaleValue, m_Rules.getByTag(DCM_RecommendedDisplayGrayscaleValue));
    DcmIODUtil::getAndCheckElementFromDataset(
        item, m_RecommendedDisplayCIELabValue, m_Rules.getByTag(DCM_RecommendedDisplayCIELabValue));
    DcmIODUtil::getAndCheckElementFromDataset(item, m_TrackingID, m_Rules.getByTag(DCM_TrackingID));
    DcmIODUtil::getAndCheckElementFromDataset(item, m_TrackingUID, m_Rules.getByTag(DCM_TrackingUID));

    return EC_Normal;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::write(DcmItem& item)
{
    OFCondition result;
    result = m_SegmentDescription.write(item);
    DcmIODUtil::copyElementToDataset(result, item, m_SegmentAlgorithmName, m_Rules.getByTag(DCM_SegmentAlgorithmName));

    if (result.good() && m_SegmentationAlgorithmIdentification.check(OFTrue /* quiet */).good())
    {
        DcmIODUtil::writeSingleItem<AlgorithmIdentificationMacro>(result,
                                                                  DCM_SegmentationAlgorithmIdentificationSequence,
                                                                  m_SegmentationAlgorithmIdentification,
                                                                  item,
                                                                  "3",
                                                                  "Segmentation Image Module");
    }

    DcmIODUtil::copyElementToDataset(
        result, item, m_RecommendedDisplayGrayscaleValue, m_Rules.getByTag(DCM_RecommendedDisplayGrayscaleValue));
    DcmIODUtil::copyElementToDataset(
        result, item, m_RecommendedDisplayCIELabValue, m_Rules.getByTag(DCM_RecommendedDisplayCIELabValue));
    DcmIODUtil::copyElementToDataset(result, item, m_TrackingID, m_Rules.getByTag(DCM_TrackingID));
    DcmIODUtil::copyElementToDataset(result, item, m_TrackingUID, m_Rules.getByTag(DCM_TrackingUID));

    return result;
}

template <typename BitsAlloc>
void DcmSegment<BitsAlloc>::clearData()
{
    m_SegmentDescription.clearData();
    m_SegmentAlgorithmName.clear();
    m_SegmentationAlgorithmIdentification.clearData();
    m_RecommendedDisplayGrayscaleValue.clear();
    m_RecommendedDisplayCIELabValue.clear();
    m_TrackingID.clear();
    m_TrackingUID.clear();
}

template <typename BitsAlloc>
DcmSegment<BitsAlloc>::~DcmSegment()
{
    clearData();
}

// protected default constructor
template <typename BitsAlloc>
DcmSegment<BitsAlloc>::DcmSegment()
    : m_SegmentationDoc(NULL)
    , m_SegmentDescription()
    , m_SegmentAlgorithmName(DCM_SegmentAlgorithmName)
    , m_SegmentationAlgorithmIdentification()
    , m_RecommendedDisplayGrayscaleValue(DCM_RecommendedDisplayGrayscaleValue)
    , m_RecommendedDisplayCIELabValue(DCM_RecommendedDisplayCIELabValue)
    , m_TrackingID(DCM_TrackingID)
    , m_TrackingUID(DCM_TrackingUID)
    , m_Rules()
{
    initIODRules();
}

template <typename BitsAlloc>
void DcmSegment<BitsAlloc>::initIODRules()
{
    m_Rules.addRule(new IODRule(DCM_SegmentAlgorithmName, "1", "1C", "SegmentationImageModule", DcmIODTypes::IE_IMAGE),
                    OFTrue);
    m_Rules.addRule(
        new IODRule(DCM_RecommendedDisplayGrayscaleValue, "1", "3", "SegmentationImageModule", DcmIODTypes::IE_IMAGE),
        OFTrue);
    m_Rules.addRule(
        new IODRule(DCM_RecommendedDisplayCIELabValue, "3", "3", "SegmentationImageModule", DcmIODTypes::IE_IMAGE),
        OFTrue);
    m_Rules.addRule(new IODRule(DCM_TrackingID, "1", "1C", "SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
    m_Rules.addRule(new IODRule(DCM_TrackingUID, "1", "1C", "SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
}

// -------------- getters --------------------

template <typename BitsAlloc>
Uint16 DcmSegment<BitsAlloc>::getSegmentNumber()
{
    Uint16 value = 0;
    if (m_SegmentationDoc != NULL)
    {
        size_t big = 0;
        m_SegmentationDoc->getSegmentNumber(this, big);
        if (big <= DCM_SEG_MAX_SEGMENTS)
        {
            value = OFstatic_cast(Uint16, big);
        }
        else
        {
            DCMSEG_ERROR("More segments (" << big << ") defined than permitted (" << DCM_SEG_MAX_SEGMENTS << ")");
        }
    }
    return value;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getSegmentLabel(OFString& value, const signed long pos)
{
    return m_SegmentDescription.getSegmentLabel(value, pos);
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getSegmentDescription(OFString& value, const signed long pos)
{
    return m_SegmentDescription.getSegmentDescription(value, pos);
}

template <typename BitsAlloc>
DcmSegTypes::E_SegmentAlgoType DcmSegment<BitsAlloc>::getSegmentAlgorithmType()
{
    return m_SegmentDescription.getSegmentAlgorithmType();
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getSegmentAlgorithmName(OFString& value, const signed long pos)
{
    return DcmIODUtil::getStringValueFromElement(m_SegmentAlgorithmName, value, pos);
}

template <typename BitsAlloc>
GeneralAnatomyMacro& DcmSegment<BitsAlloc>::getGeneralAnatomyCode()
{
    return m_SegmentDescription.getGeneralAnatomyCode();
}

template <typename BitsAlloc>
AlgorithmIdentificationMacro& DcmSegment<BitsAlloc>::getSegmentationAlgorithmIdentification()
{
    return m_SegmentationAlgorithmIdentification;
}

template <typename BitsAlloc>
CodeSequenceMacro& DcmSegment<BitsAlloc>::getSegmentedPropertyCategoryCode()
{
    return m_SegmentDescription.getSegmentedPropertyCategoryCode();
}

template <typename BitsAlloc>
CodeSequenceMacro& DcmSegment<BitsAlloc>::getSegmentedPropertyTypeCode()
{
    return m_SegmentDescription.getSegmentedPropertyTypeCode();
}

template <typename BitsAlloc>
OFVector<CodeSequenceMacro*>& DcmSegment<BitsAlloc>::getSegmentedPropertyTypeModifierCode()
{
    return m_SegmentDescription.getSegmentedPropertyTypeModifier();
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getRecommendedDisplayGrayscaleValue(Uint16& value, const unsigned long pos)
{
    return m_RecommendedDisplayGrayscaleValue.getUint16(value, pos);
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getRecommendedDisplayCIELabValue(Uint16& L, Uint16& a, Uint16& b)
{
    OFCondition result = m_RecommendedDisplayCIELabValue.getUint16(L, 0);
    if (result.good())
        result = m_RecommendedDisplayCIELabValue.getUint16(a, 1);

    if (result.good())
        result = m_RecommendedDisplayCIELabValue.getUint16(b, 2);

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getTrackingID(OFString& value, const signed long pos)
{
    return DcmIODUtil::getStringValueFromElement(m_TrackingID, value, pos);
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::getTrackingUID(OFString& value, const signed long pos)
{
    return DcmIODUtil::getStringValueFromElement(m_TrackingUID, value, pos);
}

// -------------- setters --------------------

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setSegmentLabel(const OFString& value, const OFBool checkValue)
{
    return m_SegmentDescription.setSegmentLabel(value, checkValue);
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setSegmentDescription(const OFString& value, const OFBool checkValue)
{
    return m_SegmentDescription.setSegmentDescription(value, checkValue);
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setSegmentAlgorithm(const DcmSegTypes::E_SegmentAlgoType algoType,
                                                       const OFString& algoName,
                                                       const OFBool checkValue)
{
    if (checkValue && algoType == DcmSegTypes::SAT_UNKNOWN)
    {
        DCMSEG_ERROR("Algorithm type must be initialized to a valid value");
        return EC_InvalidValue;
    }

    OFCondition result;
    // Set algorithm name if algorithm type is not manual (otherwise do not set it at all)
    if (algoType != DcmSegTypes::SAT_MANUAL)
    {
        if (checkValue && algoName.empty())
        {
            DCMSEG_ERROR("Algorithm name must be provided if Algorithm Type is not 'MANUAL'");
            return EC_MissingValue;
        }
        if (!algoName.empty())
        {
            result = (checkValue) ? DcmLongString::checkStringValue(algoName, "1") : EC_Normal;
            if (result.good())
            {
                result = m_SegmentAlgorithmName.putOFStringArray(algoName);
            }
        }
    }
    // Set algorithm type
    if (result.good())
    {
        result = m_SegmentDescription.setSegmentAlgorithmType(algoType);
    }
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setSegmentationAlgorithmIdentification(const AlgorithmIdentificationMacro& value,
                                                                          const OFBool checkValue)
{
    m_SegmentationAlgorithmIdentification = value;
    OFCondition result;
    if (checkValue)
    {
        result = m_SegmentationAlgorithmIdentification.check();
    }

    if (result.bad())
    {
        m_SegmentationAlgorithmIdentification.clearData();
    }

    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setRecommendedDisplayGrayscaleValue(const Uint16 value, const OFBool)
{
    return m_RecommendedDisplayGrayscaleValue.putUint16(value, 0);
}

template <typename BitsAlloc>
OFCondition
DcmSegment<BitsAlloc>::setRecommendedDisplayCIELabValue(const Uint16 r, const Uint16 g, const Uint16 b, const OFBool)
{
    OFCondition result = m_RecommendedDisplayCIELabValue.putUint16(r, 0);
    if (result.good())
        result = m_RecommendedDisplayCIELabValue.putUint16(g, 1);
    if (result.good())
        result = m_RecommendedDisplayCIELabValue.putUint16(b, 2);
    return result;
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setTrackingID(const OFString& value, const OFBool checkValue)
{
    // avoid compile warning on unused variable
    (void)checkValue;
    return m_TrackingID.putOFStringArray(value);
}

template <typename BitsAlloc>
OFCondition DcmSegment<BitsAlloc>::setTrackingUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result;
    if (checkValue)
    {
        result = DcmUniqueIdentifier::checkStringValue(value, "1");
    }
    if (result.good())
    {
        result = m_TrackingUID.putOFStringArray(value);
    }
    return result;
}

template <typename BitsAlloc>
void DcmSegment<BitsAlloc>::referenceSegmentationDoc(DcmSegmentation<BitsAlloc>* doc)
{
    this->m_SegmentationDoc = doc;
}
