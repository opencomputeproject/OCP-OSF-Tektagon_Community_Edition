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
#include "ApplicationTest.h"

TEST_SUITE_LABEL ("Application");

/*******************
 * Test cases
 *******************/
 
static void ApplicationTestInit (CuTest *Test)
{
	TEST_START;
    //ApplicationStart();
	//CuAssertIntEquals (Test, 0, Status);
}


TEST_SUITE_START (Application);

TEST(ApplicationTestInit);

TEST_SUITE_END;
