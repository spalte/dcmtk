/*
 *
 *  Copyright (C) 2019-2022, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
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
 *  Purpose: Test for creating, writing and reading Segmentation objects
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */

#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmseg/segment.h"
#include "dcmtk/ofstd/oftest.h"
#include "dcmtk/dcmdata/dcrledrg.h"  /* for DcmRLEDecoderRegistration */

#define LABELMAP_FILE "/home/michael/data/ownCloud/arbeit/projekte/bwh/labelmaps/objects/highdicom.dcm"

static OFString EXPECTED_DUMP("");

static void checkCreatedObject(const OFString& ds_dump);

OFTEST(dcmseg_loadlabelmap)
{
    // Make sure data dictionary is loaded
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    DcmRLEDecoderRegistration::registerCodecs();
    DcmSegmentation* seg = NULL;
    OFCondition result = DcmSegmentation::loadFile(LABELMAP_FILE, seg);
    OFCHECK(result.good());
}

static void checkCreatedObject(const OFString& ds_dump)
{
    OFBool dump_ok = (ds_dump == EXPECTED_DUMP);
    OFCHECK(dump_ok);
    if (!dump_ok)
    {
        CERR << "Dump produced: " << OFendl << ds_dump << OFendl;
        CERR << "------------------------------------" << OFendl;
        CERR << "Dump expected: " << OFendl << EXPECTED_DUMP << OFendl;
        CERR << "------------------------------------" << OFendl;
    }
}
