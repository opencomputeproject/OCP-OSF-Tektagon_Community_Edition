//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef PFR_VERIFICATION_H
#define PFR_VERIFICATION_H

// Add Active and Recovery verifcation
// Read address from UFM for active
// Set AoData
int authentication_image(void *AoData, void *EventContext);

// -- Active Region
int ActivePfmVerification(unsigned int ImageType,unsigned int ReadAddress);
int PfmSpiRegionVerification(unsigned int ImageId, unsigned int FlashSelect);

// -- Recovery Region
int RecoveryRegionVerification(int ImageType);

#endif /* PFR_VERIFICATION_H */