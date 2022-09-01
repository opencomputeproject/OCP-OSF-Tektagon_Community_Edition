# Hardware Abstraction

Hardware Abstraction will process the

- Hal
- Qpc

## Hardware Abstraction Layer (HAL)

HAL layer will provide the interface between the Application Layer and Chipset.

A hardware abstraction layer (HAL) is a logical division of code that serves as an abstraction layer between
a computer's physical hardware and its software.

### GPIO

- GPIO interface is designed to interact with various platform signals in order to control the power/reset lines
of various components (like CPU, PCH, BMC) to achieve the quiesced pre-boot stage.
- The GPIO interface controls other reset lines to components which misconfigured could prevent platform boot.
- The Common Core GPIO interface will interact between HROT and Chipset.

### SPI Control

- SPI Controller is designed to provide the CPLD RoT design with direct access to the various platform flash storage.
Access to flash storage is used for authentication and recovery for all platform firmware.

### Smbus

- Smbus is two-wire interface often used for system management communication between Application and Chipset.

### Crypto

- Cryptographic Authentication of FW - Platform FW components perform cryptographic authentication to verify the signature
of the firmware at time of launch/consumption regardless of the verification performed by Platform Root-of-Trust.

### Timer

- Timer is a basic hardware block which provides an monotonically incrementing time value which is required by
the CPLD RoT design flows to monitor the boot progress of the platform.

### Interrupt

- Interrupt Handling on GPIO/SMBus/Timer

### UDS/CDI for DICE

- The Trusted Computing Group (TCG) specifies Hardware Requirements for a Device Identifier Composition Engine (DICE).
- Unique Device Secret (UDS) and a Compound Device Identifier (CDI) are used as defined in the TCG DICE specification.

### Spi Filter

- SPI Filter provides runtime security against malicious or erroneous traffic driven from a different platform component.
- The main purpose of the SPI Filter is to monitor all traffic driven to platform SPI flash and intercept any transactions
not allowed by the set of rules programmed by the CPLD RoT firmware.

### Smbus Filter

- SPI Filter provides runtime security against malicious or erroneous traffic driven from a different platform component.
- The main purpose of the SPI Filter is to monitor all traffic driven to platform SPI flash and intercept any transactions not allowed by the set of
rules programmed by the CPLD RoT firmware.

## Quantum Platform in C (QPC)

​		The QPC provides a modern, reusable architecture of embedded applications, which combines object-orientation with the particular model of concurrency, known as active objects.

Qpc opensource will be used as library

- Source
- Ports
- Includes


### Structural Definitions / Flow

Table to explain source files for HAL

| File              | Description                                        |
|-------------------|----------------------------------------------------|
| Gpio.c            | GIPO related functions                             |
| Gpio.h            | GIPO related macros and function declations        |
| Smbus.c           | Smbus related functions                            |
| Smbus.h           | Smbus related macros and function declations       |
| I2c.c             | I2c related functions                              |
| I2c.h             | I2c related macros and function declations         |
| Spi.c             | Spi related functions                              |
| Spi.h             | Spi related macros and function declations         |
| Crypto.c          | Crypro related functions                           |
| Crypto.h          | Crypto related macros and function declations      |
| Timer.c           | Timer related functions                          	 |
| Timer.h           | Timer related macros and function declations       |
| Interrupt.c       | Interrupt related functions                        |
| Interrupt.h       | Interrupt related macros and function declations   |
| Dice.c            | Dice related functions                             |
| Dice.h            | Dice related macros and function declations        |
| SpiFilter.c       | SpiFilter related functions                        |
| SpiFilter.h       | SpiFilter related macros and function declations   |
| SmbusFilter.c     | SmbusFilter related functions                      |
| SmbusFilter.h     | SmbusFilter related macros and function declations |

#Function Interface definitions for Gpio
 **GpioInit** - Function to initialize GPIO
 **GpioGet** - Function to get value of the GPIO PIN
 **GpioSet** - Function to Set value for the GPIO PIN
 **GpioHandler** - Function to be called when the various interrupts trigger to the GPIO handler
 **GpioConfig** - Function to configure the GPIO pin
 **SysReset**  - Function to reset the EC/microcontroller
 **SerialInit** - Function to initialize UART pins, Baudrate for Serial Port
 **LedSet** - Function to set Led color
 **LedInit** -  Function to initialize the Led.

#Function Interface definitions for Smbus
 **SmbusInit** - Initiate the Smbus interface
 **SmbusRead** - To Read the Data TO FIFO
 **SmbusWrite** - To Write the Data to FIFO

#Function Interface definitions for I2c
 **I2cSlave** - Function to get the buffer sent by client.
 **I2cInit** - Function to initialize the I2cCommunication.
 **I2cSlaveWrite** -  Function to write data into slave
 **I2cSlaveRead**  -  Function to read data from slave
 **I2cPortInit** - Function to initialize a port for I2c communication

#Function Interface definitions for Spi
 **SpiInit** - Function to initialize SPI
 **SpiRead** - Function to read data in SPI port
 **SpiWrite** - Function to write data in SPI port
 **SpiErase** - Function to erase the particular chip of size 4k
 **SpiDmaRead** - Function to read the data using DMA mode
 **SpiDeInit** - Function to deinitialize the Spi port

#Function Interface definitions for Crypto
 **HashInit** - Function to initialize Hash256 and initialize the HashBuffer
 **HashFinalize** - Function to finalize the end hash256 for which buffer hash been update.
 **HashUpdate** - Function to update hash256 for the buffer.
 **HashInit_384** - Function to initialize Hash384 and initialize the HashBuffer
 **ModularExponetiation** - Function to decrypt the signature buffer and return the decrytped hash.
 **HashUpdate_384** - Function to update hash384 for particular buffer.
 **HashFinalize_384** - Function to finalize the end hash384 for which buffer hash been update.
 **ECDSA_Authentication** -   Function to perform ec curve authentication for the given hash digest and keys
 **GetKey** - Function to read character from Serial Port

#Function Interface definitions for Timer
 **BasicTimerInit** - Function to Initialize BasicTimer.
 **BasicTimerStart** - Function to Start BasicTimer
 **BasicTimerStop** - Function to Stop BasicTimer
 **TimerInit** - Function to Initialize Capture Compare Timer
 **TimerDelay** - Function to add delay
 **TimerDelayMs** - Function to add delay in milli seconds
 **TimerDelayUs** - Function to add delay in microseconds
 **TimerCount** - Function to get the current Capture Compare Value
 **RTC_time_set** - Function to set the time of RTC
 **RTC_time_get** - Function to get the time from RTC
 **RTC_date_set** - Function to set the date of RTC
 **RTC_date_get** - Function to get the date of RTC
 **RTC_init** - Function to initialize and start RTC Operations

#Function Interface definitions for Interrupt
 **InterruptInit** - Function to initialize the various Interrupts
 **TimerInterruptInit** - Function to initialize Timer Interrupt

#Function Interface definitions for SpiFilter
 **SpiFilterInit** - Function to Initialize SpiFilter.
 **SpiFilterDeInit** - Function to De-Initialize SpiFilter.
 **SpiFilterEnable** - Function to Enable SpiFilter.
 **SpiFilterDisable** - Function to Disable SpiFilter.

#Function Interface definitions for SmbusFilter
 **SmbusFilterInit** - Function to Initialize SmbusFilter.
 **SmbusFilterDeInit** -  Function to De-Initialize SmbusFilter.
 **SmbusFilterEnable** - Function to Enable SmbusFilter.
 **SmbusFilterDisable** - Function to Disable SmbusFilter.

#Function Interface definitions for Dice
 **DiceStatus** - Function to Initialize Dice.

