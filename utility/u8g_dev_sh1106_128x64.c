/*

   u8g_dev_sh1106_128x64.c

   Universal 8bit Graphics Library

   Copyright (c) 2011, olikraus@gmail.com
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


 */

/* 初衷: 惠特128x64使用的SH1106芯片,这使得SSD1306总有些奇怪的情况.
 * 这里包含修改: 提供一个写入sh1106函数,使用[U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE)]调用
 */

#include "u8g.h"

#define WIDTH 128 //SH1106 -- 132x64, different from sh1106, But in here only use 128(col2-col129)
#define HEIGHT 64
#define PAGE_HEIGHT 8

/* init with HuiTec 1.3' SH1106 I2C OLED Module
 * It use the display module: QG-2864KLBLG01 By Allvision technology
 */
static const uint8_t u8g_dev_sh1106_128x64_huitec_init_seq[] PROGMEM = {
    U8G_ESC_CS(0),           /* disable chip */
    U8G_ESC_ADR(0),         /* instruction mode */
    U8G_ESC_RST(1),         /* do reset low pulse with (1*16)+2 milliseconds */
    U8G_ESC_CS(1),           /* enable chip */

    /* Start Enter Set,First off display */
    0x0ae,                  // display off,when all this have done,we light on the display again:)
    
    /* Set Lower Column Address: (00H - 0FH) */
    
    0x002,            //Set lower column address(from col2 for 128x64,that's specal)
    
    /* Set Higher Column Address: (10H - 1FH) */
     0x010,         //Set hight column address,there just begin with start,this is the [0 0 0 1 X X X X], so the small will be [0001 0000]=10H.
    
    /* Set Display Start Line: (40H - 7FH)
       Specifies line address (refer to Figure. 8) to determine the initial display line or COM0. The RAM display data becomes the
       top line of OLED screen. It is followed by the higher number of lines in ascending order, corresponding to the duty cycle. When this command changes the line address, the smooth scrolling or page change takes place.
     */
    0x040,          //Set display start line just from 40H:)

    /* Set Page Address: (B0H - B7H)
       Specifies page address to load display RAM data to page address register. Any RAM data bit can be accessed when its page address and column address are specified. The display remains unchanged even when the page address is changed.
     */
    0x0b0,          //We Start just from B0H:)

    /* Set Contrast Control Register: (Double Bytes Command)
       This command is to set contrast setting of the display. The chip has 256 contrast steps from 00 to FF. The segment output current increases as the contrast step value increases.
       Segment output current setting: ISEG = a/256 X IREF X scale factor
       Where: a is contrast step; IREF is reference current equals 12.5μA; Scale factor = 16.*/
    0x081, //Enter The Contrast Control Mode Set: (81H)
    0x080, //Contrast Data Register Set: (00H - FFH),default value is 80H(means nothing change)

    /*  Set Segment Re-map: (A0H - A1H)
       Change the relationship between RAM column address and segment driver. The order of segment driver output pads can be
       reversed by software. This allows flexible IC layout during OLED module assembly. For details, refer to the column address section of Figure. 8. When display data is written or read, the column address is incremented by 1 as shown in Figure.
     */
    0x0a1, //Value can be A0H(normal direction-the right rotates) or A1H(reverse direction-the left rotates),The Chip Default is A1H,But The HuiTec Module use A1H as deafult,And if you A0H for HuiTec Module,EveryThing will filp:)

    /* Set Normal/Reverse Display: (A6H -A7H)
       Reverses the display ON/OFF status without rewriting the contents of the display data RAM.
     */
    0x0a6, //Value can be A6H(normal display, the RAM data is high, being OLED ON potential) or A7H(reverse display, the RAM data is low, being OLED ON potential), Chip Default is A6H

    /* Set Multiplex Ration: (Double Bytes Command)
       This command switches default 64 multiplex modes to any multiplex ratio from 1 to 64. The output pads COM0-COM63 will be switched to corresponding common signal.
     */
    0x0a8,   //Enter Multiplex Ration Mode,not sure what's this for:)
    0x03f,  //Set the Multiplex Ration Data, 0x00F(1) - 0x03F(64), default value is 0x03f(64)

    /* Set DC-DC OFF/ON: (Double Bytes Command)
       This command is to control the DC-DC voltage converter. The converter will be turned on by issuing this command then display ON command. The panel display must be off while issuing this command.
     */
    0x0ad, //Enter DC-DC Control Mode Set,The other name is called [set charge pump enable mode:)]
    0x08b, //DC-DC ON/OFF Mode Set: 8AH(off),8BH(on), default value is 8BH(on),maens dont use extern VCC.

    /* Set Pump voltage value
       Specifies output voltage (VPP) of the internal charger pump.
     */
    0x031, //Specifies output voltage (VPP) of the internal charger pump(30H-33H).The value include: {30H: 6.4V,31H: 7.4V,32H: 8.0V,33H: 9.0V},default in chip is 32H,for the HuiTec Module them use 30H.as i test seems them all be okay.

    /* Set Common Output Scan Direction
       This command sets the scan direction of the common output allowing layout flexibility in OLED module design. In addition, the display will have immediate effect once this command is issued. That is, if this command is sent during normal display, the graphic display will be vertically flipped.
     */
    0x0c8, //Value Can be C0H or C8H,C0H Means Scan from COM0 to COM [N -1].C8H Means Scan from COM [N -1] to COM0.CHIP default is C0H,but HuiTec Module use C8H

    /* Set Display Offset: (Double Bytes Command)
       This is a double byte command. The next command specifies the mapping of display start line to one of COM0-63 (it is assumed that COM0 is the display start line, that equals to 0). For example, to move the COM16 towards the COM0 direction for 16 lines, the 6-bit data in the second byte should be given by 010000. To move in the opposite direction by 16 lines, the 6-bit data should be given by (64-16), so the second byte should be 100000.
     */
    0x0d3, //Enter Display Offset Mode Set: (D3H)
    0x000, //Display Offset Data Set: (00H~3FH),Default is 00H

    /* Set Display Clock Divide Ratio/Oscillator Frequency: (Double Bytes Command)
       This command is used to set the frequency of the internal display clocks (DCLKs). It is defined as the divide ratio (Value
       from 1 to 16) used to divide the oscillator frequency. POR is 1. Frame frequency is determined by divide ratio, number of display clocks per row, MUX ratio and oscillator frequency.
     */
    0x0d5, //Enter Divide Ratio/Oscillator Frequency Mode Set: (D5H)
    0x080, //Divide Ratio/Oscillator Frequency Data Set: (00H - FFH), The Chip Default is 50H, The HuiTec Module default is 80H,Means Divide Ration is 1,oscillator frequency is +15%(1000B).

    /*  Set Dis-charge/Pre-charge Period: (Double Bytes Command)
        This command is used to set the duration of the pre-charge period. The interval is counted in number of DCLK. POR is 2 DCLKs.
     */
    0x0d9, //Enter Pre-charge Period Mode Set: (D9H)
    0x01f, //Dis-charge/Pre-charge Period Data Set: (00H - FFH),Default is 22H(2 DCLKs,2 DCLKs),USE 1FH for long lifetime (maybe)

    /* Set Common pads hardware configuration: (Double Bytes Command) */
    0x0da, //Enter Common Pads Hardware Configuration Mode Set: (DAH)
    0x012, //Sequential/Alternative Mode Set: (02H,12H),Default is 12H(COM62, 60 - 2, 0,SEG0, 1 - 130, 131,COM1, 3 - 61, 63)

    /* Set VCOM Deselect Level: (Double Bytes Command)
       This command is to set the common pad output voltage level at deselect stage.
     */
    0x0db, //Enter VCOM Deselect Level Mode Set: (DBH)
    0x040, //VCOM Deselect Level Data Set: (00H - FFH),HuiTec Module use 40H(B=0.1), VCOM = B x VREF = (0.1 + 64 X 0.006415) X VREF

    /* Display ON */
    0x0af, //Display ON OLED(AFH)

    U8G_ESC_CS(0),           /* disable chip */
    U8G_ESC_END              /* end of sequence */
};


/* select one init sequence here */
#define u8g_dev_sh1106_128x64_init_seq u8g_dev_sh1106_128x64_huitec_init_seq

static const uint8_t u8g_dev_sh1106_128x64_data_start[] PROGMEM = {
    U8G_ESC_ADR(0),         /* instruction mode */
    U8G_ESC_CS(1),           /* enable chip */
    0x010,              /* set upper 4 bit of the col adr to 0 */
    
    /* In the 128x64 display,We start with col2,and end with 122 */
    0x002,              /* set lower 4 bit of the col adr to 2  */
    U8G_ESC_END              /* end of sequence */
};

static const uint8_t u8g_dev_sh1106_sleep_on[] PROGMEM = {
    U8G_ESC_ADR(0),         /* instruction mode */
    U8G_ESC_CS(1),           /* enable chip */
    0x0ae,              /* display off */
    U8G_ESC_CS(1),           /* disable chip */
    U8G_ESC_END              /* end of sequence */
};

static const uint8_t u8g_dev_sh1106_sleep_off[] PROGMEM = {
    U8G_ESC_ADR(0),         /* instruction mode */
    U8G_ESC_CS(1),           /* enable chip */
    0x0af,              /* display on */
    
    /* Power Stabilized (100ms Delay Recommended) */
    U8G_ESC_DLY(100),     /* delay 100 ms */
    U8G_ESC_CS(1),           /* disable chip */
    U8G_ESC_END              /* end of sequence */
};

//@slboat 入口看起来是...初始化的入口功能
uint8_t u8g_dev_sh1106_128x64_fn(u8g_t *u8g, u8g_dev_t *dev, uint8_t msg, void *arg){
    //@slboat msg是个枚举的小玩意
    switch (msg) {
    case U8G_DEV_MSG_INIT:
        u8g_InitCom(u8g, dev, U8G_SPI_CLK_CYCLE_300NS);
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_128x64_init_seq);
        break;
    case U8G_DEV_MSG_STOP:
        break;
    case U8G_DEV_MSG_PAGE_NEXT:
    {
        u8g_pb_t *pb = (u8g_pb_t *)(dev->dev_mem);
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_128x64_data_start);
        u8g_WriteByte(u8g, dev, 0x0b0 | pb->p.page); /* select current page (sh1106) */
        u8g_SetAddress(u8g, dev, 1);           /* data mode */
        if (u8g_pb_WriteBuffer(pb, u8g, dev) == 0)
            return 0;
        u8g_SetChipSelect(u8g, dev, 0);
    }
    break;
    case U8G_DEV_MSG_SLEEP_ON:
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_sleep_on);
        return 1;
    case U8G_DEV_MSG_SLEEP_OFF:
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_sleep_off);
        return 1;
    }
    return u8g_dev_pb8v1_base_fn(u8g, dev, msg, arg);
}

uint8_t u8g_dev_sh1106_128x64_2x_fn(u8g_t *u8g, u8g_dev_t *dev, uint8_t msg, void *arg){
    switch (msg) {
    case U8G_DEV_MSG_INIT:
        //why use U8G_SPI_CLK_CYCLE_300NS?
        u8g_InitCom(u8g, dev, U8G_SPI_CLK_CYCLE_300NS);
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_128x64_init_seq);
        break;
    case U8G_DEV_MSG_STOP:
        break;
    case U8G_DEV_MSG_PAGE_NEXT:
    {
        u8g_pb_t *pb = (u8g_pb_t *)(dev->dev_mem);

        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_128x64_data_start);
        u8g_WriteByte(u8g, dev, 0x0b0 | (pb->p.page * 2)); /* select current page (sh1106) */
        u8g_SetAddress(u8g, dev, 1);           /* data mode */
        u8g_WriteSequence(u8g, dev, pb->width, pb->buf);
        u8g_SetChipSelect(u8g, dev, 0);

        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_128x64_data_start);
        u8g_WriteByte(u8g, dev, 0x0b0 | (pb->p.page * 2 + 1)); /* select current page (sh1106) */
        u8g_SetAddress(u8g, dev, 1);           /* data mode */
        u8g_WriteSequence(u8g, dev, pb->width, (uint8_t *)(pb->buf) + pb->width);
        u8g_SetChipSelect(u8g, dev, 0);
    }
    break;
    case U8G_DEV_MSG_SLEEP_ON:
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_sleep_on);
        return 1;
    case U8G_DEV_MSG_SLEEP_OFF:
        u8g_WriteEscSeqP(u8g, dev, u8g_dev_sh1106_sleep_off);
        return 1;
    }
    return u8g_dev_pb16v1_base_fn(u8g, dev, msg, arg);
}

U8G_PB_DEV(u8g_dev_sh1106_128x64_sw_spi, WIDTH, HEIGHT, PAGE_HEIGHT, u8g_dev_sh1106_128x64_fn, U8G_COM_SW_SPI);
U8G_PB_DEV(u8g_dev_sh1106_128x64_hw_spi, WIDTH, HEIGHT, PAGE_HEIGHT, u8g_dev_sh1106_128x64_fn, U8G_COM_HW_SPI);
U8G_PB_DEV(u8g_dev_sh1106_128x64_i2c, WIDTH, HEIGHT, PAGE_HEIGHT, u8g_dev_sh1106_128x64_fn, U8G_COM_SSD_I2C);

uint8_t   u8g_dev_sh1106_128x64_2x_buf[WIDTH * 2] U8G_NOCOMMON;
u8g_pb_t  u8g_dev_sh1106_128x64_2x_pb = { {16, HEIGHT, 0, 0, 0}, WIDTH, u8g_dev_sh1106_128x64_2x_buf};
u8g_dev_t u8g_dev_sh1106_128x64_2x_sw_spi = { u8g_dev_sh1106_128x64_2x_fn, &u8g_dev_sh1106_128x64_2x_pb, U8G_COM_SW_SPI };
u8g_dev_t u8g_dev_sh1106_128x64_2x_hw_spi = { u8g_dev_sh1106_128x64_2x_fn, &u8g_dev_sh1106_128x64_2x_pb, U8G_COM_HW_SPI };
u8g_dev_t u8g_dev_sh1106_128x64_2x_i2c = { u8g_dev_sh1106_128x64_2x_fn, &u8g_dev_sh1106_128x64_2x_pb, U8G_COM_SSD_I2C };

