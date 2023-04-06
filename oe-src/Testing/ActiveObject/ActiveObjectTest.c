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
#include "StateManager.h"
#include "ActiveObjectTest.h"

TEST_SUITE_LABEL ("ActiveObject_Layer");

/*******************
 * Test cases
 *******************/
 
static void ActiveObjectTestInit (CuTest *Test)
{
	TEST_START;
    StateManager();
	//CuAssertIntEquals (Test, 0, Status);
}


TEST_SUITE_START (ActiveObject_Layer);

TEST(ActiveObjectTestInit);

TEST_SUITE_END;
