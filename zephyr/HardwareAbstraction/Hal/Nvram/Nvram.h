//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//************************************************************************


/** @file NVRAM.h
    @brief Header file contains definitions used by NVRAM Environment.
    
**/

//---------------------------------------------------------------------------

#ifndef NVRAM_H
#define NVRAM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "Common.h"
#include "Include.h"
#include "SPIConfiguration.h"

#define TRACE printf

#pragma pack (1)
#define NVRAM_SIGNATURE "$NFNV"
#define NVRAM_READONLY_TYPE 0x04
#define NVRAM_READWRITE_TYPE 0x05
#define NVRAM_GUID	{0xc334b5af, 0x5f4a, 0x4779, {0xac, 0x57, 0xc9, 0x43, 0xf3, 0x4a, 0xde, 0x12}}
#define FLAG_INVALID 0x44
#define FLAG_VALID 0x55

#define SET_DIFF_SIZE 0x2

///
/// EFI_GUID .
/// This structure indicates the GUID data format.
///
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;
///
/// NVRAM_HEADER Contains the Header data of NVRAM.
/// This structure indicates the intialization of NVRAM.
///
typedef struct {
    EFI_GUID Header;
    uint32_t NvramAddress;
    uint32_t NvramSize;
    uint32_t HeaderLength;
    uint32_t *FirstVar;
    uint32_t *EndVar;
    uint32_t DataStoreSize;
    uint8_t CheckSum;
} NVRAM_HEADER;

///
/// NVRAM_VARIABLE is used for holding Variable data.
/// Where the data will be stored in a sequence based on dynamic length.
///
typedef struct {
    char signature[5];
    uint32_t VariableNameSize;
    uint8_t VariableAttribute;
    uint32_t VariableLength;
    uint8_t Flag;
    //char *VariableName;
    //uint8_t *Data;
    //uint8_t CheckSum;    
}NVRAM_VARIABLE;

typedef struct {
    uint32_t VariableLength;
    uint8_t VariableAttribute;
    uint8_t VariableData;
    char *VariableName;
}NVRAM_PREDEFINED_DATA;

#pragma pack()
extern uint8_t         gClearNvram;

/**
  @internal
  Function to initialize NVRAMGuid and header to SPI0.
  **/
RETURN_STATUS NvramInit();

/**
  @internal
  Function to get the NvramStore from SPI0.
  
  @param IN OUT 	uint8_t *NvramStore - Buffer though which NVRAm data will be updated.
  @param IN			uint8_t Position    - Position in NvStore to read from.
**/
RETURN_STATUS GetNvramStore(
        uint8_t     *NvramStore, 
        uint32_t    Position);

/**
  @internal
  Function to get the NvramVarible from NVRAM store..
  
  @param IN         Char *VaribleName - Variable Name to which data will be update.
  @param IN OUT     uint8_t *variableAttribute - Variable attribute type.
  @param IN OUT     uint32_t *VariableSize - Size of the Variable.
  @param OUT        uint8_t* Data - Data to be returned.
**/
RETURN_STATUS GetVariableData (
        char        *VariableName, 
        uint8_t     *VariableAttribute, 
        uint32_t    *VariableSize, 
        uint8_t     *Data);

/**
  @internal
  Function to Set the NvramVarible to NVRAM store..
  
  @param IN  		Char *VaribleName - Variable Name to which data will be updated.
  @param IN  		uint8_t variableAttribute - Variable attribute type.
  @param IN  		uint32_t VariableSize - Size of the Variable.
  @param IN  		uint8_t* Data - Data to be returned.User is responsible to pass the valid pointer for VariableSize
**/
RETURN_STATUS SetVariableData (
        char        *VariableName, 
        uint8_t     VariableAttribute, 
        uint32_t    VariableSize, 
        uint8_t     *Data);

/**
  @internal
  Function to get next valid Variable.
  
  @param IN OUT 	NVRAM_VARIABLE **Variable - Variable pointer of NVRAm data..
  @param IN     	NVRAM_HEADER *Header - Header of NVRAM.
  @param IN     	uint32_t Position -Position of Variable.
  @param IN     	uint32_t ReadSize - Size read from NvStore.
**/
RETURN_STATUS GetNextVariable (
        NVRAM_VARIABLE  **Variable,
        NVRAM_HEADER    *Header,
        uint32_t        Position, 
        uint32_t        ReadSize);

/**
  @internal
  Function to check if NvramVarible is valid.
  
  @param IN NVRAM_VARIABLE *Variable - NVRAM variable pointer..
**/
RETURN_STATUS IsVariableValid(
        NVRAM_VARIABLE  *Variable);

/**
    @internal
    This function calculates the checksum of the given data

    @param data - pointer of data for checksum calculation
    @param length - size of the data for checksum calculation
**/
uint8_t CheckVariableIntegrity (
        uint8_t         *data,
        uint32_t        length);

/**
  @internal
  Function to Find the NvramVarible based on name.
  
  @param IN      Char *VaribleName          - Variable Name to which data will be updated.
  @param IN OUT  NVRAM_VARIABLE **variable  - NvramVarible pointer.
  @param IN      NVRAM_HEADER *Header       - NVRAM variable store details.
  @param IN OUT  uint32_t *Location         - Variable position from NVRAM.
  @param IN      uint32_t SizeRead          - Total size read from NvStore.
  @param IN OUT  uint32_t *Position         - position of NvramBuffer.
**/
RETURN_STATUS FindVariable(
        char            *VariableName,
        NVRAM_VARIABLE  **Variable,
        NVRAM_HEADER    *Header, 
        uint32_t        *Location, 
        uint32_t        ReadSize,
        uint32_t        *Positon);

/**
  @internal
  Function to set the NvramStore from SPI0.
  
  @param IN uint8_t    *NvramStore - Buffer though which NVRAm data will be updated.
  @param IN uint32_t   Location    - Position in NvramStore.
  @param IN uint32_t   Size        - Size to write.
**/
RETURN_STATUS SetNvramStore(
        uint8_t         *NvramStore, 
        uint32_t        Location, 
        uint32_t        Size);

/**
  @internal
  Function to remove invalid variables from NvramStore.
  **/
RETURN_STATUS BeginGarbageCollection();

/**
  @internal
  Function to calculate the NvramStore CheckSum.
**/
uint8_t CalculateNvramCheckSum();
uint8_t CheckVariableIntegrity (uint8_t *Data,uint32_t Length);

#ifdef  __cplusplus
}
#endif

#endif
