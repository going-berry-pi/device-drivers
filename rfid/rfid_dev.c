#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#define RFID_MAJOR_NUMBER 503
#define RFID_MINOR_NUMBER 103
#define RFID_DEV_NAME "rfid"
#define PICC_CMD_REQA 0x26
#define GPIO_BASE_ADDR 0x3F200000
#define SPI_BASE_ADDR  0x3f204000
#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1c
#define GPCLR0 0x28
#define GPLEV0 0x34
#define TModeReg 0x2A<<1
#define TPrescalerReg 0x2B<<1
#define TReloadRegH 0x2C<<1
#define TReloadRegL 0x2D<<1
#define TxASKReg 0x15<<1
#define ModeReg 0x11<<1
#define TxControlReg 0x14<<1
#define CommandReg 0x01<<1
#define ErrorReg 0x06<<1
#define PCD_SoftReset 0x0F
#define FIFODataReg 0x09<<1
#define FIFOLevelReg 0x0A<<1
#define BitFramingReg 0x0D<<1
#define ControlReg 0x0C<<1
#define STATUS_COLLISION 3
#define STATUS_MIFARE_NACK 9
#define STATUS_CRC_WRONG 8
#define STATUS_OK 1
#define ComIrqReg 8
#define DivIrqReg 10
#define STATUS_TIMEOUT 4
#define PCD_Idle 0
#define CRCResultRegL 68
#define CRCResultRegH 66
#define PCD_Transceive 0x0C
#define PCD_CalcCRC 0x03
#define STATUS_ERROR 2
#define STATUS_NO_ROOM 5
#define CollReg 0x0E<<1 // bit position of the first bit-collision detected on the RF interface
#define PICC_CMD_SEL_CL1 0x93
#define PICC_CMD_SEL_CL2 0x95
#define PICC_CMD_SEL_CL3 0x97
#define STATUS_INTERNAL_ERROR 6
#define STATUS_INVALID 7
#define PICC_CMD_CT 0x88
static void __iomem * gpio_base;
static void __iomem * spi_base;
volatile unsigned int* gpsel0;
volatile unsigned int* gpsel1;
volatile unsigned int* gpsel2;
volatile unsigned int* gpset1;
volatile unsigned int* gpclr1;
volatile unsigned int* gplev;
volatile unsigned int* gpclr1;
typedef uint8_t byte;
typedef uint16_t word;
// A struct used for passing the UID of a PICC.
	typedef struct {
		byte		size;			// Number of bytes in the UID. 4, 7 or 10.
		byte		uidByte[10];
		byte		sak;			// The SAK (Select acknowledge) byte returned from the PICC after successful selection.
	} Uid;
Uid uid;
char transfer(char tbuf1, char tbuf2) {
	volatile uint32_t* paddr = spi_base + 0;
	volatile uint32_t* fifo = spi_base + 4;
	char tbuf[2];
	tbuf[0] = tbuf1;
	tbuf[1] = tbuf2;
	
	char rbuf[2];
	uint32_t TXCnt = 0;
	uint32_t RXCnt = 0;

	*paddr = *paddr | 0x00000030; //clear
	*paddr = *paddr | 0x00000080;/*!< Transfer Active */

	while ((TXCnt < 2) || (RXCnt < 2))
	{
		/* TX fifo not full, so add some more bytes */
		while ((*paddr & 0x00040000) && (TXCnt < 2))
		{
			//  printk(KERN_ALERT "fifo : %x", *fifo);
			*fifo = tbuf[TXCnt];
			//    printk(KERN_ALERT "tbuf -> fifo : %x", *fifo);
			TXCnt++;
		}
		/* Rx fifo not empty, so get the next received bytes */
		while (((*paddr & 0x00020000)) && (RXCnt < 2))
		{
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			rbuf[RXCnt] = *fifo;
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			RXCnt++;
		}
	}
	/* Wait for DONE to be set */
	while (!(*paddr & 0x00010000));

	uint32_t v = *paddr;
	v = (v & ~0x00000080) | (0 & 0x00000080);
	*paddr = v;
	return rbuf[1];
}

uint8_t spi_transfer(uint8_t value) {
	volatile uint32_t* paddr = spi_base + 0;
	volatile uint32_t* fifo = spi_base + 4;
	char tbuf[1];
	tbuf[0] = value;
	
	uint32_t TXCnt = 0;
	uint32_t RXCnt = 0;

	*paddr = *paddr | 0x00000030; //clear
	*paddr = *paddr | 0x00000080;/*!< Transfer Active */

	while ((TXCnt < 1) || (RXCnt < 1))
	{
		/* TX fifo not full, so add some more bytes */
		while ((*paddr & 0x00040000) && (TXCnt < 1))
		{
			//  printk(KERN_ALERT "fifo : %x", *fifo);
			*fifo = tbuf[TXCnt];
			//    printk(KERN_ALERT "tbuf -> fifo : %x", *fifo);
			TXCnt++;
		}
		/* Rx fifo not empty, so get the next received bytes */
		while (((*paddr & 0x00020000)) && (RXCnt < 1))
		{
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			tbuf[RXCnt] = *fifo;
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			RXCnt++;
		}
	}
	/* Wait for DONE to be set */
	while (!(*paddr & 0x00010000));

	uint32_t v = *paddr;
	v = (v & ~0x00000080) | (0 & 0x00000080);
	*paddr = v;
	return tbuf[0];
}
void PCD_WriteRegister1(byte reg, byte value) {
	volatile uint32_t* paddr = spi_base + 0;
	volatile uint32_t* fifo = spi_base + 4;
	char tbuf[2];
	tbuf[0] = reg&0x7E;
	tbuf[1] = value;
	
	uint32_t TXCnt = 0;
	uint32_t RXCnt = 0;

	*paddr = *paddr | 0x00000030; //clear
	*paddr = *paddr | 0x00000080;/*!< Transfer Active */

	while ((TXCnt < 2) || (RXCnt < 2))
	{
		/* TX fifo not full, so add some more bytes */
		while ((*paddr & 0x00040000) && (TXCnt < 2))
		{
			//  printk(KERN_ALERT "fifo : %x", *fifo);
			*fifo = tbuf[TXCnt];
			//    printk(KERN_ALERT "tbuf -> fifo : %x", *fifo);
			TXCnt++;
		}
		/* Rx fifo not empty, so get the next received bytes */
		while (((*paddr & 0x00020000)) && (RXCnt < 2))
		{
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			tbuf[RXCnt] = *fifo;
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			RXCnt++;
		}
	}
	/* Wait for DONE to be set */
	while (!(*paddr & 0x00010000));

	uint32_t v = *paddr;
	v = (v & ~0x00000080) | (0 & 0x00000080);
	*paddr = v;
}
void PCD_WriteRegister2(byte reg, byte count, byte *value){
	byte index = 0;
	for(index =0; index <count; index++){
		PCD_WriteRegister1(reg, value[index]);
	}

}

byte PCD_ReadRegister1(byte reg) {
	volatile uint32_t* paddr = spi_base + 0;
	volatile uint32_t* fifo = spi_base + 4;
	char tbuf[2];
	tbuf[0] = (reg&0x7E)|0x80;
	
	uint32_t TXCnt = 0;
	uint32_t RXCnt = 0;

	*paddr = *paddr | 0x00000030; //clear
	*paddr = *paddr | 0x00000080;/*!< Transfer Active */

	while ((TXCnt < 2) || (RXCnt < 2))
	{
		/* TX fifo not full, so add some more bytes */
		while ((*paddr & 0x00040000) && (TXCnt < 2))
		{
			//  printk(KERN_ALERT "fifo : %x", *fifo);
			*fifo = tbuf[TXCnt];
			//    printk(KERN_ALERT "tbuf -> fifo : %x", *fifo);
			TXCnt++;
		}
		/* Rx fifo not empty, so get the next received bytes */
		while (((*paddr & 0x00020000)) && (RXCnt < 2))
		{
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			tbuf[RXCnt] = *fifo;
			//printk(KERN_ALERT "rbuf[RxCnt] : %x", rbuf[RXCnt]);
			RXCnt++;
		}
	}
	/* Wait for DONE to be set */
	while (!(*paddr & 0x00010000));

	uint32_t v = *paddr;
	v = (v & ~0x00000080) | (0 & 0x00000080);
	*paddr = v;
	return tbuf[1];
}

void PCD_ReadRegister2(byte reg, byte count, byte* values, byte rxAlign){
	if (count == 0) {
		return;
	}
	//Serial.print(F("Reading ")); 	Serial.print(count); Serial.println(F(" bytes from register."));
	byte address = 0x80 | (reg & 0x7E);		// MSB == 1 is for reading. LSB is not used in address. Datasheet section 8.1.2.3.
	byte index = 0;							// Index in values array.
	count--;								// One read is performed outside of the loop
	spi_transfer(address);
	while (index < count) {
		if (index == 0 && rxAlign) {		// Only update bit positions rxAlign..7 in values[0]
		  // Create bit mask for bit positions rxAlign..7
			byte mask = 0;
			byte i = 0;
			for (i = rxAlign; i <= 7; i++) {
				mask |= (1 << i);
			}
			// Read value and tell that we want to read the same address again.
			byte value = spi_transfer(address);
			// Apply mask to both current value of values[0] and the new data in value.
			values[0] = (values[index] & ~mask) | (value & mask);
		}
		else { // Normal case
			values[index] = spi_transfer(address);
		}
		index++;
	}
	values[index] = spi_transfer(0);			// Read the final byte. Send 0 to stop reading.
}

void PCD_ClearRegisterBitMask(byte reg,	///< The register to update. One of the PCD_Register enums.
	byte mask	///< The bits to clear.
) {
	byte tmp;
	tmp = PCD_ReadRegister1(reg);
	PCD_WriteRegister1(reg, tmp & (~mask));		// clear bit mask
} // End PCD_ClearRegisterBitMask()
void PCD_SetRegisterBitMask(byte reg,	///< The register to update. One of the PCD_Register enums.
	byte mask	///< The bits to set.
) {
	byte tmp;
	tmp = PCD_ReadRegister1(reg);
	PCD_WriteRegister1(reg, tmp | mask);			// set bit mask
} // End PCD_SetRegisterBitMask()

byte PCD_CalculateCRC(byte* data,		///< In: Pointer to the data to transfer to the FIFO for CRC calculation.
	byte length,	///< In: The number of bytes to transfer.
	byte* result	///< Out: Pointer to result buffer. Result is written to result[0..1], low byte first.
) {
	PCD_WriteRegister1(CommandReg, PCD_Idle);		// Stop any active command.
	PCD_WriteRegister1(DivIrqReg, 0x04);				// Clear the CRCIRq interrupt request bit
	PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);		// FlushBuffer = 1, FIFO initialization
	PCD_WriteRegister2(FIFODataReg, length, data);	// Write data to the FIFO
	PCD_WriteRegister1(CommandReg, PCD_CalcCRC);		// Start the calculation

	// Wait for the CRC calculation to complete. Each iteration of the while-loop takes 17.73�s.
	word i = 5000;
	byte n;
	while (1) {
		n = PCD_ReadRegister1(DivIrqReg);	// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
		if (n & 0x04) {						// CRCIRq bit set - calculation done
			break;
		}
		if (--i == 0) {						// The emergency break. We will eventually terminate on this one after 89ms. Communication with the MFRC522 might be down.
			return STATUS_TIMEOUT;
		}
	}
	PCD_WriteRegister1(CommandReg, PCD_Idle);		// Stop calculating CRC for new content in the FIFO.

	// Transfer the result from the registers to the result buffer
	result[0] = PCD_ReadRegister1(CRCResultRegL);
	result[1] = PCD_ReadRegister1(CRCResultRegH);
	return STATUS_OK;
} // End PCD_CalculateCRC()
byte PCD_CommunicateWithPICC(byte command,		///< The command to execute. One of the PCD_Command enums.
	byte waitIRq,		///< The bits in the ComIrqReg register that signals successful completion of the command.
	byte* sendData,		///< Pointer to the data to transfer to the FIFO.
	byte sendLen,		///< Number of bytes to transfer to the FIFO.
	byte* backData,		///< NULL or pointer to buffer if data should be read back after executing the command.
	byte* backLen,		///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
	byte* validBits,	///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits.
	byte rxAlign,		///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
	int checkCRC		///< In: True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
) {
	byte n, _validBits;
	unsigned int i;

	// Prepare values for BitFramingReg
	byte txLastBits = validBits ? *validBits : 0;
	byte bitFraming = (rxAlign << 4) + txLastBits;		// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

	PCD_WriteRegister1(CommandReg, PCD_Idle);			// Stop any active command.
	PCD_WriteRegister1(ComIrqReg, 0x7F);					// Clear all seven interrupt request bits
	PCD_SetRegisterBitMask(FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
	PCD_WriteRegister2(FIFODataReg, sendLen, sendData);	// Write sendData to the FIFO
	PCD_WriteRegister1(BitFramingReg, bitFraming);		// Bit adjustments
	PCD_WriteRegister1(CommandReg, command);				// Execute the command
	if (command == PCD_Transceive) {
		PCD_SetRegisterBitMask(BitFramingReg, 0x80);	// StartSend=1, transmission of data starts
	}

	// Wait for the command to complete.
	// In PCD_Init() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
	// Each iteration of the do-while-loop takes 17.86�s.
	i = 2000;
	while (1) {
		n = PCD_ReadRegister1(ComIrqReg);	// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		if (n & waitIRq) {					// One of the interrupts that signal success has been set.
			break;
		}
		if (n & 0x01) {						// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}
		if (--i == 0) {						// The emergency break. If all other condions fail we will eventually terminate on this one after 35.7ms. Communication with the MFRC522 might be down.
			return STATUS_TIMEOUT;
		}
	}

	// Stop now if any errors except collisions were detected.
	byte errorRegValue = PCD_ReadRegister1(ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
	if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
		return STATUS_ERROR;
	}

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		n = PCD_ReadRegister1(FIFOLevelReg);			// Number of bytes in the FIFO
		if (n > * backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = n;											// Number of bytes returned
		PCD_ReadRegister2(FIFODataReg, n, backData, rxAlign);	// Get received data from FIFO
		_validBits = PCD_ReadRegister1(ControlReg) & 0x07;		// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (errorRegValue & 0x08) {		// CollErr
		return STATUS_COLLISION;
	}

	// Perform CRC_A validation if requested.
	if (backData && backLen && checkCRC) {
		// In this case a MIFARE Classic NAK is not OK.
		if (*backLen == 1 && _validBits == 4) {
			return STATUS_MIFARE_NACK;
		}
		// We need at least the CRC_A value and all 8 bits of the last byte must be received.
		if (*backLen < 2 || _validBits != 0) {
			return STATUS_CRC_WRONG;
		}
		// Verify CRC_A - do our own calculation and store the control in controlBuffer.
		byte controlBuffer[2];
		n = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
		if (n != STATUS_OK) {
			return n;
		}
		if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
			return STATUS_CRC_WRONG;
		}
	}

	return STATUS_OK;
} // End PCD_CommunicateWithPICC()

byte PCD_TransceiveData(byte* sendData,		///< Pointer to the data to transfer to the FIFO.
	byte sendLen,		///< Number of bytes to transfer to the FIFO.
	byte* backData,		///< NULL or pointer to buffer if data should be read back after executing the command.
	byte* backLen,		///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
	byte* validBits,	///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits. Default NULL.
	byte rxAlign,		///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
	int checkCRC		///< In: True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
) {
	byte waitIRq = 0x30;		// RxIRq and IdleIRq
	return PCD_CommunicateWithPICC(PCD_Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
} // End PCD_TransceiveData()

byte PICC_REQA_or_WUPA(byte command, 		///< The command to send - PICC_CMD_REQA or PICC_CMD_WUPA
	byte* bufferATQA,	///< The buffer to store the ATQA (Answer to request) in
	byte* bufferSize	///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
) {
	byte validBits;
	byte status;

	if (bufferATQA == NULL || *bufferSize < 2) {	// The ATQA response is 2 bytes long.
		return STATUS_NO_ROOM;
	}
	PCD_ClearRegisterBitMask(CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
	validBits = 7;									// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
	status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits, 0, 0);
	if (status != STATUS_OK) {
		return status;
	}
	if (*bufferSize != 2 || validBits != 0) {		// ATQA must be exactly 16 bits.
		return STATUS_ERROR;
	}
	return STATUS_OK;
} // End PICC_REQA_or_WUPA()
byte PICC_RequestA(byte* bufferATQA,	///< The buffer to store the ATQA (Answer to request) in
	byte* bufferSize	///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
) {
	return PICC_REQA_or_WUPA(PICC_CMD_REQA, bufferATQA, bufferSize);
} // End PICC_RequestA()
int PICC_IsNewCardPresent(void) {
	byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);
	byte result = PICC_RequestA(bufferATQA, &bufferSize);
	printk(KERN_ALERT "Ruquest Result is %d\n", result);
	return (result == STATUS_OK || result == STATUS_COLLISION);
} // End PICC_IsNewCardPresent()
byte PICC_Select(Uid* uid,			///< Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
	int validBits		///< The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
) {
	bool uidComplete;
	bool selectDone;
	bool useCascadeTag;
	byte cascadeLevel = 1;
	byte result;
	byte count;
	byte index;
	byte uidIndex;					// The first index in uid->uidByte[] that is used in the current Cascade Level.
	signed char currentLevelKnownBits;		// The number of known UID bits in the current Cascade Level.
	byte buffer[9];					// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
	byte bufferUsed;				// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
	byte rxAlign;					// Used in BitFramingReg. Defines the bit position for the first bit received.
	byte txLastBits;				// Used in BitFramingReg. The number of valid bits in the last transmitted byte. 
	byte* responseBuffer;
	byte responseLength;

	// Description of buffer structure:
	//		Byte 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
	//		Byte 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete bytes, Low nibble: Extra bits. 
	//		Byte 2: UID-data or CT		See explanation below. CT means Cascade Tag.
	//		Byte 3: UID-data
	//		Byte 4: UID-data
	//		Byte 5: UID-data
	//		Byte 6: BCC					Block Check Character - XOR of bytes 2-5
	//		Byte 7: CRC_A
	//		Byte 8: CRC_A
	// The BCC and CRC_A is only transmitted if we know all the UID bits of the current Cascade Level.
	//
	// Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
	//		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
	//		========	=============	=====	=====	=====	=====
	//		 4 bytes		1			uid0	uid1	uid2	uid3
	//		 7 bytes		1			CT		uid0	uid1	uid2
	//						2			uid3	uid4	uid5	uid6
	//		10 bytes		1			CT		uid0	uid1	uid2
	//						2			CT		uid3	uid4	uid5
	//						3			uid6	uid7	uid8	uid9

	// Sanity checks
	if (validBits > 80) {
		return STATUS_INVALID;
	}

	// Prepare MFRC522
	PCD_ClearRegisterBitMask(CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.

	// Repeat Cascade Level loop until we have a complete UID.
	uidComplete = false;
	while (!uidComplete) {
		// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
		switch (cascadeLevel) {
		case 1:
			buffer[0] = PICC_CMD_SEL_CL1;
			uidIndex = 0;
			useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
			break;

		case 2:
			buffer[0] = PICC_CMD_SEL_CL2;
			uidIndex = 3;
			useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
			break;

		case 3:
			buffer[0] = PICC_CMD_SEL_CL3;
			uidIndex = 6;
			useCascadeTag = false;						// Never used in CL3.
			break;

		default:
			return STATUS_INTERNAL_ERROR;
			break;
		}

		// How many UID bits are known in this Cascade Level?
		currentLevelKnownBits = validBits - (8 * uidIndex);
		if (currentLevelKnownBits < 0) {
			currentLevelKnownBits = 0;
		}
		// Copy the known bits from uid->uidByte[] to buffer[]
		index = 2; // destination index in buffer[]
		if (useCascadeTag) {
			buffer[index++] = PICC_CMD_CT;
		}
		byte bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
		if (bytesToCopy) {
			byte maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
			if (bytesToCopy > maxBytes) {
				bytesToCopy = maxBytes;
			}
			for (count = 0; count < bytesToCopy; count++) {
				buffer[index++] = uid->uidByte[uidIndex + count];
			}
		}
		// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
		if (useCascadeTag) {
			currentLevelKnownBits += 8;
		}

		// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
		selectDone = false;
		while (!selectDone) {
			// Find out how many bits and bytes to send and receive.
			if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
		  //Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
				buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
				// Calculate BCC - Block Check Character
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
				// Calculate CRC_A
				result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
				if (result != STATUS_OK) {
					return result;
				}
				txLastBits = 0; // 0 => All 8 bits are valid.
				bufferUsed = 9;
				// Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
				responseBuffer = &buffer[6];
				responseLength = 3;
			}
			else { // This is an ANTICOLLISION.
		  //Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
				txLastBits = currentLevelKnownBits % 8;
				count = currentLevelKnownBits / 8;	// Number of whole bytes in the UID part.
				index = 2 + count;					// Number of whole bytes: SEL + NVB + UIDs
				buffer[1] = (index << 4) + txLastBits;	// NVB - Number of Valid Bits
				bufferUsed = index + (txLastBits ? 1 : 0);
				// Store response in the unused part of buffer
				responseBuffer = &buffer[index];
				responseLength = sizeof(buffer) - index;
			}

			// Set bit adjustments
			rxAlign = txLastBits;											// Having a seperate variable is overkill. But it makes the next line easier to read.
			PCD_WriteRegister1(BitFramingReg, (rxAlign << 4) + txLastBits);	// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

			// Transmit the buffer and receive the response.
			result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign, 0);
			if (result == STATUS_COLLISION) { // More than one PICC in the field => collision.
				result = PCD_ReadRegister1(CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
				if (result & 0x20) { // CollPosNotValid
					return STATUS_COLLISION; // Without a valid collision position we cannot continue
				}
				byte collisionPos = result & 0x1F; // Values 0-31, 0 means bit 32.
				if (collisionPos == 0) {
					collisionPos = 32;
				}
				if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen 
					return STATUS_INTERNAL_ERROR;
				}
				// Choose the PICC with the bit set.
				currentLevelKnownBits = collisionPos;
				count = (currentLevelKnownBits - 1) % 8; // The bit to modify
				index = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
				buffer[index] |= (1 << count);
			}
			else if (result != STATUS_OK) {
				return result;
			}
			else { // STATUS_OK
				if (currentLevelKnownBits >= 32) { // This was a SELECT.
					selectDone = true; // No more anticollision 
					// We continue below outside the while.
				}
				else { // This was an ANTICOLLISION.
				  // We now have all 32 bits of the UID in this Cascade Level
					currentLevelKnownBits = 32;
					// Run loop again to do the SELECT.
				}
			}
		} // End of while (!selectDone)

		// We do not check the CBB - it was constructed by us above.

		// Copy the found UID bytes from buffer[] to uid->uidByte[]
		index = (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		bytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < bytesToCopy; count++) {
			uid->uidByte[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
			return STATUS_ERROR;
		}
		// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
		result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
		if (result != STATUS_OK) {
			return result;
		}
		if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
			return STATUS_CRC_WRONG;
		}
		if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
			cascadeLevel++;
		}
		else {
			uidComplete = true;
			uid->sak = responseBuffer[0];
		}
	} // End of while (!uidComplete)

	// Set correct uid->size
	uid->size = 3 * cascadeLevel + 1;

	return STATUS_OK;
} // End PICC_Select()

int PICC_ReadCardSerial(void) {
	byte result = PICC_Select(&uid, 0);
	printk(KERN_ALERT "The Readcard result is : %d\n", result);
	return (result == STATUS_OK);
} // End PICC_ReadCardSerial()
int rfid_open(struct inode* inode, struct file* flip) {
	printk(KERN_ALERT "rfid open function called\n");

	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	spi_base = ioremap(SPI_BASE_ADDR,0x18);
	
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gpset1 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr1 = (volatile unsigned int *)(gpio_base + GPCLR0);
	gplev = (volatile unsigned int *)(gpio_base + GPLEV0);
	*gpsel0 = (1<<23);
	*gpsel0 |= (1<<26);
	*gpsel0 |= (1<<29);
	*gpsel1 = (1<<2);
	*gpsel1 |= (1<<5);
	PCD_WriteRegister1(CommandReg, PCD_SoftReset);
	printk(KERN_ALERT "not in the while.......");
	while(PCD_ReadRegister1(CommandReg)&(1<<4)){
	printk(KERN_ALERT "in the while.......");
	}
	PCD_WriteRegister1(TModeReg, 0x80);			// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
  	PCD_WriteRegister1(TPrescalerReg, 0xA9);		// TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25�s.
 	PCD_WriteRegister1(TReloadRegH, 0x03);		// Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
 	PCD_WriteRegister1(TReloadRegL, 0xE8);
 	PCD_WriteRegister1(TxASKReg, 0x40);		// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
 	PCD_WriteRegister1(ModeReg, 0x3D);	
	byte value = PCD_ReadRegister1(TxControlReg);
	if((value&0x03) != 0x03){
	PCD_WriteRegister1(TxControlReg, value|0x03);
}
	return 0;
}



int rfid_release(struct inode* inode, struct file* flip) {
	printk(KERN_ALERT "rfid release function called\n");
	return 0;
}

ssize_t rfid_read(struct file* flip, char* buf, size_t count, loff_t* f_pos) {
	int int_send_uid = 0;
	int send_uid[4] = {0,0,0,0};
	if(PICC_IsNewCardPresent() == 1){
	if(PICC_ReadCardSerial() == 1){
	printk(KERN_ALERT "the UID is : %s\n",uid.uidByte);
	send_uid[0] = (uid.uidByte)[0];
	send_uid[1] = (uid.uidByte)[1];
	send_uid[2] = (uid.uidByte)[2];
	send_uid[3] = (uid.uidByte)[3];
	int_send_uid = (send_uid[0]<<24) + (send_uid[1]<<16) + (send_uid[2]<<8) + send_uid[3];
	copy_to_user(buf, &int_send_uid, 4);
	} 
	}
	printk(KERN_ALERT "rfid read funtion called\n");
	return count;
}

static struct file_operations sys_fops = {
	.owner = THIS_MODULE,
	.read = rfid_read,
	.open = rfid_open,
	.release = rfid_release
};

int __init rfid_init(void) {
	if (register_chrdev(RFID_MAJOR_NUMBER, RFID_DEV_NAME, &sys_fops) < 0)
		printk(KERN_ALERT "rfid driver init failed\n");
	else
		printk(KERN_ALERT "rfid driver init successful\n");
	return 0;
}

void __exit rfid_exit(void) {
	unregister_chrdev(RFID_MAJOR_NUMBER, RFID_DEV_NAME);
	printk(KERN_ALERT "rfid driver cleanup\n");
}

module_init(rfid_init);
module_exit(rfid_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Byeongjun Park");
MODULE_DESCRIPTION("This is the hello world example for device driver in system programming lecture");
