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
#include "HardwareAbstractionTest.h"

TEST_SUITE_LABEL ("HardwareAbstraction_Layer");

/*******************
 * Test cases
 *******************/
 
static void HardwareAbstractionTestInit (CuTest *Test)
{
	TEST_START;
    HalConfiguration();
	//CuAssertIntEquals (Test, 0, Status);
}


TEST_SUITE_START (HardwareAbstraction_Layer);

TEST(HardwareAbstractionTestInit);

TEST_SUITE_END;
