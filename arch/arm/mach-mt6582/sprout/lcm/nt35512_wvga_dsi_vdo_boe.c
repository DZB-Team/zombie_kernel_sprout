/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef BUILD_LK
#else
#include <linux/string.h>
#endif

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
    #include <string.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                          (480)
#define FRAME_HEIGHT                                         (800)

#define REGFLAG_DELAY                                         0XFE
#define REGFLAG_END_OF_TABLE                                  0x00   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                             (lcm_util.udelay(n))
#define MDELAY(n)                                             (lcm_util.mdelay(n))

//static kal_bool IsFirstBoot = KAL_TRUE;

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


static struct LCM_setting_table lcm_initialization_setting[] = {

    /*
    Note :

    Data ID will depends on the following rule.

        count of parameters > 1    => Data ID = 0x39
        count of parameters = 1    => Data ID = 0x15
        count of parameters = 0    => Data ID = 0x05

    Structure Format :

    {DCS command, count of parameters, {parameter list}}
    {REGFLAG_DELAY, milliseconds of time, {}},

    ...

    Setting ending by predefined flag

    {REGFLAG_END_OF_TABLE, 0x00, {}}
    */
    {0xFF,    4,    {0xAA, 0x55, 0xA5, 0x80}},
    {0xF7,    15,    {0x63, 0x40, 0x00, 0x00, 0x00, 0x01, 0xC4, 0xA2, 0x00, 0x02,0x64,0x54,0x48,0x00,0xD0}},
    {0xFF,    4,    {0xAA, 0x55, 0xA5, 0x00}},


    {0xF0,    5,    {0x55, 0xAA, 0x52, 0x08, 0x01}},
    //{0xc0,    2,    {0x04,0x08}},
    {0xB0,    1,    {0x12}},
    {0xB1,    1,    {0x12}},
    {0xB2,    1,    {0x00}},
    {0xB3,    1,    {0x07}},

    {0xB6,    1,    {0x14}},
    {0xB7,    1,    {0x14}},
    {0xB8,    1,    {0x24}},
    {0xB9,    1,    {0x34}},
    {0xBA,    1,    {0x14}},
    {0xBF,    1,    {0x01}},

    {0xC3,    1,    {0x04}},
    {0xC2,    1,    {0x00}},
    {0xC0,    2,    {0x04,0x08}},

    {0xBC,    3,    {0x00,0x70,0x00}},
    {0xBD,    3,    {0x00,0x70,0x00}},
    {0xBE,    2,    {0x00,0x4B}},


    {0xD1,    52,    {0x00,0x00,0x00,0x0C,0x00,0x22,0x00,0x36,0x00,0x47,0x00,0x66,0x00,0x82,0x00,0xB2,0x00,0xDA,0x01,0x1B,0x01,0x51,0x01,0xA5,0x01,0xED,0x01,0xEE,0x02,0x30,0x02,0x75,0x02,0x9E,0x02,0xD7,0x02,0xF9,0x03,0x28,0x03,0x47,0x03,0x6C,0x03,0x81,0x03,0x90,0x03,0xC0,0x03,0xF8}},
    {0xD2,    52, {0x00,0x48,0x00,0x50,0x00,0x5F,0x00,0x6D,0x00,0x7A,0x00,0x94,0x00,0xAB,0x00,0xD4,0x00,0xF7,0x01,0x31,0x01,0x61,0x01,0xB2,0x01,0xF7,0x01,0xF9,0x02,0x38,0x02,0x7C,0x02,0xA4,0x02,0xDE,0x02,0xFF,0x03,0x30,0x03,0x4D,0x03,0x72,0x03,0x8A,0x03,0xA6,0x03,0xC0,0x03,0xF8}},
    {0xD3,    52, {0x00,0x00,0x00,0x0C,0x00,0x23,0x00,0x37,0x00,0x4A,0x00,0x69,0x00,0x85,0x00,0xB7,0x00,0xE0,0x01,0x21,0x01,0x57,0x01,0xAC,0x01,0xF3,0x01,0xF5,0x02,0x31,0x02,0x78,0x02,0xA1,0x02,0xD7,0x02,0xF8,0x03,0x26,0x03,0x48,0x03,0x6A,0x03,0x71,0x03,0x76,0x03,0xF7,0x03,0xF8}},
    {0xD4,    52, {0x00,0x00,0x00,0x0C,0x00,0x22,0x00,0x36,0x00,0x47,0x00,0x66,0x00,0x82,0x00,0xB2,0x00,0xDA,0x01,0x1B,0x01,0x51,0x01,0xA5,0x01,0xED,0x01,0xEE,0x02,0x30,0x02,0x75,0x02,0x9E,0x02,0xD7,0x02,0xF9,0x03,0x28,0x03,0x47,0x03,0x6C,0x03,0x81,0x03,0x90,0x03,0xC0,0x03,0xF8}},
    {0xD5,    52, {0x00,0x48,0x00,0x50,0x00,0x5F,0x00,0x6D,0x00,0x7A,0x00,0x94,0x00,0xAB,0x00,0xD4,0x00,0xF7,0x01,0x31,0x01,0x61,0x01,0xB2,0x01,0xF7,0x01,0xF9,0x02,0x38,0x02,0x7C,0x02,0xA4,0x02,0xDE,0x02,0xFF,0x03,0x30,0x03,0x4D,0x03,0x72,0x03,0x8A,0x03,0xA6,0x03,0xC0,0x03,0xF8}},
    {0xD6,    52, {0x00,0x00,0x00,0x0C,0x00,0x23,0x00,0x37,0x00,0x4A,0x00,0x69,0x00,0x85,0x00,0xB7,0x00,0xE0,0x01,0x21,0x01,0x57,0x01,0xAC,0x01,0xF3,0x01,0xF5,0x02,0x31,0x02,0x78,0x02,0xA1,0x02,0xD7,0x02,0xF8,0x03,0x26,0x03,0x48,0x03,0x6A,0x03,0x71,0x03,0x76,0x03,0xF7,0x03,0xF8}},


    {0xF0,    5,    {0x55,0xAA,0x52,0x08,0x00}},

    {0xB6,    1,    {0x03}},//SDT:
    {0xB7,    2,    {0x73,0x73}},
    {0xB8,    4,    {0x01,0x06,0x06,0x06}},//Source EQ:
    {0xBC,    1,    {0x00}},
    {0xB1,    3,    {0x63, 0x00, 0x01}},
    {0xB4,    1,    {0x10}},


    // Note
    // Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.

    // Setting ending by predefined flag
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};



static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 100, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 0, {0x00}},

    // Sleep Mode On
    {0x10, 0, {0x00}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};





static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                //MDELAY(10);//soso add or it will fail to send register
           }
    }

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
        memset(params, 0, sizeof(LCM_PARAMS));

        params->type   = LCM_TYPE_DSI;

        params->width  = FRAME_WIDTH;
        params->height = FRAME_HEIGHT;


        params->dsi.mode   = SYNC_EVENT_VDO_MODE;

        // DSI
        /* Command mode setting */
        params->dsi.LANE_NUM                = LCM_TWO_LANE;
        //The following defined the fomat for data coming from LCD engine.
        params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
        params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
        params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        // Highly depends on LCD driver capability.
        // Not support in MT6573
        params->dsi.packet_size=256;

        // Video mode setting
        params->dsi.intermediat_buffer_num = 2;

        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

        params->dsi.vertical_sync_active                = 2;
        params->dsi.vertical_backporch                    = 50;//50
        params->dsi.vertical_frontporch                    = 20;//20
        params->dsi.vertical_active_line                = FRAME_HEIGHT;

        params->dsi.horizontal_sync_active                = 2;
        params->dsi.horizontal_backporch                = 100;
        params->dsi.horizontal_frontporch                = 100;
        params->dsi.horizontal_active_pixel                = FRAME_WIDTH;


        // Bit rate calculation
        params->dsi.pll_div1=30;//32        // fref=26MHz, fvco=fref*(div1+1)    (div1=0~63, fvco=500MHZ~1GHz)
        params->dsi.pll_div2=1;         // div2=0~15: fout=fvo/(2*div2)
}

static void lcm_init(void)
{
    unsigned int data_array[64];

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(120);//Must > 120ms

    //IsFirstBoot = KAL_TRUE;

#if 1
    // 1
    data_array[0] = 0x00053902;
    data_array[1] = 0xA555AAFF;
    data_array[2] = 0x00000080;
    dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x00103902;
    data_array[1] = 0x004063F7;
    data_array[2] = 0xC4010000;
    data_array[3] = 0x640200A2;
    data_array[4] = 0xD0004854;//0x54,0x48,0x00,0xD0
    dsi_set_cmdq(&data_array, 5, 1);

    data_array[0] = 0x00053902;
    data_array[1] = 0xA555AAFF;//0xFF,0xAA, 0x55, 0xA5
    data_array[2] = 0x00000000;
    dsi_set_cmdq(&data_array, 3, 1);

    // 2
    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0;//0Xf0 0x55, 0xAA, 0x52
    data_array[2] = 0x00000108;//0x08, 0x01
    dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x12B01500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x12B11500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x00B21500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x07B31500;dsi_set_cmdq(&data_array, 1, 1);

    data_array[0] = 0x14B61500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x14B71500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x24B81500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x34B91500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x14BA1500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x01BF1500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x04C31500;dsi_set_cmdq(&data_array, 1, 1);
    data_array[0] = 0x00C21500;dsi_set_cmdq(&data_array, 1, 1);

    data_array[0] = 0x00033902;
    data_array[1] = 0x000804C0;
    dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x00043902;
    data_array[1] = 0x007000BC;
    dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x00043902;
    data_array[1] = 0x007000BD;
    dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x00033902;
    data_array[1] = 0x004B00BE;
    dsi_set_cmdq(&data_array, 2, 1);

    //Gamma
    data_array[0] = 0x00353902;
    data_array[1] = 0x000000D1;//D1,0x00,0x00,0x00,
    data_array[2] = 0x0022000C;//0x0C,0x00,0x22,0x00
    data_array[3] = 0x00470036;//0x36,0x00,0x47,0x00,
    data_array[4] = 0x00820066;//0x66,0x00,0x82,0x00
    data_array[5] = 0x01DA00B2;//0xB2,0x00,0xDA,0x01
    data_array[6] = 0x0151011B;//0x1B,0x01,0x51,0x01
    data_array[7] = 0x01ED01A5;//0xA5,0x01,0xED,0x01
    data_array[8] = 0x023002EE;//0xEE,0x02,0x30,0x02
    data_array[9] = 0x029E0275;//0x75,0x02,0x9E,0x02
    data_array[10] = 0x03F902D7;//,0xD7,0x02,0xF9,0x03
    data_array[11] = 0x03470328;//0x28,0x03,0x47,0x03
    data_array[12] = 0x0381036C;//0x6C,0x03,0x81,0x03
    data_array[13] = 0x03C00390;//0x90,0x03,0xC0,0x03,
    data_array[14] = 0x000000F8;//0xF8
    dsi_set_cmdq(&data_array, 15, 1);

    data_array[0] = 0x00353902;
    data_array[1] = 0x004800D2;//D2,0x00,0x48,0x00,
    data_array[2] = 0x005F0050;//0x50,0x00,0x5F,0x00
    data_array[3] = 0x007A006D;//0x6D,0x00,0x7A,0x00
    data_array[4] = 0x00AB0094;//0x94,0x00,0xAB,0x00
    data_array[5] = 0x01F700D4;//0xD4,0x00,0xF7,0x01
    data_array[6] = 0x01610131;//0x31,0x01,0x61,0x01
    data_array[7] = 0x01F701B2;//0xB2,0x01,0xF7,0x01
    data_array[8] = 0x023802F9;//0xF9,0x02,0x38,0x02
    data_array[9] = 0x02A4027C;//0x7C,0x02,0xA4,0x02
    data_array[10] = 0x03FF02DE;//,0xDE,0x02,0xFF,0x03
    data_array[11] = 0x034D0330;//0x30,0x03,0x4D,0x03
    data_array[12] = 0x038A0372;//0x72,0x03,0x8A,0x03
    data_array[13] = 0x03C003A6;//0xA6,0x03,0xC0,0x03
    data_array[14] = 0x000000F8;//0xF8
    dsi_set_cmdq(&data_array, 15, 1);

    data_array[0] = 0x00353902;
    data_array[1] = 0x000000D3;//D3,0x00,0x00,0x00,
    data_array[2] = 0x0023000C;//0x0C,0x00,0x23,0x00
    data_array[3] = 0x004A0037;//0x37,0x00,0x4A,0x00,
    data_array[4] = 0x00850069;//0x69,0x00,0x85,0x00
    data_array[5] = 0x01E000B7;//0xB7,0x00,0xE0,0x01
    data_array[6] = 0x015710121;//0x21,0x01,0x57,0x01
    data_array[7] = 0x01F301AC;//0xAC,0x01,0xF3,0x01
    data_array[8] = 0x023102F5;//0xF5,0x02,0x31,0x02
    data_array[9] = 0x02A10278;//0x78,0x02,0xA1,0x02
    data_array[10] = 0x03F802D7;//,0xD7,0x02,0xF8,0x03
    data_array[11] = 0x03480326;//0x26,0x03,0x48,0x03
    data_array[12] = 0x0371036A;//0x6A,0x03,0x71,0x03
    data_array[13] = 0x0F700376;//0x76,0x03,0xF7,0x03,
    data_array[14] = 0x000000F8;//0xF8
    dsi_set_cmdq(&data_array, 15, 1);


    data_array[0] = 0x00353902;
    data_array[1] = 0x000000D4;//D1,0x00,0x00,0x00,
    data_array[2] = 0x0022000C;//0x0C,0x00,0x22,0x00
    data_array[3] = 0x00470036;//0x36,0x00,0x47,0x00,
    data_array[4] = 0x00820066;//0x66,0x00,0x82,0x00
    data_array[5] = 0x01DA00B2;//0xB2,0x00,0xDA,0x01
    data_array[6] = 0x0151011B;//0x1B,0x01,0x51,0x01
    data_array[7] = 0x01ED01A5;//0xA5,0x01,0xED,0x01
    data_array[8] = 0x023002EE;//0xEE,0x02,0x30,0x02
    data_array[9] = 0x029E0275;//0x75,0x02,0x9E,0x02
    data_array[10] = 0x03F902D7;//,0xD7,0x02,0xF9,0x03
    data_array[11] = 0x03470328;//0x28,0x03,0x47,0x03
    data_array[12] = 0x0381036C;//0x6C,0x03,0x81,0x03
    data_array[13] = 0x03C00390;//0x90,0x03,0xC0,0x03,
    data_array[14] = 0x000000F8;//0xF8
    dsi_set_cmdq(&data_array, 15, 1);

    data_array[0] = 0x00353902;
    data_array[1] = 0x004800D5;//D2,0x00,0x48,0x00,
    data_array[2] = 0x005F0050;//0x50,0x00,0x5F,0x00
    data_array[3] = 0x007A006D;//0x6D,0x00,0x7A,0x00
    data_array[4] = 0x00AB0094;//0x94,0x00,0xAB,0x00
    data_array[5] = 0x01F700D4;//0xD4,0x00,0xF7,0x01
    data_array[6] = 0x01610131;//0x31,0x01,0x61,0x01
    data_array[7] = 0x01F701B2;//0xB2,0x01,0xF7,0x01
    data_array[8] = 0x023802F9;//0xF9,0x02,0x38,0x02
    data_array[9] = 0x02A4027C;//0x7C,0x02,0xA4,0x02
    data_array[10] = 0x03FF02DE;//,0xDE,0x02,0xFF,0x03
    data_array[11] = 0x034D0330;//0x30,0x03,0x4D,0x03
    data_array[12] = 0x038A0372;//0x72,0x03,0x8A,0x03
    data_array[13] = 0x03C003A6;//0xA6,0x03,0xC0,0x03
    data_array[14] = 0x000000F8;//0xF8
    dsi_set_cmdq(&data_array, 15, 1);

    data_array[0] = 0x00353902;
    data_array[1] = 0x000000D6;//D3,0x00,0x00,0x00,
    data_array[2] = 0x0023000C;//0x0C,0x00,0x23,0x00
    data_array[3] = 0x004A0037;//0x37,0x00,0x4A,0x00,
    data_array[4] = 0x00850069;//0x69,0x00,0x85,0x00
    data_array[5] = 0x01E000B7;//0xB7,0x00,0xE0,0x01
    data_array[6] = 0x015710121;//0x21,0x01,0x57,0x01
    data_array[7] = 0x01F301AC;//0xAC,0x01,0xF3,0x01
    data_array[8] = 0x023102F5;//0xF5,0x02,0x31,0x02
    data_array[9] = 0x02A10278;//0x78,0x02,0xA1,0x02
    data_array[10] = 0x03F802D7;//,0xD7,0x02,0xF8,0x03
    data_array[11] = 0x03480326;//0x26,0x03,0x48,0x03
    data_array[12] = 0x0371036A;//0x6A,0x03,0x71,0x03
    data_array[13] = 0x0F700376;//0x76,0x03,0xF7,0x03,
    data_array[14] = 0x000000F8;//0xF8
    dsi_set_cmdq(&data_array, 15, 1);
    //Gamma

    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0;
    data_array[2] = 0x00000008;
    dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x03B61500;
    dsi_set_cmdq(&data_array, 1, 1);

    data_array[0] = 0x00033902;
    data_array[1] = 0x007373B7;
    dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x00053902;
    data_array[1] = 0x060601B8;
    data_array[2] = 0x00000006;
    dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x00BC1500;
    dsi_set_cmdq(&data_array, 1, 1);

    data_array[0] = 0x00043902;
    data_array[1] = 0x010063B1;
    dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x10B41500;
    dsi_set_cmdq(&data_array, 1, 1);

#else
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
#endif
}


static void lcm_suspend(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms

    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
    //lcm_compare_id();

    lcm_init();

    push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
static unsigned int lcm_compare_id(void)
{
    unsigned int id = 0;
    unsigned char buffer[3];
    unsigned int array[16];

    SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    array[0] = 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0x04, buffer, 3);
    id = buffer[1]; //we only need ID
#if defined(BUILD_UBOOT)
    /*The Default Value should be 0x00,0x80,0x00*/
    printf("\n\n\n\n[soso]%s, id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x\n", __func__, buffer[0],buffer[1],buffer[2]);
#endif
    return (id == 0x80)?1:0;
}


LCM_DRIVER nt35512_wvga_dsi_vdo_boe_drv =
{
    .name            = "nt35512_dsi_vdo_lcm_drv",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

