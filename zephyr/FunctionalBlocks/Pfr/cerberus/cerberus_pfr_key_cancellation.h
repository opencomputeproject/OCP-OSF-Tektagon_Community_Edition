//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef CERBERUS_PFR_KEY_CANCELLATION_H_
#define CERBERUS_PFR_KEY_CANCELLATION_H_

#if CONFIG_CERBERUS_PFR_SUPPORT

int cerberus_validate_key_cancellation_flag(struct pfr_manifest *manifest);
int cerberus_verify_csk_key_id(struct pfr_manifest *manifest, uint32_t key_id);
int cerberus_cancel_csk_key_id(struct pfr_manifest *manifest, uint32_t key_id);

#endif
#endif /*INTEL_PFR_KEY_CANCELLATION_H_*/
