/* 
 * File:   main.c
 * Author: matt
 *
 * Created on 08 June 2014, 15:38
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <plib.h>
#include <math.h>

#include "colour_map.h"
#include "splash_img.h"

//80Mhz
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1

//40Mhz
//#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_2, FWDTEN = OFF
//#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1

#define WS2812

#define	GetPeripheralClock()		(SYS_FREQ/(1 << OSCCONbits.PBDIV))

//#define DEBUG

#define SYS_FREQ (80000000L)

//Refresh Definitions
#define FRAME_REFRESH_FREQ 50
#define FRAME_REFRESH_CNT  ((SYS_FREQ/2/FRAME_REFRESH_FREQ)-8890)
#define TIMEOUT_CNT 4*FRAME_REFRESH_CNT  //Lazyness make it 40ms Timeout

#define UART_DATA_AVAIL UARTReceivedDataIsAvailable ( UART2 )
#define UART_STALL BIT_10

#define APFBUFFSIZE 768

#define SPI_HIGH 0xE0000000 //This will be sent for highs
#define SPI_LOW 0x80000000 //This will be sent for lows
#define SPI_BITSPERBIT 4 //Equals the numbers of bits defined above

//Mul25 Lookup array to avoid Multiplies
uint32_t mul25lu[] = {  0,  25,  50,  75,
                      100, 125, 150, 175,
                      200, 225, 250, 275,
                      300, 325, 350, 375,
                      400 };

uint32_t mul16lu[] = {  0,  16,  32,  48,
                       64,  80,  96, 112,
                      128, 144, 160, 176,
                      192, 208, 224, 240,
                      256 };

//Mul6 Lookup Array to avoid Multiplies
uint32_t mul6lu[] = {  0,  6,  12,  18};

//Mul256 Lookip Array to avoid Multiplies
#define MUL256(X) (X<<8)

/* UnpaddedFrameBuffer Upfb[N][256]
 * Holds N *Compressed* Frames at a time straight from UART
 * Each Frame contains 256 * 32bit words in the format:
 *
 * 31                          0
 *  -----------------------------
 *  | 0x00 |  Rn  |  Gn  |  Bn  |
 *  -----------------------------
 *     8b     8b     8b     8b
 *
 * Each 32bit word is a pixel, where index   0 -> 15 is row one
 *                                   index 240 -> 255 is row 16
 *
 * Pfb         : Two Dimensional Array containing the Data
 * PfbCount    : Number of valid Entries in PFB
 * PfbNextFree : Points to the next free empty in the Pfb
 * PfbHead     : Head of the Pfb, next to be filled into none active frame
 * PfbFull     : Is Pfb Buffer Full
 */

#define PFB_FRAMES 10

uint8_t  PfbFull = 0;
uint8_t  PfbNextFree = 0;
uint8_t  PfbHead = 0;
uint8_t  PfbCount = 0;
uint32_t Pfb [PFB_FRAMES][256];

void pfbFlushBuffers(void) {
uint32_t i = 0; uint32_t j = 0;

//Init and Clear Pfb
PfbNextFree = 0;
PfbHead = 0;
PfbFull = 0;
PfbCount = 0;

for (i = 0; i < PFB_FRAMES; i++)
    for (j =0; j < 256; j++)
        Pfb[i][j] = 0;
}

void pfbPush(void) {
    int tmpNext;
    //Choose Next Free Slot
    tmpNext = PfbNextFree + 1;
    PfbNextFree = (tmpNext == PFB_FRAMES) ? 0 : tmpNext; // Roll Round
    ++PfbCount; //Increment Count
    PfbFull = (PfbCount >= PFB_FRAMES) ? 1 : 0; // If PfbCount == PFB_FRAMES then Full
}

void pfbPop(void) {
    int tmpHead;
    //Choose Next Free Slot
    tmpHead = PfbHead + 1;
    PfbHead = (tmpHead == PFB_FRAMES) ? 0 : tmpHead; // Roll Round
    --PfbCount; //Increment CountI
    PfbFull = (PfbCount >= PFB_FRAMES) ? 1 : 0; // If PfbCount == PFB_FRAMES then Full
}

void pfbDrawPixel( uint32_t * pfb , uint8_t x, uint8_t y, uint32_t colour) {
    pfb[mul16lu[y]+x] = colour;
}
inline void pfbDrawVertLine( uint32_t * pfb , uint8_t x, uint8_t y0, uint8_t y1, uint32_t colour) {
    uint8_t i, ytmp;
    if( y1 < y0 ) { ytmp = y0; y0=y1; y1=ytmp;}
    for(i=y0; i<=y1; i++)
        pfb[mul16lu[i]+x] = colour;
}
inline void pfbDrawHorizLine( uint32_t * pfb , uint8_t x0, uint8_t x1, uint8_t y, uint32_t colour) {
    uint8_t i, xtmp;
    uint32_t y_base = mul16lu[y];
    if( x1 < x0 ) { xtmp = x0; x0=x1; x1=xtmp;}
    for(i=x0; i<=x1; i++)
        pfb[y_base+i] = colour;
}
//TO BE IMPLEMENTED
inline void pfbDrawLine( uint32_t * pfb , uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint32_t colour) {

}
inline void pfbDrawRectangle( uint32_t * pfb , uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint32_t line_colour) {
    uint8_t i, xytmp;
    if( x1 < x0 ) { xytmp = x0; x0=x1; x1=xytmp;}
    if( y1 < y0 ) { xytmp = y0; y0=y1; y1=xytmp;}
    pfbDrawHorizLine(pfb,x0,x1,y0,line_colour);
    pfbDrawHorizLine(pfb,x0,x1,y1,line_colour);
    pfbDrawVertLine(pfb,x0,y0,y1,line_colour);
    pfbDrawVertLine(pfb,x1,y0,y1,line_colour);
}
//TO BE IMPLEMENTED
inline void pfbDrawFilledRectangle( uint32_t * pfb , uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint32_t line_colour, uint32_t fill_colour) {

}
//TO BE IMPLEMENTED
inline void pfbDrawFont( uint32_t * pfb , uint8_t x0, uint8_t x1, uint8_t letter, uint32_t colour) {

}



/*ActivePaddedFrames
 * Holds 2 *Uncompressed* Frames ready to be sent via SPI to LED Matrix
 *
 * When in SCAN mode (!WS2812) extra padding needed for unused PWM channels
 * and extra 4 precision bits padding per LED.
 *
 * When in WS2812 mode each bit of pixel data is expanded to 4 bits to fufil
 * WS2812 Timing requirements.
 *
 * There are two of these Frames,
 * One active being used to drive the Display, The other is being filled if Pfb is not empty
 *
 * SCAN:
 * where R = NextRow to SPI 
 * Row Data can be found between 25R -> 25R+23,
 * Row Select can be found in    25R+24,
 *
 * WS2812:
 *
 *
 * Apf            : Two Dimensional Array containing the uncompressed data.
 * ApfNextRow     : The next row to be sent via SPI from the Active Frame
 * ApfActiveFrame : Which of the two ActivePaddedFrames is currently displayed
 */
uint8_t  ApfActiveFrame = 0;
uint8_t  ApfNextRow = 0;
uint8_t  ApfSwitchPending = 0;

uint32_t Apf [2][APFBUFFSIZE];

void apfFlushBuffers(void) {
uint32_t i = 0; uint32_t j = 0; uint32_t row = 0; uint32_t row_reord = 0 ;

//Init and Clear Apf
ApfActiveFrame = 0;
ApfNextRow = 0;
ApfSwitchPending = 0;
for (i = 0; i < 2; i++)
    for (j = 0; j < APFBUFFSIZE; j++)
        Apf[i][j] = 0;

}

inline uint32_t colorMap8_8Func (uint32_t a) {
    uint32_t tmpR, tmpG, tmpB;
    tmpR = colorMap8_8[(a & 0xFF000000) >> 24];
    tmpG = colorMap8_8[(a & 0x000FF000) >> 12];
    tmpB = colorMap8_8[ a & 0x000000FF];
    return tmpB | tmpG << 12 | tmpR << 24;
}

void apfPack(uint32_t* unpadded_i, uint32_t* padded_o) {
    char spipos = 0;
    uint32_t *p, *buf;
    uint32_t pixel;
    uint32_t i;
    uint32_t j;
    uint32_t dir;
    int32_t bitpos;

    buf = unpadded_i;
    p = padded_o;

    *p = 0;

    dir = 0; // 0=Inc Ptr !0=Dec Ptr
    for (i = 0; i < 16; i++) {
        dir = i & 1; //If Odd Row, Buff Dec else Buff Inc
        for (j = 0; j < 16; j++) {

            pixel = *buf;
            //alterGamma(&pixel);

            for (bitpos = 15; bitpos != -1; bitpos--) {

                if ((pixel & (0x00000001 << bitpos)) > 0) {
                    *p |= (SPI_HIGH >> spipos);
                } else {
                    *p |= (SPI_LOW >> spipos);
                }

                spipos += SPI_BITSPERBIT;

                if (spipos > 31) {
                    *(++p) = 0;
                    spipos = spipos - 32;
                }

                //To Account for 0x00RRGGBB rather than 0x00GGRRBB
                if(bitpos == 8){ bitpos  = 24; } //Start at 23
                if(bitpos == 16){ bitpos  = 8; } // Start at 7
            }
            if (dir) buf--;
            else     buf++;
           // buf++;
        }
        if (dir) buf +=17;
        else     buf +=15;
    }
}

    //RX Statemachine
    typedef enum {CMD_SM, DECODE_CMD_SM, DECODE_DRAW_SM, DECODE_STREAM_SM } RxSm_t;
    RxSm_t RxState = CMD_SM;
    uint8_t RxStateReset = 0;

    //RX Transactions
    //2 Byte Command Transactions {'C', <ENUM>}
    typedef enum {PING_CMD=0, START_DRAW_CMD=1, START_STREAM_CMD=2, FLUSH_BUFFERS_CMD=4,
                  GET_LIGHT_CMD=8, GET_TEMP_CMD=16} RxCmd_t;
    //4 Byte Response Transactions {'R', <ENUM>, VALUE[1:0]}
    typedef enum {PING_RSP=0, ACK_RSP=1, ERR_RSP=2, TEMP_RSP=4, LIGHT_RSP=8} RxRsp_t;
    //8 Byte Draw Transactions {'D', <ENUM>, x0x1, y0y1, COLOUR[3:0]}
    typedef enum {PIXEL_DRAW=0, HLINE_DRAW=1, VLINE_DRAW=2, LINE_DRAW=4, FONT_DRAW=8,
                  RECT_DRAW=16} RxDrawCmd_t; //8Byte

    volatile uint8_t uartStall = 0;

inline uint8_t uartGetByte(void){
   uint8_t a = 0;
   a = UARTGetDataByte(UART2);
   return a;
}
void uartSendRsp(RxRsp_t rspType, uint16_t payload) {
    uint8_t i;
    uint8_t buffer[] = {'R',
                        (uint8_t)rspType,
                        (uint8_t)((payload & 0xFF00)>>8),
                        (uint8_t)(payload & 0x00FF)
                       };

    for (i = 0; i<4;i++) {
       while(!UARTTransmitterIsReady(UART2));
       UARTSendDataByte(UART2, buffer[i]);
    }
}
void uartFlushFifo(void) {
    uint8_t dump;
    while(UARTReceivedDataIsAvailable(UART2))
        dump = UARTGetDataByte(UART2);
}
void uartDebug(uint8_t* mess, uint8_t messLen){
#ifdef DEBUG
    uint8_t i;
    while(!UARTTransmitterIsReady(UART2));
    UARTSendDataByte(UART2,'D');
    while(!UARTTransmitterIsReady(UART2));
    UARTSendDataByte(UART2,':');

    for (i = 0; i<messLen;i++) {
        while(!UARTTransmitterIsReady(UART2));
        UARTSendDataByte(UART2, mess[i]);
    }
    
    while(!UARTTransmitterIsReady(UART2));
    UARTSendDataByte(UART2, '\r');
    while(!UARTTransmitterIsReady(UART2));
    UARTSendDataByte(UART2, '\n');
#endif
}

inline void uartSetStall( void ) {
    mPORTBSetBits(UART_STALL);
    uartStall = 1;
}

inline void uartClearStall (void){
    mPORTBClearBits(UART_STALL);
    uartStall = 0;
}

uint8_t dcpParse(uint32_t* pfb, uint64_t* dcPtr) {
    uint8_t term = 0;
    uint8_t cmdPre, cmdX0, cmdY0, cmdX1, cmdY1, cmdLet;
    uint32_t cmdCol;
    RxDrawCmd_t cmdType;

    if (*dcPtr == 0xFFFF000000000000) return 1; //MIGHT NEED CORRECTING
    cmdType = (RxDrawCmd_t)(*dcPtr >> 56);
    cmdX0   = (uint8_t)((*dcPtr >> 52) & 0x0F);
    cmdY0   = (uint8_t)((*dcPtr >> 46) & 0x0F);
    cmdX1   = (uint8_t)((*dcPtr >> 40) & 0x0F);
    cmdY1   = (uint8_t)((*dcPtr >> 34) & 0x0F);
    cmdLet  = (uint8_t)((*dcPtr >> 36) & 0xFF);
    cmdCol  = (uint32_t)(*dcPtr & 0xFFFFFFFF);

    switch(cmdType)
    {
        case PIXEL_DRAW: // {PIXEL_DRAW, x0_0y0,y00_x1,0y10, 0xRR0GG0BB}
            pfbDrawPixel (pfb,cmdX0,cmdY0,cmdCol);
            break;
        case HLINE_DRAW: // {HLINE_DRAW, x0_0y0,y00_x1,0000, 0xRR0GG0BB}
            pfbDrawHorizLine (pfb,cmdX0,cmdX1,cmdY0,cmdCol);
            break;
        case VLINE_DRAW: // {VLINE_DRAW, x0_0y0,y00_00,0y10, 0xRR0GG0BB}
            pfbDrawVertLine (pfb,cmdX0,cmdY0,cmdY1,cmdCol);
            break;
        case LINE_DRAW:  // {LINE_DRAW,  x0_0y0,y00_x1,0y10,, 0xRR0GG0BB}
            pfbDrawLine (pfb,cmdX0,cmdX1,cmdY0,cmdY1,cmdCol);
            break;
        case FONT_DRAW:  // {FONT_DRAW,  x0_0y0,y00_L1,L200,  0xRR0GG0BB}
            pfbDrawFont (pfb,cmdX0,cmdX1,cmdLet,cmdCol);
            break;
        case RECT_DRAW:  // {RECT_DRAW,  x0_0y0,y00_x1,0y10, 0xRR0GG0BB}
            pfbDrawRectangle (pfb,cmdX0,cmdX1,cmdY0,cmdY1,cmdCol);
            break;
        default:
            //Do Nothing as unknown command
            break;
    }
    return term;
}

DmaChannel chn0 = DMA_CHANNEL0;
DmaChannel chn1 = DMA_CHANNEL1;
DmaChannel chn2 = DMA_CHANNEL2;
DmaChannel chn3 = DMA_CHANNEL3;
uint8_t    dmaRxComplete = 0;
uint8_t    dmaRxLast = 0;
uint8_t    dmaTxOffIdx = 0;
uint32_t*  dmaTxFramePtr = 0;
uint32_t*  dmaRxPfb;

void dmaStreamSetup( uint32_t* pfb ){
   dmaRxComplete = 0;
   dmaRxLast = 0;
   dmaRxPfb  = pfb;
   // Configure the dma channels to chain
   DmaChnOpen(chn2,DMA_CHN_PRI0,DMA_OPEN_DEFAULT);
   DmaChnOpen(chn3,DMA_CHN_PRI0,DMA_OPEN_CHAIN_HI);
   //UART2 rx interrupt to start transfer, stops after 1024KB has be transferered
   DmaChnSetEventControl(chn2, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_UART2_RX_IRQ));
   DmaChnSetEventControl(chn3, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_UART2_RX_IRQ));
   // set the transfer source and dest addresses, source and dest sizes and the cell size
   DmaChnSetTxfer(chn2, (void*)&U2RXREG, (void*)dmaRxPfb,     1, 256, 1);
   DmaChnSetTxfer(chn3, (void*)&U2RXREG, (void*)dmaRxPfb+256, 1, 256, 1);
   //DmaChnSetTxfer(chn2, (void*)&U2RXREG, (void*)pfb+512, 1, 256, 1);
   //DmaChnSetTxfer(chn3, (void*)&U2RXREG, (void*)pfb+768, 1, 256, 1);
   DmaChnSetEvEnableFlags(chn3, DMA_EV_BLOCK_DONE); // enable the transfer done interrupt on final chained dma

   // Set up DMA Block Complete interrupt with a priority of 7 and zero sub-priority 
   INTSetVectorPriority(INT_DMA_3_VECTOR, INT_PRIORITY_LEVEL_7);
   INTSetVectorSubPriority(INT_DMA_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
   INTEnable(INT_DMA3, INT_ENABLED);

   // Enable multi-vector interrupts
   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
   INTEnableInterrupts();
   // enable the chn0 to start the DMA Chain
   DmaChnEnable(chn2);
}

void dmaDrawSetup( uint8_t* dcbuff ){
  // configure the channel
  /*  DmaChnOpen(chn, DMA_CHN_PRI2, DMA_OPEN_MATCH);
  //UART2 rx interrupt to start transfer, stops upon detection of 0xFFFF
  DmaChnSetMatchPattern(chn, 0xFFFF); // set \r as ending character
  DCH1CON |= 1<<11;//Dual Byte Match Mode?
  DmaChnSetEventControl(chn, DMA_EV_START_IRQ_EN | DMA_EV_MATCH_EN | DMA_EV_START_IRQ(_UART2_RX_IRQ));
  // set the transfer source and dest addresses, source and dest sizes and the cell size
  DmaChnSetTxfer(chn, (void*) &U2RXREG, dcbuff, 1, sizeof (dcbuff), 1);
  //DmaChnSetEvEnableFlags(chn, DMA_EV_BLOCK_DONE); // enable the transfer done interrupt: pattern match or all the characters transferred
  // enable the chn
  DmaChnEnable(chn);
  */
}


// Hardware Setup

void setupSPI(void){
    // 32 bits/char, input data sampled at end of data, Try inverted Clock
    SpiOpenFlags oFlags=SPI_OPEN_MODE32 |SPI_OPEN_MSTEN;
    // Open SPI module, use SPI channel 1, use flags set above, Divide Fpb by 4
    SpiChnOpen(SPI_CHANNEL2, oFlags, 24); //18/16 gives 833Khz
}


void setupUART(void){

    // Use PortB Pin to signal UART Stall through CTS faking when PFB is full.
    mPORTBClearBits(UART_STALL);
    mPORTBSetPinsDigitalOut(UART_STALL);

    UARTConfigure(UART2, UART_ENABLE_PINS_TX_RX_ONLY);
    //UARTSetFifoMode(UART2, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    //150465(115200) 299520(230400) 599040(460800)  FUDGE MULT 1.3
    UARTSetDataRate(UART2, GetPeripheralClock(), 599040);
    UARTEnable(UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
}

void setupFrameRefresh(void){
    // Configure Timer 3
    OpenTimer3(T3_ON | T3_PS_1_256, FRAME_REFRESH_CNT);

    // Set up Timer 3 interrupt with a priority of 6 and zero sub-priority
    INTEnable(INT_T3, INT_ENABLED);
    INTSetVectorPriority(INT_TIMER_3_VECTOR, INT_PRIORITY_LEVEL_5);
    INTSetVectorSubPriority(INT_TIMER_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);

    // Enable multi-vector interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();
}

void startTimeoutTimer(void){
       // Configure Timer 3
    OpenTimer2(T2_ON | T2_PS_1_256, TIMEOUT_CNT);

    // Set up Timer 2 interrupt with a priority of 6 and zero sub-priority
    INTEnable(INT_T2, INT_ENABLED);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_5);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);

    // Enable multi-vector interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();

}

void stopTimeoutTimer(void){
    CloseTimer2();
}

uint8_t ledTweak[256];
void genGammaMap(){
    // Calculates gamma values
    float f;
    for (f = 0; f < 256; f++) {
        ledTweak[(uint8_t) f] = (uint8_t) floor(
                (f * (f / 256) + 1)+0.5
                );
    }

    ledTweak[0] = 0;
}

void alterGamma(uint32_t* color) {
    uint32_t tmp = *color;
    *color = (ledTweak[(tmp >> 24) & 0xff] << 24) |
             (ledTweak[(tmp >> 12) & 0xff] << 12) |
             (ledTweak[(tmp      ) & 0xff]);
}

void sendWS2812(uint32_t* frame ){;
   dmaTxOffIdx   = 1;
   dmaTxFramePtr = frame;
   //Offset
   //0,256,512,768..3840,
   //0, 1,  2,  3 ..15
   DmaChnOpen(chn0,DMA_CHN_PRI0,DMA_OPEN_DEFAULT);
   //DmaChnOpen(chn1,DMA_CHN_PRI1,DMA_OPEN_CHAIN_HI);
   //UART2 rx interrupt to start transfer, stops after 1024KB has be transferered
   DmaChnSetEventControl(chn0, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_SPI2_TX_IRQ));
   //DmaChnSetEventControl(chn1, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_SPI2_TX_IRQ));
   // set the transfer source and dest addresses, source and dest sizes and the cell size
   DmaChnSetTxfer(chn0, (void*)dmaTxFramePtr,  (void*)&SPI2BUF,  256, 4, 4);
   DmaChnSetEvEnableFlags(chn0, DMA_EV_BLOCK_DONE); // enable the transfer done interrupt on final chained dma
   //DmaChnSetEvEnableFlags(chn1, DMA_EV_BLOCK_DONE);

   // Set up DMA Block Complete interrupt with a priority of 7 and zero sub-priority
   INTSetVectorPriority(INT_DMA_0_VECTOR, INT_PRIORITY_LEVEL_7);
   INTSetVectorSubPriority(INT_DMA_0_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
   INTEnable(INT_DMA0, INT_ENABLED);
   //INTSetVectorPriority(INT_DMA_1_VECTOR, INT_PRIORITY_LEVEL_7);
   //INTSetVectorSubPriority(INT_DMA_1_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
   //INTEnable(INT_DMA1, INT_ENABLED);

   // Enable multi-vector interrupts
   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
   INTEnableInterrupts();
   // enable the chn0 to start the DMA Chain
   DmaChnEnable(chn0);
   DmaChnForceTxfer(chn0);

}

// Ping Pong Chn0 WS2812B ISR
void __ISR(_DMA_0_VECTOR, ipl7) DMA0Handler(void) {
    
    INTClearFlag(INT_DMA0);
   if (dmaTxOffIdx < 12) {
        DmaChnOpen(chn0,DMA_CHN_PRI0,DMA_OPEN_DEFAULT);
        //DmaChnOpen(chn0,DMA_CHN_PRI0,DMA_OPEN_DEFAULT|DMA_OPEN_DET_EN);
        DmaChnSetEventControl(chn0, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_SPI2_TX_IRQ));
        DmaChnSetTxfer(chn0, (void*)dmaTxFramePtr+MUL256(dmaTxOffIdx),  (void*)&SPI2BUF,  256, 4, 4);
        DmaChnSetEvEnableFlags(chn0, DMA_EV_BLOCK_DONE); // enable the transfer done interrupt on final chained dma
        INTEnable(INT_DMA0, INT_ENABLED);
        DmaChnEnable(chn0);
        //DmaChnForceTxfer(chn0);
        ++dmaTxOffIdx;
    }
}

// Ping Pong Chn1 WS2812B ISR
void __ISR(_DMA_1_VECTOR, ipl7) DMA1Handler(void) {
    
    //DmaChnSetTxfer(chn1, (void*)dmaTxFramePtr+MUL256(dmaTxOffIdx),  (void*)&SPI2BUF,  256, 4, 4);
    //DmaChnSetTxfer(chn1, (void*)dmaTxFramePtr+MUL256(dmaTxOffIdx),  (void*)&SPI2BUF,  1, 4, 4);
    //INTClearFlag(INT_DMA1);
    //if (dmaTxOffIdx <= 6) {
    //DmaChnSetTxfer(chn0, (void*)dmaTxFramePtr+MUL256(dmaTxOffIdx),  (void*)&SPI2BUF,  4, 4, 4);
    //DmaChnSetTxfer(chn1, (void*)dmaTxFramePtr+MUL256(dmaTxOffIdx+1),  (void*)&SPI2BUF,  4, 4, 4);
    //DmaChnEnable(chn0);
    //DmaChnForceTxfer(chn0);
    //dmaTxOffIdx += 2;
   // }
}

// Frame Refresh Interrupt Handler
void __ISR(_TIMER_3_VECTOR, ipl5) Timer3Handler(void) {
    uint32_t i = 0;
    uint32_t lastPfb = 0;
    uint32_t ApfToFill = (ApfActiveFrame) ? 0 : 1;

    INTClearFlag(INT_T3);                               // Clear the interrupt flag
    sendWS2812(Apf[ApfActiveFrame]);

    //If Pfb isn't empty and Switch not pending then convert and pop.
    if ( (PfbCount > 0) && (ApfSwitchPending == 0) )  {
        apfPack(Pfb[PfbHead], Apf[ApfToFill]);
        lastPfb = PfbHead;
        
        pfbPop();
        if ((PfbFull == 0) && (uartStall != 0)) uartClearStall();
        ///////////////////////////////////////
        //for (i = 0; i < 256; i++)
        //    Pfb[lastPfb][i] = 0;
        ///////////////////////////////////////
        ApfActiveFrame = ApfToFill;
    }



}

// DMA3 Block Complete Interrupt Handler
void __ISR(_DMA_3_VECTOR, ipl7) DMA3Handler(void) {
    if(dmaRxLast) { dmaRxComplete = 1;}
    else {
        uartSetStall(); //Stall UART while setting up next UART
        // Configure the dma channels to chain
        DmaChnOpen(chn2,DMA_CHN_PRI0,DMA_OPEN_DEFAULT);
        DmaChnOpen(chn3,DMA_CHN_PRI0,DMA_OPEN_CHAIN_HI);
        //UART2 rx interrupt to start transfer, stops after 1024KB has be transferered
        DmaChnSetEventControl(chn2, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_UART2_RX_IRQ));
        DmaChnSetEventControl(chn3, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_UART2_RX_IRQ));
        // set the transfer source and dest addresses, source and dest sizes and the cell size
        DmaChnSetTxfer(chn2, (void*)&U2RXREG, (void*)dmaRxPfb+512,     1, 256, 1);
        DmaChnSetTxfer(chn3, (void*)&U2RXREG, (void*)dmaRxPfb+768,     1, 256, 1);

        DmaChnSetEvEnableFlags(chn3, DMA_EV_BLOCK_DONE); // enable the transfer done interrupt on final chained dma

        // Set up DMA Block Complete interrupt with a priority of 7 and zero sub-priority
        INTSetVectorPriority(INT_DMA_3_VECTOR, INT_PRIORITY_LEVEL_7);
        INTSetVectorSubPriority(INT_DMA_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
        INTEnable(INT_DMA3, INT_ENABLED);

        // Enable multi-vector interrupts
        INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
        INTEnableInterrupts();
        // enable the chn0 to start the DMA Chain
        DmaChnEnable(chn2);
        dmaRxLast = 1;
        uartClearStall();
    }
    INTClearFlag(INT_DMA3);                               // Clear the interrupt flag
}

// Frame Refresh Interrupt Handler
void __ISR(_TIMER_2_VECTOR, ipl5) Timer2Handler(void) {

    stopTimeoutTimer();
    
    //Reset Statemachine
    RxStateReset = 1;

    //Flush Uart Buffer
    uartFlushFifo();

    //Cancel UART DMA Channels
    DmaChnDisable(chn2);
    DmaChnDisable(chn3);
    DmaChnAbortTxfer(chn2);
    DmaChnAbortTxfer(chn3);

    INTClearFlag(INT_T2);                               // Clear the interrupt flag
}


int main(void) {

    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES |SYS_CFG_PCACHE);
    mJTAGPortEnable(DEBUG_JTAGPORT_OFF);

    mPORTDSetBits(BIT_1);
    mPORTDClearBits(BIT_1);
    mPORTDSetPinsDigitalOut(BIT_1);

    /* Setup UART Interface to RaspberryPi*/
    setupUART();

    uartDebug("Booting Up...",13);

    pfbFlushBuffers();
    apfFlushBuffers();

    apfPack(splash_union, Apf[0]); // Load Debug PFB into APF

    /* Setup SPI Interface to LED Matrix*/
    setupSPI();

    /*Setup Buffer Refresh*/
    setupFrameRefresh();

    /* Generate Gamma Map*/
    genGammaMap();
    
    uartDebug("Initialization Complete",23);


    RxRsp_t RxRsp;
    uint16_t RxRspValue;
    uint8_t DcBuffer8[1024];
    uint64_t* DcBuffer64;
    uint16_t i,j;

    while(1) {
        i = 0;

        switch (RxState) {
            case CMD_SM:
                stopTimeoutTimer();
                if (UART_DATA_AVAIL) {
                    //while (PfbFull); // Wait till there is space in PFB
                    RxState = DECODE_CMD_SM;
                }
                break;

            case DECODE_CMD_SM:
                startTimeoutTimer(); //40ms to complete before timeout
                if (uartGetByte() == 'C') {
                    while((UART_DATA_AVAIL == 0) && (RxStateReset == 0) );
                    if (RxStateReset) break; //If dmaTimeout Exit State
                    switch((RxCmd_t)uartGetByte()) {
                        case PING_CMD:
                            uartDebug("Rx Ping Command",15);
                            RxRsp = PING_RSP;
                            RxRspValue = ('Z'<<8)|'X';
                            RxState = CMD_SM;
                            break;
                        case START_DRAW_CMD:
                            uartDebug("Rx Start Draw Command",21);
                            RxRsp = ACK_RSP;
                            RxRspValue = 0x0000;
                            RxState = DECODE_DRAW_SM;
                            break;
                        case START_STREAM_CMD:
                            uartDebug("Rx Start Stream Command",23);
                            RxRsp = ACK_RSP;
                            RxRspValue = 0x0000;
                            RxState = DECODE_STREAM_SM;
                            break;
                        case FLUSH_BUFFERS_CMD:
                            uartDebug("Rx Flush Command",16);
                            RxRsp = ACK_RSP;
                            RxRspValue = 0x0000;
                            pfbFlushBuffers();
                            apfFlushBuffers();
                            RxState = CMD_SM;
                            break;
                        case GET_LIGHT_CMD:
                            uartDebug("Rx Light Command",16);
                            RxRsp = LIGHT_RSP;
                            RxRspValue = 0x1234; // TODO
                            RxState = CMD_SM;
                            break;
                        case GET_TEMP_CMD:
                            uartDebug("Rx Temp Command",15);
                            RxRsp = TEMP_RSP;
                            RxRspValue = 0x5678; // TODO
                            RxState = CMD_SM;
                            break;
                        default:
                            uartDebug("Rx Invalid Command",18);
                            RxRsp = ERR_RSP;
                            RxRspValue = ('I'<<8)|'C'; //Invalid Command
                            RxState = CMD_SM;
                            break;
                    }
                    uartSendRsp(RxRsp,RxRspValue);
                    } else {
                        uartSendRsp(ERR_RSP, (('X'<<8)|'X') );
                        uartFlushFifo();
                        RxState = CMD_SM;
                    }
                    break;
                case DECODE_DRAW_SM:
                    i = 0;
                    //dmaDrawSetup( DcBuffer8 );
                    uartDebug("Dma Draw Setup",14);
                    //while(DCH1CON | (1<<15) );//while DMA is running
                    DcBuffer64 = (uint64_t*) DcBuffer8;
                    uartDebug("Dma Draw Complete",17);
                    //while (dcpParse(Pfb[PfbNextFree],&DcBuffer64[i]) == 0) i++;
                    uartDebug("DCP Parse Complete",18);
                    //pfbPush();
                    RxState = CMD_SM;
                    break;
                case DECODE_STREAM_SM:
                    dmaStreamSetup( Pfb[PfbNextFree] );
                    uartDebug("Dma Stream Setup",16);

                    while( (dmaRxComplete == 0) && (RxStateReset == 0)){};
                    if (RxStateReset) break; //If dmaTimeout Exit State

                    uartDebug("Dma Stream Complete",19);

                    pfbPush();
                    if (PfbFull) uartSetStall();

                    //uartSendRsp(ACK_RSP,0x0001);
                    RxState = CMD_SM;
                    uartDebug("End Stream",10);
                    break;
                default:
                    uartDebug("Bad State",9);
                    RxState = CMD_SM; //Reset SM
                    break;
            }
            //end state loop
        if (RxStateReset) { RxState = CMD_SM; RxStateReset = 0; uartDebug("Statemachine Reset!",19);}

    }
    return (EXIT_SUCCESS);
}


