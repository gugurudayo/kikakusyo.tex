 
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <hidapi/hidapi.h>
#include <stdint.h>
 
typedef uint8_t u8;   
typedef uint16_t u16; 
typedef uint32_t u32; 
typedef uint64_t u64; 
typedef int8_t s8;    
typedef int16_t s16;  
typedef int32_t s32;  
typedef int64_t s64;  
 
typedef enum {
    JOYCON_ERR_NONE             = 0,   
    JOYCON_ERR_INVALID_ARGUMENT = -1,  
    JOYCON_ERR_OPEN_FAILED      = -2,  
    JOYCON_ERR_WRITE_FAILED     = -3,  
    JOYCON_ERR_READ_FAILED      = -4,  
    JOYCON_ERR_READ_TIMEOUT     = -5,  
    JOYCON_ERR_READ_NODATA      = -6,  
    JOYCON_ERR_INVALID_TYPE     = -7,  
    JOYCON_ERR_MCU_1ID31        = -11, 
    JOYCON_ERR_MCU_2MCUON       = -12, 
    JOYCON_ERR_MCU_3MCUONBUSY   = -13, 
    JOYCON_ERR_MCU_4MCUMODESET  = -14, 
    JOYCON_ERR_MCU_5MCUSETBUSY  = -15, 
    JOYCON_ERR_MCU_6NFCPOLL     = -16, 
    JOYCON_ERR_MCU_NOTAG        = -17, 
    JOYCON_ERR_MCU_NOTSUPPORTED = -18, 
    JOYCON_ERR_MCU_READ         = -19, 
    JOYCON_ERR_MCU_IRNOREADY    = -21, 
    JOYCON_ERR_MCU_6IRMODESET   = -26, 
    JOYCON_ERR_MCU_7IRSETBUSY   = -27, 
    JOYCON_ERR_MCU_8IRCFG       = -28, 
    JOYCON_ERR_MCU_9IRFCFG      = -29, 
    JOYCON_ERR_UNKNOWN          = -99  
} joycon_err;
 
typedef enum {
    JOYCON_L = 0x1, 
    JOYCON_R = 0x2  
} joycon_type;
 
#pragma pack(push, 1)
typedef union {
    u8 byte[3];
    struct {
        u8 Y : 1;       
        u8 X : 1;       
        u8 B : 1;       
        u8 A : 1;       
        u8 SR_r : 1;    
        u8 SL_r : 1;    
        u8 R : 1;       
        u8 ZR : 1;      
        u8 Minus : 1;   
        u8 Plus : 1;    
        u8 RStick : 1;  
        u8 LStick : 1;  
        u8 Home : 1;    
        u8 Capture : 1; 
        u8 dummy : 1;   
        u8 grip : 1;    
        u8 Down : 1;    
        u8 Up : 1;      
        u8 Right : 1;   
        u8 Left : 1;    
        u8 SR_l : 1;    
        u8 SL_l : 1;    
        u8 L : 1;       
        u8 ZL : 1;      
    } btn;
} joycon_btn;
 
typedef struct {
    u16 L;    
    u16 R;    
    u16 ZL;   
    u16 ZR;   
    u16 SL;   
    u16 SR;   
    u16 Home; 
} joycon_elapsed;
 
typedef struct {
    u8 mc_duration : 4; 
    u8 mc_num : 4;      
    u8 fc_num : 4;      
    u8 intensity : 4;   
    struct {
        u8 mc2_intensity : 4;  
        u8 mc1_intensity : 4;  
        u8 mc1_duration : 4;   
        u8 mc1_transition : 4; 
        u8 mc2_duration : 4;   
        u8 mc2_transition : 4; 
    } mc[7];                   
    u8 unused : 4;             
    u8 mc15_intensity : 4;     
    u8 mc15_duration : 4;      
    u8 mc15_transition : 4;    
} joycon_homeled;
#pragma pack(pop)
 
#define JOYCON_HOMELED_OFF ((joycon_homeled*)0x11)
#define JOYCON_HOMELED_ON ((joycon_homeled*)0x11f0f00f)
#define JOYCON_HOMELED_BLINK ((joycon_homeled*)0x21f0f00f0f)
 
#define JOYCON_LED_1_ON (0x01)    
#define JOYCON_LED_2_ON (0x02)    
#define JOYCON_LED_3_ON (0x04)    
#define JOYCON_LED_4_ON (0x08)    
#define JOYCON_LED_1_BLINK (0x10) 
#define JOYCON_LED_2_BLINK (0x20) 
#define JOYCON_LED_3_BLINK (0x40) 
#define JOYCON_LED_4_BLINK (0x80) 
 
typedef struct {
    float x; 
    float y; 
} joycon_stick;
 
typedef struct {
    float acc_x;  
    float acc_y;  
    float acc_z;  
    float gyro_x; 
    float gyro_y; 
    float gyro_z; 
} joycon_axis;
 
typedef struct {
    hid_device* handle;  
    joycon_type type;    
    u8 led_bk;           
    u8 packnum;          
    u8 rumble[4];        
    u8 battery;          
    joycon_btn button;   
    joycon_stick stick;  
    joycon_axis axis[3]; 
 
    u16 stick_cal_x[3];      
    u16 stick_cal_y[3];      
    float acc_cal_coeff[3];  
    float gyro_cal_coeff[3]; 
    s16 sensor_cal[3];       
 
    u8 ir_enable;      
    u8 ir_max_frag_no; 
    u16 ir_resolution; 
    u16 ir_exposure;   
} joyconlib_t;
 
#define JOYCON_NFC_TYPE_NTAG (0x02)
#define JOYCON_NFC_TYPE_MIFARE (0x04)
 
typedef struct {
    u8 tag_type;       
    u8 tag_uid_size;   
    u8 tag_uid[10];    
    u8 ntag_data_size; 
    u8 ntag_data[924]; 
} joycon_nfc_data;
 
typedef enum {
    JOYCON_IR320X240 = 320, 
    JOYCON_IR160X120 = 160, 
    JOYCON_IR80X60   = 80,  
    JOYCON_IR40X30   = 40   
} joycon_ir_resolution;
 
typedef struct {
    u8 avg_intensity; 
    u8 unknown;       
    u16 white_pixels; 
    u16 noise_pixels; 
} joycon_ir_result;
 
extern joycon_err joycon_open(joyconlib_t* jc, joycon_type type);
 
extern joycon_err joycon_close(joyconlib_t* jc);
 
extern joycon_err joycon_get_state(joyconlib_t* jc);
 
extern joycon_err joycon_rumble(joyconlib_t* jc, int amp);
 
extern joycon_err joycon_rumble_raw(joyconlib_t* jc, int hfreq, int hamp, int lfreq, int lamp);
 
extern joycon_err joycon_get_button_elapsed(joyconlib_t* jc, joycon_elapsed* etime);
 
extern joycon_err joycon_set_led(joyconlib_t* jc, u8 led);
 
extern joycon_err joycon_set_homeled(joyconlib_t* jc, joycon_homeled* data);
 
extern joycon_err joycon_play_rumble(joyconlib_t* jc, char* mml, size_t sz, int (*callback)(void*), void* data);
 
extern joycon_err joycon_enable_ir(joyconlib_t* jc, joycon_ir_resolution resolution, u16 exposure);
 
extern joycon_err joycon_disable_ir(joyconlib_t* jc);
 
extern joycon_err joycon_read_ir(joyconlib_t* jc, u8* image, size_t size, u16 exposure, joycon_ir_result* result);
 
extern joycon_err joycon_read_nfc(joyconlib_t* jc, joycon_nfc_data* nfc);
 
#ifdef __cplusplus
}
#endif
/*
MIT License
 
Copyright (c) 2022 K. Morita
 
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
