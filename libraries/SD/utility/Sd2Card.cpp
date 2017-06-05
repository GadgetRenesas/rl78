/* Arduino Sd2Card Library
 * Copyright (C) 2009 by William Greiman
 *
 * This file is part of the Arduino Sd2Card Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Sd2Card Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Modified 1 March 2013 by masahiko.nagata.cj@renesas.com
 */
#include <Arduino.h>
#include "Sd2Card.h"
//------------------------------------------------------------------------------
#ifndef SOFTWARE_SPI
// functions for hardware SPI
static void spiInit(uint8_t channel)
{

	if(channel == 1){
	    digitalWrite(SPI_SCK_PIN, HIGH);
	    digitalWrite(SPI_MOSI_PIN, HIGH);

	#ifdef WORKAROUND_READ_MODIFY_WRITE
	    SBI2( SFR2_PER0, SFR2_BIT_SAUxEN );  /* supply SAU1 clock */
	    _NOP();
	    _NOP();
	    _NOP();
	    _NOP();
	    SPI_SPSx     = 0x0001;  // osc/2

	    SPI_STx     |= SPI_CHx;  /* disable CSIxx */
	    SBI( SFR_MKxx, SFR_BIT_CSIMKxx );   /* disable INTCSIxx interrupt */

	    /* Set INTCSIxx high priority */
	    CBI( SFR_PR1xx, SFR_BIT_CSIPR1xx );
	    CBI( SFR_PR0xx, SFR_BIT_CSIPR0xx );
	    SPI_SIRxx    = 0x0007U;  /* clear error flag */
	    SPI_SMRxx    = 0x0020U;
	    SPI_SCRxx    = 0xF007U;
	    SPI_SDRxx    = 0x7E00U;  /* (osc/2)*(1/(63+1))*(1/2) = osc/256 */

	    SPI_SOx     |= SPI_CHx << 8;  /* CSIxx clock initial level */
	    SPI_SOx     &= ~SPI_CHx; /* CSIxx SO initial level */
	    SPI_SOEx    |= SPI_CHx;  /* enable CSIxx output */

	    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );   /* clear INTCSIxx interrupt flag */
	    SPI_SSx     |= SPI_CHx;   /* enable CSIxx */
	#else
	    SPI_SAUxEN   = 1U;    /* supply SAUx clock */
	    NOP();
	    NOP();
	    NOP();
	    NOP();
	    SPI_SPSx     = 0x0001;  // osc/2

	    SPI_STx     |= SPI_CHx;  /* disable CSIxx */
	    SPI_CSIMKxx  = 1U;       /* disable INTCSIxx interrupt */

	    /* Set INTCSIxx high priority */
	    SPI_CSIPR1xx = 0U;
	    SPI_CSIPR0xx = 0U;
	    SPI_SIRxx    = 0x0007U;  /* clear error flag */
	    SPI_SMRxx    = 0x0020U;
	    SPI_SCRxx    = 0xF007U;
	    SPI_SDRxx    = 0x7E00U;  /* (osc/2)*(1/(63+1))*(1/2) = osc/256 */
	    SPI_SOx     |= SPI_CHx << 8;  /* CSIxx clock initial level */
	    SPI_SOx     &= ~SPI_CHx; /* CSIxx SO initial level */
	    SPI_SOEx    |= SPI_CHx;  /* enable CSIxx output */

	    SPI_CSIIFxx  = 0U;        /* clear INTCSIxx interrupt flag */
	    SPI_SSx     |= SPI_CHx;   /* enable CSIxx */
	#endif

	} else {
		pinMode(SCK2, OUTPUT);
		pinMode(MISO2, INPUT);
		pinMode(MOSI2, OUTPUT);
		pinMode(SS2, OUTPUT);

		digitalWrite(SCK2, HIGH);
		digitalWrite(MOSI2, HIGH);
		digitalWrite(SS2, HIGH);

		if (SPI2_SAUxEN == 0) {
#ifdef WORKAROUND_READ_MODIFY_WRITE
			SBI2(SFR2_PER0, SFR22_BIT_SAUxEN);  // クロック供給開始
#else
					SPI_SAUxEN = 1;                    // クロック供給開始
#endif
			_NOP();
			_NOP();
			_NOP();
			_NOP();
			SPI2_SPSx = 0x0001;                 // 動作クロック設定
		}

	    SPI2_STx     |= SPI2_CHx;  /* disable CSIxx */
	    SBI( SFR2_MKxx, SFR2_BIT_CSIMKxx );   /* disable INTCSIxx interrupt */

	    /* Set INTCSIxx high priority */
	    CBI( SFR2_PR1xx, SFR2_BIT_CSIPR1xx );
	    CBI( SFR2_PR0xx, SFR2_BIT_CSIPR0xx );
	    SPI2_SIRxx    = 0x0007U;  /* clear error flag */
	    SPI2_SMRxx    = 0x0020U;
	    SPI2_SCRxx    = 0xF007U;
	    SPI2_SDRxx    = 0x7E00U;  /* (osc/2)*(1/(63+1))*(1/2) = osc/256 */

	    SPI2_SOx     |= SPI2_CHx << 8;  /* CSIxx clock initial level */
	    SPI2_SOx     &= ~SPI2_CHx; /* CSIxx SO initial level */
	    SPI2_SOEx    |= SPI2_CHx;  /* enable CSIxx output */

	    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );   /* clear INTCSIxx interrupt flag */
	    SPI2_SSx     |= SPI2_CHx;   /* enable CSIxx */
	}
}
/** Send a byte to the card */
static void spiSend(uint8_t b, uint8_t channel) {

	if(channel == 1){
		SPI_SIOxx = b;
		while(!SPI_CSIIFxx);
	#ifdef WORKAROUND_READ_MODIFY_WRITE
	    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
	#else
		SPI_CSIIFxx = 0U;
	#endif

	} else {

		SPI2_SIOxx = b;
		while(!SPI2_CSIIFxx);
	    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
	}
}
/** Receive a byte from the card */
static  uint8_t spiRec(uint8_t channel) {
  spiSend(0XFF, channel);
  if(channel == 1){
	  return((uint8_t)SPI_SIOxx);

  }else{
	  return((uint8_t)SPI2_SIOxx);

  }
}
#else  // SOFTWARE_SPI
//------------------------------------------------------------------------------
/** nop to tune soft SPI timing */
#define nop asm volatile ("nop\n\t")
//------------------------------------------------------------------------------
/** Soft SPI receive */
uint8_t spiRec(void) {
  uint8_t data = 0;
  // no interrupts during byte receive - about 8 us
  cli();
  // output pin high - like sending 0XFF
  fastDigitalWrite(SPI_MOSI_PIN, HIGH);

  for (uint8_t i = 0; i < 8; i++) {
    fastDigitalWrite(SPI_SCK_PIN, HIGH);

    // adjust so SCK is nice
    nop;
    nop;

    data <<= 1;

    if (fastDigitalRead(SPI_MISO_PIN)) data |= 1;

    fastDigitalWrite(SPI_SCK_PIN, LOW);
  }
  // enable interrupts
  sei();
  return data;
}
//------------------------------------------------------------------------------
/** Soft SPI send */
void spiSend(uint8_t data) {
  // no interrupts during byte send - about 8 us
  cli();
  for (uint8_t i = 0; i < 8; i++) {
    fastDigitalWrite(SPI_SCK_PIN, LOW);

    fastDigitalWrite(SPI_MOSI_PIN, data & 0X80);

    data <<= 1;

    fastDigitalWrite(SPI_SCK_PIN, HIGH);
  }
  // hold SCK high for a few ns
  nop;
  nop;
  nop;
  nop;

  fastDigitalWrite(SPI_SCK_PIN, LOW);
  // enable interrupts
  sei();
}
#endif  // SOFTWARE_SPI
//------------------------------------------------------------------------------
// send command and return error code.  Return zero for OK
uint8_t Sd2Card::cardCommand(uint8_t cmd, uint32_t arg) {
  // end read if in partialBlockRead mode
  readEnd();

  // select card
  chipSelectLow();

  // wait up to 300 ms if busy
  waitNotBusy(300);

  // send command
  spiSend(cmd | 0x40, channel);

  // send argument
  for (int8_t s = 24; s >= 0; s -= 8) spiSend(arg >> s, channel);

  // send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0) crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8) crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  spiSend(crc, channel);

  // wait for response
  for (uint8_t i = 0; ((status_ = spiRec(channel)) & 0X80) && i != 0XFF; i++);
  return status_;
}
//------------------------------------------------------------------------------
/**
 * Determine the size of an SD flash memory card.
 *
 * \return The number of 512 byte data blocks in the card
 *         or zero if an error occurs.
 */
uint32_t Sd2Card::cardSize(void) {
  csd_t csd;
  if (!readCSD(&csd)) return 0;
  if (csd.v1.csd_ver == 0) {
    uint8_t read_bl_len = csd.v1.read_bl_len;
    uint16_t c_size = (csd.v1.c_size_high << 10)
                      | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
    uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1)
                          | csd.v1.c_size_mult_low;
    return (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
  } else if (csd.v2.csd_ver == 1) {
    uint32_t c_size = ((uint32_t)csd.v2.c_size_high << 16)
                      | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
    return (c_size + 1) << 10;
  } else {
    error(SD_CARD_ERROR_BAD_CSD);
    return 0;
  }
}
//------------------------------------------------------------------------------
void Sd2Card::chipSelectHigh(void) {
  digitalWrite(chipSelectPin_, HIGH);
}
//------------------------------------------------------------------------------
void Sd2Card::chipSelectLow(void) {
  digitalWrite(chipSelectPin_, LOW);
}
//------------------------------------------------------------------------------
/** Erase a range of blocks.
 *
 * \param[in] firstBlock The address of the first block in the range.
 * \param[in] lastBlock The address of the last block in the range.
 *
 * \note This function requests the SD card to do a flash erase for a
 * range of blocks.  The data on the card after an erase operation is
 * either 0 or 1, depends on the card vendor.  The card must support
 * single block erase.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::erase(uint32_t firstBlock, uint32_t lastBlock) {
  if (!eraseSingleBlockEnable()) {
    error(SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
    goto fail;
  }
  if (type_ != SD_CARD_TYPE_SDHC) {
    firstBlock <<= 9;
    lastBlock <<= 9;
  }
  if (cardCommand(CMD32, firstBlock)
    || cardCommand(CMD33, lastBlock)
    || cardCommand(CMD38, 0)) {
      error(SD_CARD_ERROR_ERASE);
      goto fail;
  }
  if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
    error(SD_CARD_ERROR_ERASE_TIMEOUT);
    goto fail;
  }
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/** Determine if card supports single block erase.
 *
 * \return The value one, true, is returned if single block erase is supported.
 * The value zero, false, is returned if single block erase is not supported.
 */
uint8_t Sd2Card::eraseSingleBlockEnable(void) {
  csd_t csd;
  return readCSD(&csd) ? csd.v1.erase_blk_en : 0;
}
//------------------------------------------------------------------------------
/**
 * Initialize an SD flash memory card.
 *
 * \param[in] sckRateID SPI clock rate selector. See setSckRate().
 * \param[in] chipSelectPin SD chip select pin number.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.  The reason for failure
 * can be determined by calling errorCode() and errorData().
 */
uint8_t Sd2Card::init(uint8_t sckRateID, uint8_t chipSelectPin) {
  errorCode_ = inBlock_ = partialBlockRead_ = type_ = 0;
  chipSelectPin_ = chipSelectPin;
  // 16-bit init start time allows over a minute
  uint16_t t0 = (uint16_t)millis();
  uint32_t arg;

  // set pin modes
  if(channel == 1){
	  pinMode(chipSelectPin_, OUTPUT);
	  chipSelectHigh();
	  pinMode(SPI_MISO_PIN, INPUT);
	  pinMode(SPI_MOSI_PIN, OUTPUT);
	  pinMode(SPI_SCK_PIN, OUTPUT);
  } else {
	  pinMode(chipSelectPin_, OUTPUT);
	  chipSelectHigh();
	  pinMode(SPI2_MISO_PIN, INPUT);
	  pinMode(SPI2_MOSI_PIN, OUTPUT);
	  pinMode(SPI2_SCK_PIN, OUTPUT);

  }

#ifndef SOFTWARE_SPI
  // SS must be in output mode even it is not chip select
  if(channel == 1){
	  pinMode(SS_PIN, OUTPUT);
	  digitalWrite(SS_PIN, HIGH); // disable any SPI device using hardware SS pin
  } else {
	  pinMode(SS2_PIN, OUTPUT);
	  digitalWrite(SS2_PIN, HIGH); // disable any SPI device using hardware SS pin

  }
  // Enable SPI, Master, clock rate f_osc/128
  spiInit(channel);
#endif  // SOFTWARE_SPI

  // must supply min of 74 clock cycles with CS high.
  for (uint8_t i = 0; i < 10; i++) spiSend(0XFF, channel);

  chipSelectLow();

  // command to go idle in SPI mode
  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {
    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_CMD0);
      goto fail;
    }
  }
  // check SD version
  if ((cardCommand(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND)) {
    type(SD_CARD_TYPE_SD1);
  } else {
    // only need last byte of r7 response
    for (uint8_t i = 0; i < 4; i++) status_ = spiRec(channel);
    if (status_ != 0XAA) {
      error(SD_CARD_ERROR_CMD8);
      goto fail;
    }
    type(SD_CARD_TYPE_SD2);
  }
  // initialize card and send host supports SDHC if SD2
  arg = type() == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

  while ((status_ = cardAcmd(ACMD41, arg)) != R1_READY_STATE) {
    // check for timeout
    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_ACMD41);
      goto fail;
    }
  }
  // if SD2 read OCR register to check for SDHC card
  if (type() == SD_CARD_TYPE_SD2) {
    if (cardCommand(CMD58, 0)) {
      error(SD_CARD_ERROR_CMD58);
      goto fail;
    }
    if ((spiRec(channel) & 0XC0) == 0XC0) type(SD_CARD_TYPE_SDHC);
    // discard rest of ocr - contains allowed voltage range
    for (uint8_t i = 0; i < 3; i++) spiRec(channel);
  }
  chipSelectHigh();

#ifndef SOFTWARE_SPI
  return setSckRate(sckRateID);
#else  // SOFTWARE_SPI
  return true;
#endif  // SOFTWARE_SPI

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/**
 * Enable or disable partial block reads.
 *
 * Enabling partial block reads improves performance by allowing a block
 * to be read over the SPI bus as several sub-blocks.  Errors may occur
 * if the time between reads is too long since the SD card may timeout.
 * The SPI SS line will be held low until the entire block is read or
 * readEnd() is called.
 *
 * Use this for applications like the Adafruit Wave Shield.
 *
 * \param[in] value The value TRUE (non-zero) or FALSE (zero).)
 */
void Sd2Card::partialBlockRead(uint8_t value) {
  readEnd();
  partialBlockRead_ = value;
}
//------------------------------------------------------------------------------
/**
 * Read a 512 byte block from an SD card device.
 *
 * \param[in] block Logical block to be read.
 * \param[out] dst Pointer to the location that will receive the data.

 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::readBlock(uint32_t block, uint8_t* dst) {
  return readData(block, 0, 512, dst);
}
//------------------------------------------------------------------------------
/**
 * Read part of a 512 byte block from an SD card.
 *
 * \param[in] block Logical block to be read.
 * \param[in] offset Number of bytes to skip at start of block
 * \param[out] dst Pointer to the location that will receive the data.
 * \param[in] count Number of bytes to read
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::readData(uint32_t block,
        uint16_t offset, uint16_t count, uint8_t* dst) {
  uint16_t n;
  if (count == 0) return true;
  if ((count + offset) > 512) {
    goto fail;
  }
  if (!inBlock_ || block != block_ || offset < offset_) {
    block_ = block;
    // use address if not SDHC card
    if (type()!= SD_CARD_TYPE_SDHC) block <<= 9;
    if (cardCommand(CMD17, block)) {
      error(SD_CARD_ERROR_CMD17);
      goto fail;
    }
    if (!waitStartBlock()) {
      goto fail;
    }
    offset_ = 0;
    inBlock_ = 1;
  }

#ifdef OPTIMIZE_HARDWARE_SPI
  if(channel == 1){
	  // start first spi transfer
	  SPI_SIOxx = 0xFF;

	  // skip data before offset
	  for (;offset_ < offset; offset_++) {
	    while (!SPI_CSIIFxx);
	#ifdef WORKAROUND_READ_MODIFY_WRITE
	    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
	#else
	    SPI_CSIIFxx = 0U;
	#endif
	    SPI_SIOxx = 0xFF;
	  }
	  // transfer data
	  n = count - 1;
	  for (uint16_t i = 0; i < n; i++) {
	    while (!SPI_CSIIFxx);
	#ifdef WORKAROUND_READ_MODIFY_WRITE
	    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
	#else
	    SPI_CSIIFxx = 0U;
	#endif
	    dst[i] = SPI_SIOxx;
	    SPI_SIOxx = 0xFF;
	  }
	  // wait for last byte
	  while (!SPI_CSIIFxx);
	#ifdef WORKAROUND_READ_MODIFY_WRITE
	  CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
	#else
	  SPI_CSIIFxx = 0U;
	#endif
	  dst[n] = SPI_SIOxx;

  } else {
	  // start first spi transfer
	  SPI2_SIOxx = 0xFF;

	  // skip data before offset
	  for (;offset_ < offset; offset_++) {
	    while (!SPI2_CSIIFxx);
	    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
	    SPI2_SIOxx = 0xFF;
	  }
	  // transfer data
	  n = count - 1;
	  for (uint16_t i = 0; i < n; i++) {
	    while (!SPI2_CSIIFxx);
	    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
	    dst[i] = SPI2_SIOxx;
	    SPI2_SIOxx = 0xFF;
	  }
	  // wait for last byte
	  while (!SPI2_CSIIFxx);
	  CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
	  dst[n] = SPI2_SIOxx;

  }

#else  // OPTIMIZE_HARDWARE_SPI

  // skip data before offset
  for (;offset_ < offset; offset_++) {
    spiRec();
  }
  // transfer data
  for (uint16_t i = 0; i < count; i++) {
    dst[i] = spiRec();
  }
#endif  // OPTIMIZE_HARDWARE_SPI

  offset_ += count;
  if (!partialBlockRead_ || offset_ >= 512) {
    // read rest of data, checksum and set chip select high
    readEnd();
  }
  return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/** Skip remaining data in a block when in partial block read mode. */
void Sd2Card::readEnd(void) {
  if (inBlock_) {
	  if(channel == 1){
	      // skip data and crc
	#ifdef OPTIMIZE_HARDWARE_SPI
	    // optimize skip for hardware
	    SPI_SIOxx = 0xFF;
	    while (offset_++ < 513) {
	      while (!SPI_CSIIFxx);
	#ifdef WORKAROUND_READ_MODIFY_WRITE
	      CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
	#else
	      SPI_CSIIFxx = 0U;
	#endif
	      SPI_SIOxx = 0xFF;
	    }
	    // wait for last crc byte
	    while (!SPI_CSIIFxx);
	#ifdef WORKAROUND_READ_MODIFY_WRITE
	    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
	#else
	    SPI_CSIIFxx = 0U;
	#endif
	#else  // OPTIMIZE_HARDWARE_SPI
	    while (offset_++ < 514) spiRec();
	#endif  // OPTIMIZE_HARDWARE_SPI

	  } else {
	      // skip data and crc
	#ifdef OPTIMIZE_HARDWARE_SPI
	    // optimize skip for hardware
	    SPI2_SIOxx = 0xFF;
	    while (offset_++ < 513) {
	      while (!SPI2_CSIIFxx);
	      CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
	      SPI2_SIOxx = 0xFF;
	    }
	    // wait for last crc byte
	    while (!SPI2_CSIIFxx);
	    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
	#else  // OPTIMIZE_HARDWARE_SPI
	    while (offset_++ < 514) spiRec();
	#endif  // OPTIMIZE_HARDWARE_SPI

	  }
    chipSelectHigh();
    inBlock_ = 0;
  }
}
//------------------------------------------------------------------------------
/** read CID or CSR register */
uint8_t Sd2Card::readRegister(uint8_t cmd, void* buf) {
  uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
  if (cardCommand(cmd, 0)) {
    error(SD_CARD_ERROR_READ_REG);
    goto fail;
  }
  if (!waitStartBlock()) goto fail;
  // transfer data
  for (uint16_t i = 0; i < 16; i++) dst[i] = spiRec(channel);
  spiRec(channel);  // get first crc byte
  spiRec(channel);  // get second crc byte
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/**
 * Set the SPI clock rate.
 *
 * \param[in] sckRateID A value in the range [0, 6].
 *
 * The SPI clock will be set to F_CPU/pow(2, 1 + sckRateID). The maximum
 * SPI rate is F_CPU/2 for \a sckRateID = 0 and the minimum rate is F_CPU/128
 * for \a scsRateID = 6.
 *
 * \return The value one, true, is returned for success and the value zero,
 * false, is returned for an invalid value of \a sckRateID.
 */
uint8_t Sd2Card::setSckRate(uint8_t sckRateID) {
  if (sckRateID > 6) {
    error(SD_CARD_ERROR_SCK_RATE);
    return false;
  }
  // see avr processor datasheet for SPI register bit definitions
  if(channel == 1){
#ifdef WORKAROUND_READ_MODIFY_WRITE
  SPI_STx     |= SPI_CHx;      /* disable CSIxx */
  SPI_SOEx    &= ~SPI_CHx;   /* disable CSIxx output */
  SPI_SDRxx    = (uint16_t)((1 << sckRateID) -1) << 9; // ID=0:osc/4,ID=1:osc/8　- ,ID=6:osc/256
  SPI_SOx     |= SPI_CHx << 8;  /* CSIxx clock initial level */
  SPI_SOx     &= ~SPI_CHx; /* CSIxx SO initial level */
  SPI_SOEx    |= SPI_CHx;  /* enable CSIxx output */
  CBI( SFR_IFxx, SFR_BIT_CSIIFxx );   /* clear INTCSIxx interrupt flag */
  SPI_SSx     |= SPI_CHx;  /* enable CSIxx */
#else
  SPI_STx     |= SPI_CHx;      /* disable CSIxx */
  SPI_SOEx    &= ~SPI_CHx;   /* disable CSIxx output */
  SPI_SDRxx    = (uint16_t)((1 << sckRateID) -1) << 9; // ID=0:osc/4,ID=1:osc/8　- ,ID=6:osc/256
  SPI_SOx     |= SPI_CHx << 8;  /* CSIxx clock initial level */
  SPI_SOx     &= ~SPI_CHx; /* CSIxx SO initial level */
  SPI_SOEx    |= SPI_CHx;  /* enable CSIxx output */
  SPI_CSIIFxx  = 0U;       /* clear INTCSIxx interrupt flag */
  SPI_SSx     |= SPI_CHx;  /* enable CSIxx */
#endif

  } else {
  SPI2_STx     |= SPI2_CHx;      /* disable CSIxx */
  SPI2_SOEx    &= ~SPI2_CHx;   /* disable CSIxx output */
  SPI2_SDRxx    = (uint16_t)((1 << sckRateID) -1) << 9; // ID=0:osc/4,ID=1:osc/8　- ,ID=6:osc/256
  SPI2_SOx     |= SPI2_CHx << 8;  /* CSIxx clock initial level */
  SPI2_SOx     &= ~SPI2_CHx; /* CSIxx SO initial level */
  SPI2_SOEx    |= SPI2_CHx;  /* enable CSIxx output */
  CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );   /* clear INTCSIxx interrupt flag */
  SPI2_SSx     |= SPI2_CHx;  /* enable CSIxx */

  }
  return true;
}
//------------------------------------------------------------------------------
// wait for card to go not busy
uint8_t Sd2Card::waitNotBusy(uint16_t timeoutMillis) {
  uint16_t t0 = millis();
  do {
    if (spiRec(channel) == 0XFF) return true;
  }
  while (((uint16_t)millis() - t0) < timeoutMillis);
  return false;
}
//------------------------------------------------------------------------------
/** Wait for start block token */
uint8_t Sd2Card::waitStartBlock(void) {
  uint16_t t0 = millis();
  while ((status_ = spiRec(channel)) == 0XFF) {
    if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) {
      error(SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }
  if (status_ != DATA_START_BLOCK) {
    error(SD_CARD_ERROR_READ);
    goto fail;
  }
  return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/**
 * Writes a 512 byte block to an SD card.
 *
 * \param[in] blockNumber Logical block to be written.
 * \param[in] src Pointer to the location of the data to be written.
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::writeBlock(uint32_t blockNumber, const uint8_t* src) {
#if SD_PROTECT_BLOCK_ZERO
  // don't allow write to first block
  if (blockNumber == 0) {
    error(SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    goto fail;
  }
#endif  // SD_PROTECT_BLOCK_ZERO

  // use address if not SDHC card
  if (type() != SD_CARD_TYPE_SDHC) blockNumber <<= 9;
  if (cardCommand(CMD24, blockNumber)) {
    error(SD_CARD_ERROR_CMD24);
    goto fail;
  }
  if (!writeData(DATA_START_BLOCK, src)) goto fail;

  // wait for flash programming to complete
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    error(SD_CARD_ERROR_WRITE_TIMEOUT);
    goto fail;
  }
  // response is r2 so get and check two bytes for nonzero
  if (cardCommand(CMD13, 0) || spiRec(channel)) {
    error(SD_CARD_ERROR_WRITE_PROGRAMMING);
    goto fail;
  }
  chipSelectHigh();
return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/** Write one data block in a multiple block write sequence */
uint8_t Sd2Card::writeData(const uint8_t* src) {
  // wait for previous write to finish
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    error(SD_CARD_ERROR_WRITE_MULTIPLE);
    chipSelectHigh();
    return false;
  }
  return writeData(WRITE_MULTIPLE_TOKEN, src);
}
//------------------------------------------------------------------------------
// send one block of data for write block or write multiple blocks
uint8_t Sd2Card::writeData(uint8_t token, const uint8_t* src) {
	if(channel == 1){
#ifdef OPTIMIZE_HARDWARE_SPI
		  // send data - optimized loop
		  SPI_SIOxx = token;

		  // send two byte per iteration
		  for (uint16_t i = 0; i < 512; i += 2) {
		    while(!SPI_CSIIFxx);
		#ifdef WORKAROUND_READ_MODIFY_WRITE
		    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
		#else
		    SPI_CSIIFxx = 0U;
		#endif
		    SPI_SIOxx = src[i];
		    while (!SPI_CSIIFxx);
		#ifdef WORKAROUND_READ_MODIFY_WRITE
		    CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
		#else
		    SPI_CSIIFxx = 0U;
		#endif
		    SPI_SIOxx = src[i+1];
		  }

		  // wait for last data byte
		  while (!SPI_CSIIFxx);
		#ifdef WORKAROUND_READ_MODIFY_WRITE
		  CBI( SFR_IFxx, SFR_BIT_CSIIFxx );
		#else
		  SPI_CSIIFxx = 0U;
		#endif
		#else  // OPTIMIZE_HARDWARE_SPI
		  spiSend(token);
		  for (uint16_t i = 0; i < 512; i++) {
		    spiSend(src[i]);
		  }
		#endif  // OPTIMIZE_HARDWARE_SPI
		  spiSend(0xff, channel);  // dummy crc
		  spiSend(0xff, channel);  // dummy crc

		  status_ = spiRec(channel);
		  if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
		    error(SD_CARD_ERROR_WRITE);
		    chipSelectHigh();
		    return false;
		  }

	} else {
#ifdef OPTIMIZE_HARDWARE_SPI
		  // send data - optimized loop
		  SPI2_SIOxx = token;

		  // send two byte per iteration
		  for (uint16_t i = 0; i < 512; i += 2) {
		    while(!SPI2_CSIIFxx);
		    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
		    SPI2_SIOxx = src[i];
		    while (!SPI2_CSIIFxx);
		    CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
		    SPI2_SIOxx = src[i+1];
		  }

		  // wait for last data byte
		  while (!SPI2_CSIIFxx);
		  CBI( SFR2_IFxx, SFR2_BIT_CSIIFxx );
#else  // OPTIMIZE_HARDWARE_SPI
  spiSend(token);
  for (uint16_t i = 0; i < 512; i++) {
    spiSend(src[i]);
  }
#endif  // OPTIMIZE_HARDWARE_SPI
		  spiSend(0xff, channel);  // dummy crc
		  spiSend(0xff, channel);  // dummy crc

		  status_ = spiRec(channel);
		  if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
		    error(SD_CARD_ERROR_WRITE);
		    chipSelectHigh();
		    return false;
		  }

	}

  return true;
}
//------------------------------------------------------------------------------
/** Start a write multiple blocks sequence.
 *
 * \param[in] blockNumber Address of first block in sequence.
 * \param[in] eraseCount The number of blocks to be pre-erased.
 *
 * \note This function is used with writeData() and writeStop()
 * for optimized multiple block writes.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::writeStart(uint32_t blockNumber, uint32_t eraseCount) {
#if SD_PROTECT_BLOCK_ZERO
  // don't allow write to first block
  if (blockNumber == 0) {
    error(SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    goto fail;
  }
#endif  // SD_PROTECT_BLOCK_ZERO
  // send pre-erase count
  if (cardAcmd(ACMD23, eraseCount)) {
    error(SD_CARD_ERROR_ACMD23);
    goto fail;
  }
  // use address if not SDHC card
  if (type() != SD_CARD_TYPE_SDHC) blockNumber <<= 9;
  if (cardCommand(CMD25, blockNumber)) {
    error(SD_CARD_ERROR_CMD25);
    goto fail;
  }
  return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/** End a write multiple blocks sequence.
 *
* \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::writeStop(void) {
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  spiSend(STOP_TRAN_TOKEN, channel);
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  chipSelectHigh();
  return true;

 fail:
  error(SD_CARD_ERROR_STOP_TRAN);
  chipSelectHigh();
  return false;
}

void Sd2Card::channelselect(uint8_t ch){
	channel = ch;

}