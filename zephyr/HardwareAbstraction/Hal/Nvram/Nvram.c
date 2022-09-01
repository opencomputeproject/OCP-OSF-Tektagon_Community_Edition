//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************

/** @file Nvram.c
    @brief This file contains Nvram initialization and features to control
    Nvram variables data by getting and setting.
**/

//---------------------------------------------------------------------------

#include "Include.h"
#include "Nvram.h"
#include <string.h>

/**
  Function to Get read and write count based on port type
  @param  PortType: type of the port
  @param  ReadCount: (out)based on port type read count will be updated.
  @return Nil
**/
RETURN_STATUS GetReadCount(int PortType, uint32_t *ReadCount)
{
   
	return GetReadCountWrapper(PortType, ReadCount);
}

/**
  @internal
  Function to get the NvramStore from SPI0.
  
  @param uint8_t *NvramStore - Buffer though which NVRAm data will be updated.
  @param uint8_t Position    - Position in NvStore to read from.
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_INVALID_PARAMETER if the SpiConfig is wrong.
          RETURN_NVRAM_EMPTY is reaing NVRAM data failed.
  @endinternal
**/
RETURN_STATUS GetNvramStore(uint8_t *NvramStore, uint32_t Position)
{
    return GetNvramStoreWrapper(NvramStore, Position);
}

/**
  @internal
  Function to get the NvramVarible from NVRAM store..
  
  @param IN         Char 		*VaribleName - Variable Name to which data will be update.
  @param OUT        uint8_t 	*variableAttribute - Variable attribute type.
  @param IN OUT     uint32_t 	*VariableSize - Size of the Variable.
  @param OUT        uint8_t 	*Data - Data to be returned.
  
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_UNSUPPORTED if the NVRAM region not found.
          RETURN_NVRAMSTORE_EMPTY if the Data store is empty.
          RETURN_INVALID_NVRAM_VARIABLE if the NVRAM signature is not matching.
          RETURN_ABORTED if the Variable's integrity is false.
  @endinternal
**/
RETURN_STATUS GetVariableData (char *VariableName, uint8_t *VariableAttribute, uint32_t *VariableSize, uint8_t* Data)
{

    return GetVariableDataWrapper(VariableName,VariableAttribute,VariableSize, Data);
}

/**
  @internal
  Function to Set the NvramVarible to NVRAM store..
  
  @param IN  Char *VaribleName - Variable Name to which data will be updated.
  @param IN  uint8_t variableAttribute - Variable attribute type.
  @param IN  uint32_t VariableSize - Size of the Variable.
  @param IN  uint8_t *Data - Data to be returned, ser is responsible to pass the valid pointer for VariableSize
  
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_UNSUPPORTED if the NVRAM region not found.
          RETURN_NVRAMSTORE_EMPTY if the Data store is empty.
          RETURN_INVALID_NVRAM_VARIABLE if the NVRAM signature is not matching.
          RETURN_ABORTED if the Variable's integrity is false.
  @endinternal
**/
RETURN_STATUS SetVariableData (char *VariableName, uint8_t VariableAttribute, uint32_t VariableSize, uint8_t* Data)
{
	return SetVariableDataWrapper (VariableName, VariableAttribute,VariableSize, Data);
}

/**
  @internal
  Function to get next valid Variable.
  
   @param IN OUT 	NVRAM_VARIABLE **Variable - Variable pointer of NVRAm data..
  @param IN     	NVRAM_HEADER *Header - Header of NVRAM.
  @param IN     	uint32_t Position -Position of Variable.
  @param IN     	uint32_t ReadSize - Size read from NvStore.
  
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_INVALID_NVRAM_VARIABLE if the NVRAM signature is not matching.
          RETURN_ABORTED if data store is full.
  @endinternal
**/
RETURN_STATUS GetNextVariable (NVRAM_VARIABLE **Variable,NVRAM_HEADER *Header,uint32_t Position, uint32_t ReadSize)
{
   return GetNextVariableWrapper(Variable, Header, Position, ReadSize);
}

/**
  @internal
  Function to check if NvramVarible is valid.
  
  @param IN NVRAM_VARIABLE *Variable - NVRAM variable pointer..
  
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_ABORTED if the Variable signature mismatching.
  @endinternal
**/
RETURN_STATUS IsVariableValid(NVRAM_VARIABLE *Variable)
{
	return IsVariableValidWrapper(Variable);
}


/**
  @internal
  Function to Find the NvramVarible based on name.
  
  @param IN      Char *VaribleName          - Variable Name to which data will be updated.
  @param IN OUT  NVRAM_VARIABLE **variable  - NvramVarible pointer.
  @param IN      NVRAM_HEADER *Header       - NVRAM variable store details.
  @param IN OUT  uint32_t *Location         - position of NvStore.
  @param IN      uint32_t SizeRead          - Total size read from NvStore.
  @param IN OUT  uint32_t *Position         - position of NvramBuffer.
  
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_NOT_FOUND if the NVRAM data is not found.

  @endinternal
**/
RETURN_STATUS FindVariable(char *VariableName, NVRAM_VARIABLE **Variable, NVRAM_HEADER *Header, uint32_t *Location, uint32_t ReadSize,uint32_t *Position)
{

	return 0;//FindVariabeWrapper(VariableName,Variable,Header,Location,ReadSize,Position);
}

/**
  @internal
  Function to set the NvramStore from SPI0.
  
  @param uint8_t    *NvramStore - Buffer though which NVRAm data will be updated.
  @param uint32_t   Location    - Position in NvramStore.
  @param uint32_t   Size        - Size to write.
  
  @retval RETURN_SUCCESS if function works successfully,
          RETURN_ABORTED if the SPI write failed.
          RETURN_NVRAM_EMPTY is reading NVRAM data failed.
          RETURN_BUFFER_EMPTY if buffer is NULL.
  @endinternal
**/
RETURN_STATUS SetNvramStore(uint8_t *NvramStore, uint32_t Location, uint32_t Size)
{
    return SetNvramStoreWrapper(NvramStore, Location, Size);
}


/**
  @internal
  Function to calculate the NvramStore CheckSum and set in header.
  
  @param Nil.
  
  @retval uint8_t returns Checksum of whole NVRAM.
  @endinternal
**/
uint8_t CalculateNvramCheckSum()
{

   return CalculateNvramCheckSumWrapper();
}

/**
  @internal
  Function to remove invalid variables from NvramStore.
  
  @param Nil.
  
  @retval 
        RETURN_SUCCESS - function works properly.
  @endinternal
**/
RETURN_STATUS BeginGarbageCollection()
{
      return BeginGarbageCollectionWrapper();
}

/**
    @internal
    This function calculates the checksum of the given data

    @param Data - pointer of data for checksum calculation
    @param Length - size of the data for checksum calculation

    @retval Crc - checksum value
    @endinternal

**/
uint8_t CheckVariableIntegrity (uint8_t *Data,uint32_t Length)
{
   uint8_t Crc = 0x00;
   uint8_t Extract = 0;
   uint8_t Sum = 0;
   uint32_t i = 0;
   uint8_t j = 0;
   
   for(i=0;i<Length;i++)
   {
      Extract = *Data;
      for ( j = 8; j; j--) 
      {
         Sum = (Crc ^ Extract) & 0x01;
         Crc >>= 1;
         if (Sum)
            Crc ^= 0x8C;
         Extract >>= 1;
      }
      Data++;
   }
   return Crc;
}


/**
  @internal
  Function to initialize NVRAMGuid and header to SPI0.
  It will verify the checksum and do garbage collection.
  
  @param Nil
  
  @retval Nil
  @endinternal
**/
RETURN_STATUS NvramInit()
{
    NVRAM_HEADER    HeaderData = {0};
    uint8_t         NVRAMBuffer[4096] = {0};
    EFI_GUID        NvramGuid = NVRAM_GUID;
    RETURN_STATUS   Status = RETURN_SUCCESS;
    
    Status = GetNvramStore(&NVRAMBuffer[0], 0);
    if(Status != RETURN_SUCCESS)
        return RETURN_OUT_OF_RESOURCES;
    memcpy(&HeaderData,&NVRAMBuffer[0],sizeof(HeaderData));
    //verifies NVRAM Guid matches properly.
    if (memcmp(&HeaderData.Header,&NvramGuid,sizeof(EFI_GUID)) != 0){
        HeaderData.Header =  NvramGuid;
        HeaderData.NvramAddress = 0;//NVRAM_ADDRESS;
        HeaderData.NvramSize = 0;//NVRAM_SIZE;
        HeaderData.HeaderLength = sizeof(HeaderData);
        HeaderData.FirstVar = (uint32_t*)&NVRAMBuffer[sizeof(HeaderData)];
        HeaderData.EndVar = (uint32_t*)&NVRAMBuffer[sizeof(HeaderData)];
        HeaderData.DataStoreSize = 0;
        memcpy(&NVRAMBuffer[0],&HeaderData,sizeof(HeaderData));
        HeaderData.CheckSum = CheckVariableIntegrity(&NVRAMBuffer[0],sizeof(HeaderData)-sizeof(HeaderData.CheckSum));
        memcpy(&NVRAMBuffer[0],&HeaderData,sizeof(HeaderData));
        Status = SetNvramStore(&NVRAMBuffer[0], 0 ,sizeof(HeaderData));
        if(Status == RETURN_SUCCESS)
            TRACE("NVRAM Initialization done\r\n");
        else
            TRACE("NVRAM Initialization failed\r\n");
    } else {
		TRACE("NVRAM Initialization done\r\n");
        if(HeaderData.CheckSum != CalculateNvramCheckSum()){
            TRACE("NVRAM corrupted\r\n");
            return RETURN_ABORTED;
        }
        Status = BeginGarbageCollection();
        if(Status == RETURN_SUCCESS)
            TRACE("\r\n");
    }
    return Status;
}
