//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef INTEL_PFR_KEY_CANCELLATION_H_
#define INTEL_PFR_KEY_CANCELLATION_H_


int validate_key_cancellation_flag(struct pfr_manifest *manifest);
int verify_csk_key_id(struct pfr_manifest *manifest, uint32_t key_id);
int cancel_csk_key_id(struct pfr_manifest *manifest, uint32_t key_id);

#endif /*INTEL_PFR_KEY_CANCELLATION_H_*/
