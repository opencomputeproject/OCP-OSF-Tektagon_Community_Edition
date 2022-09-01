//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************


#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "ManifestProcessTest.h"

TEST_SUITE_LABEL ("ManifestProcessTest");

/*******************
 * Test cases
 *******************/
 
static void ManifestProcessTestInit (CuTest *Test)
{
	TEST_START;
	printf("ManifestProcessTestInit\n");
	//CuAssertIntEquals (Test, 0, Status);
}


TEST_SUITE_START (ManifestProcessTest);

TEST(ManifestProcessTestInit);

TEST_SUITE_END;
