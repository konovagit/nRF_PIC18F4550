#include <xc.h>
#include "nRF2401.h"
#include <plib/spi.h>


unsigned char nrf_getStatus(void) {
    unsigned char status;
    CSN = nrf_CLEAR;
    SSPBUF = 0xFF;
    while(!SSPSTATbits.BF);
    status = SSPBUF;
    CSN = nrf_SET;
    return status;
}

unsigned char nrf_send(unsigned char * tx_buf, unsigned char * rx_buf) {
    char status;
    int i;

    nrf_SPI_RW_Reg(FLUSH_TX,0);
    
    status = nrf_getStatus();
    nrf_SPI_RW_Reg(WRITE_REG + STATUS_REG, status);	//nrf_CLEAR max RT bit
    nrf_SPI_Write_Buf(WR_TX_PLOAD,tx_buf,TX_PLOAD_WIDTH); //load the data into the NRF

    //wait for response
    CE = nrf_SET;
    for(i=0; i<2000; i++);
    CE = nrf_CLEAR;

    status = nrf_getStatus();
    if(status & RX_DR) {
        nrf_SPI_RW_Reg(WRITE_REG + STATUS_REG, RX_DR);		
        nrf_SPI_Read_Buf(RD_RX_PLOAD,rx_buf,ACK_PAYLOAD);
        nrf_SPI_RW_Reg(FLUSH_RX,0);
        return YES_ACK;
    } else {
        return NO_ACK;
    }
}

void nrf_send_noack(unsigned char * tx_buf, unsigned char * rx_buf) {
    char status;
    int i;

    nrf_SPI_RW_Reg(FLUSH_TX,0);

    status = nrf_getStatus();
    nrf_SPI_RW_Reg(WRITE_REG + STATUS_REG, status);	//nrf_CLEAR max RT bit
    nrf_SPI_Write_Buf(WR_TX_PLOAD_NOACK,tx_buf,TX_PLOAD_WIDTH); //load the data into the NRF

    CE = nrf_SET;
}

unsigned char nrf_receive(unsigned char * tx_buf, unsigned char * rx_buf) {
    char status;
    char ffstat;
    char config;
    char ffstatcount;

    //------ load ACK payload data -------------

    nrf_SPI_RW_Reg(FLUSH_TX,0);
    nrf_SPI_Write_Buf(W_ACK_PAYLOAD,tx_buf,ACK_PAYLOAD);

    //config = nrf_SPI_Read(CONFIG);

    status = nrf_getStatus();
    ffstat = nrf_SPI_Read(FIFO_STATUS);

    if(((status & RX_DR))||(!(ffstat & 0x01))) {
        ffstatcount = 0;
        while((ffstatcount++ < 4) && ((ffstat & 0x01) == 0)) {
            //read entire buffer---------
            nrf_SPI_Read_Buf(RD_RX_PLOAD,rx_buf,32);
            ffstat = nrf_SPI_Read(FIFO_STATUS);
        }
        nrf_SPI_RW_Reg(WRITE_REG + STATUS_REG, 0x70);	//nrf_CLEAR all flags
        return YES_DATA;
    } else {
        return NO_DATA;
    }
}

void nrf_init(void) {
    char status=0;
    unsigned char Test[5];
    OpenSPI(SPI_FOSC_64, MODE_00, SMPMID);
    
    CE = nrf_SET;                           //default to Standby II, nrf_CLEAR to default to Standby I (which is low power mode; no TX/RX functions)
    CSN = nrf_SET;	
 
    CE = nrf_CLEAR;
     
    nrf_SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // Writes TX_Address to nRF24L01
  
    nrf_SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // Use the same address on the RX device as the TX device

    /*
    CSN = nrf_CLEAR;
    WriteSPI(0x10);
    for(int i=0; i<5; i++)
    {
        Test[i]=ReadSPI();
    }
    CSN = nrf_SET;
    */
    
    nrf_SPI_RW_Reg(ACTIVATE,0x73);					//activate feature register
    
    nrf_SPI_RW_Reg(WRITE_REG + FEATURE, 0x06);	        	//nrf_SET features for DPL
    
    nrf_SPI_RW_Reg(WRITE_REG + DYNPD,     0b111111);		//enable DPL on all pipes

    nrf_SPI_RW_Reg(WRITE_REG + EN_AA,     0b111111);            // Enable Auto.Ack:(all pipes)
    
    nrf_SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0b0001);              // Enable Pipe0
    
    nrf_SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x12); // 500us + 86us, 2 retrans...
    
    nrf_SPI_RW_Reg(WRITE_REG + RF_CH, 40);        // Select RF channel 40
    
    nrf_SPI_RW_Reg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width
    
    nrf_SPI_RW_Reg(WRITE_REG + RX_PW_P1, TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width
    
    nrf_SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   // TX_PWR:0dBm, Datarate:1Mbps, LNA:HCURR

    nrf_SPI_RW_Reg(FLUSH_RX,0);
   
    nrf_SPI_RW_Reg(FLUSH_TX,0);

    status=nrf_SPI_Read(STATUS);
    
    nrf_SPI_RW_Reg(WRITE_REG + STATUS, status);
 
    nrf_SPI_RW_Reg(WRITE_REG + SETUP_AW, 0x3);

    CE = nrf_SET;
}

//Wait for 5ms after calling this before calling a send/receive
void nrf_rxmode(void) {
    CE = nrf_CLEAR;

    nrf_SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);     // Set PWR_UP bit, enable CRC(2 unsigned chars) & Prim:RX. RX_DR enabled..
    
    CE = nrf_SET;
}

//Wait for 5ms after calling this before calling a send/receive
void nrf_txmode(void) {
    CE = nrf_CLEAR;
    
    nrf_SPI_RW_Reg(WRITE_REG + CONFIG, 0x0E);     // Set PWR_UP bit, enable CRC(2 unsigned chars) & Prim:TX. MAX_RT & TX_DS enabled..
    
    CE = nrf_SET;
}

void nrf_powerdown(void) {
    CE = nrf_CLEAR;

    nrf_SPI_RW_Reg(WRITE_REG + CONFIG, 0x0C);     // Clear PWR_UP bit

    CE = nrf_SET;
}

void nrf_setTxAddr(char addr) {
    CE = nrf_CLEAR;
    __delay_ms(1);
    TX_ADDRESS[0] = addr;
    nrf_SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);
    CE = nrf_SET;
}

void nrf_setRxAddr(char pipe, char addr) {
    CE = nrf_CLEAR;
    __delay_ms(1);
    TX_ADDRESS[0] = addr;
    nrf_SPI_Write_Buf(WRITE_REG + RX_ADDR_P0 + pipe, TX_ADDRESS, TX_ADR_WIDTH);
    CE = nrf_SET;
}

char nrf_enablePipe(char pipe) {
    char pipeStatus;
    
    CE = nrf_CLEAR;
    __delay_ms(1);
    pipeStatus = nrf_SPI_Read(EN_RXADDR);
  
    pipeStatus = pipeStatus | (0b1 << pipe);
    nrf_SPI_RW_Reg(WRITE_REG + EN_RXADDR, pipeStatus);
    
    CE = nrf_SET;
    return pipeStatus;
}

char nrf_disablePipe(char pipe) {
    char pipeStatus;
    
    CE = nrf_CLEAR;
    __delay_ms(1);
    pipeStatus = nrf_SPI_Read(EN_RXADDR);
    pipeStatus = pipeStatus & ~(0b1 << pipe);
    nrf_SPI_RW_Reg(WRITE_REG + EN_RXADDR, pipeStatus);
    CE = nrf_SET;
    return pipeStatus;
}

char nrf_readRegister(char loc) {
    return nrf_SPI_Read(loc);
}









/////////////////////////////////////////////////
//  Internal Functions
/////////////////////////////////////////////////

/**************************************************
 * Function: nrf_SPI_RW();
 * 
 * Description:
 * Writes one unsigned char to nRF24L01, and return the unsigned char read
 * from nRF24L01 during write, according to SPI protocol
 **************************************************/
unsigned char nrf_SPI_RW(unsigned char data) {
    SPI_BUFFER = data;
    while(!SPI_BUFFER_FULL_STAT);
    data = SPI_BUFFER;
    return(data);
}
/**************************************************/

/**************************************************
 * Function: nrf_SPI_RW_Reg();
 * 
 * Description:
 * Writes value 'value' to register 'reg'
 * must be used along with the WRITE mask
**************************************************/
unsigned char nrf_SPI_RW_Reg(unsigned char reg, unsigned char value) {
    unsigned char status;

    CSN = nrf_CLEAR;                   // CSN low, init SPI transaction
    status = nrf_SPI_RW(reg);             // select register
    nrf_SPI_RW(value);                    // ..and write value to it..
    CSN = nrf_SET;                    // CSN high again

    return status;                   // return nRF24L01 status unsigned char
}
/**************************************************/

/**************************************************
 * Function: nrf_SPI_Read();
 * 
 * Description:
 * Read one unsigned char from nRF24L01 register, 'reg'
**************************************************/
unsigned char nrf_SPI_Read(unsigned char reg) {
    unsigned char reg_val;

    CSN = nrf_CLEAR;                // CSN low, initialize SPI communication...
    nrf_SPI_RW(reg);                   // Select register to read from..
    reg_val = nrf_SPI_RW(0);           // ..then read register value
    CSN = nrf_SET;                  // CSN high, terminate SPI communication

    return(reg_val);               // return register value
}
/**************************************************/

/**************************************************
 * Function: nrf_SPI_Read_Buf();
 * 
 * Description:
 * Reads 'unsigned chars' #of unsigned chars from register 'reg'
 * Typically used to read RX payload, Rx/Tx address
**************************************************/
unsigned char nrf_SPI_Read_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes) {
    unsigned char status,i;

    CSN = nrf_CLEAR;                   // Set CSN low, init SPI tranaction
    status = nrf_SPI_RW(reg);       	    // Select register to write to and read status unsigned char

    for(i=0;i<bytes;i++) {
        pBuf[i] = nrf_SPI_RW(0xFF);    // Perform nrf_SPI_RW to read unsigned char from nRF24L01
    }

    CSN = nrf_SET;                   // Set CSN high again

    return(status);                  // return nRF24L01 status unsigned char
}
/**************************************************/

/**************************************************
 * Function: nrf_SPI_Write_Buf();
 * 
 * Description:
 * Writes contents of buffer '*pBuf' to nRF24L01
 * Typically used to write TX payload, Rx/Tx address
**************************************************/
unsigned char nrf_SPI_Write_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes) {
    unsigned char status,i;

    CSN = nrf_CLEAR;                   // Set CSN low, init SPI tranaction
    status = nrf_SPI_RW(reg);             // Select register to write to and read status unsigned char
    for(i=0;i<bytes; i++) {             // then write all unsigned char in buffer(*pBuf)
        nrf_SPI_RW(*pBuf);
        *pBuf++;
    }
    CSN = nrf_SET;                   // Set CSN high again
    return(status);                  // return nRF24L01 status unsigned char
}
/**************************************************/
