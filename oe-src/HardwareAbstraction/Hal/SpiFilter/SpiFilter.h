// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Timer Handling functions
 */

#ifndef SPIFILTER_COMMON_H_
#define SPIFILTER_COMMON_H_

#include "spi_filter/spi_filter_interface.h"

#define SPI_FILTER_READ_PRIV 0
#define SPI_FILTER_WRITE_PRIV 1

#define SPI_FILTER_PRIV_ENABLE 0
#define SPI_FILTER_PRIV_DIABLE 1

struct SpiFilterEngine{
    struct spi_filter_interface base;
    int dev_id;
};

int SpiFilterInit(struct SpiFilterEngine *SpiFilter);

#endif /* SPIFILTER_H_ */
