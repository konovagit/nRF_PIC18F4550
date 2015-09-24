#include <p18f4550.h>
#define _XTAL_FREQ 8000000 

// Pin Definitions
#define CE			PORTCbits.RC0
#define TRIS_CE		TRISCbits.TRISC0
#define CSN			PORTCbits.RC1
#define TRIS_CSN	TRISCbits.TRISC1
#define IRQ			PORTCbits.RC2
#define TRIS_IRQ	TRISCbits.TRISC2

#define TRIS_SCK	TRISBbits.TRISB1
#define TRIS_MISO	TRISBbits.TRISB0
#define TRIS_MOSI	TRISCbits.TRISC7

//superceded by the hardwareprofile.h
#define SPI_BUFFER_FULL_STAT    SSPSTATbits.BF
#define SPI_BUFFER              SSPBUF

#define SPI_STATUS				SSPSTAT
#define SPI_CLK_EDGE			SSPSTATbits.CKE //clock edge
#define SPI_CLK_POL				SSPCON1bits.CKP //clock polarity
#define SPI_CONFIG_1			SSPCON1
#define SPI_ENABLE				SSPCON1bits.SSPEN

#define SPI_CONFIG_1_VALUE  0b00100001
