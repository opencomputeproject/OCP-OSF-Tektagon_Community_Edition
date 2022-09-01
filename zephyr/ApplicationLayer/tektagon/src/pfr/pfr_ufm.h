//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef PFR_UFM_H
#define PFR_UFM_H

int ufm_read(uint32_t ufm_id, uint32_t offset, uint8_t *data, uint32_t data_length);
int ufm_write(uint32_t ufm_id, uint32_t offset, uint8_t *data, uint32_t data_length);

#endif /*PFR_UFM_H*/