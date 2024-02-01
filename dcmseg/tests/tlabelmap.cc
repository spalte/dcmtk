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

#include "dcmtk/dcmdata/dcxfer.h"
#include "dcmtk/dcmseg/segdoc.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/oftest.h"
#include "dcmtk/ofstd/offile.h"
#include "dcmtk/dcmdata/dcrledrg.h"  /* for DcmRLEDecoderRegistration */
#include "dcmtk/dcmdata/dcrleerg.h"  /* for DcmRLEEncoderRegistration */


#define LABELMAP_FILE "/home/michael/data/ownCloud/arbeit/projekte/bwh/labelmaps/objects/highdicom_normalized.dcm"

static OFString EXPECTED_DUMP("");

static void checkCreatedObject(const OFString& ds_dump);

OFTEST(dcmseg_labelmaproundtrip)
{
    // Make sure data dictionary is loaded
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    DcmRLEDecoderRegistration::registerCodecs();
    DcmRLEEncoderRegistration::registerCodecs();
    DcmSegmentation* seg = NULL;
    DcmFileFormat dcmff;
    OFCondition result = dcmff.loadFile(LABELMAP_FILE);
    OFCHECK_MSG(result.good(), "Could not load labelmap file into file format object");
    if (result.good())
    {
        // Save dump to string for later comparison
        OFOStringStream oss;
        dcmff.getDataset()->print(oss);
        EXPECTED_DUMP = oss.str().c_str();
    }
    result = DcmSegmentation::loadFile(LABELMAP_FILE, seg);
    OFCHECK_MSG(result.good(), "Could not load labelmap file into segmentation object");
    if (result.good())
    {
        OFString ds_dump;
        OFOStringStream oss;
        DcmDataset ds;
        result = seg->writeDataset(ds);
        result = seg->saveFile( "/tmp/out.dcm", EXS_RLELossless);
        OFCHECK(ds.chooseRepresentation(EXS_RLELossless, NULL).good());
        OFCHECK_MSG(result.good(), "Could not write segmentation object to dataset");
        ds.print(oss);
        ds_dump = oss.str().c_str();
        checkCreatedObject(ds_dump);
    }
}

static void checkCreatedObject(const OFString& ds_dump)
{
    OFBool dump_ok = (ds_dump == EXPECTED_DUMP);
    OFCHECK(dump_ok);
    if (!dump_ok)
    {
        // CERR << "Dump produced: " << OFendl << ds_dump << OFendl;
        // CERR << "------------------------------------" << OFendl;
        // CERR << "Dump expected: " << OFendl << EXPECTED_DUMP << OFendl;
        // CERR << "------------------------------------" << OFendl;
        // Also write to text files for later analysis
        OFString dump_file = "/tmp/dump_created.txt";
        OFString expected_file = "/tmp/dump_expected.txt";
        // use C file api
        FILE* f = fopen(dump_file.c_str(), "w");
        if (f)
        {
            fputs(ds_dump.c_str(), f);
            fclose(f);
        }
        f = fopen(expected_file.c_str(), "w");
        if (f)
        {
            fputs(EXPECTED_DUMP.c_str(), f);
            fclose(f);
        }

        OFConsole::instance().lockCerr() << "Stored dump files to " << dump_file << " and " << expected_file << OFendl;

    }
}
