//------------------------------------------------------------------------------------------------------------
//
// ADS7846 TFT-LCD Board Keypad Application. (Use wiringPi Library)
// Defined port number is wiringPi port number.
//
// Compile : gcc -o <create excute file name> <source file name> -lwiringPi -lwiringPiDev -lpthread
// Run : sudo ./<created excute file name>
//
//------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
 
#include <unistd.h>
#include <string.h>
#include <time.h>
 
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <wiringSerial.h>
 
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
 
//------------------------------------------------------------------------------------------------------------
//
// Global handle Define
//
//------------------------------------------------------------------------------------------------------------
#define KEY_PRESS   1
#define KEY_RELEASE 1022
 
#define HIGH        1
#define LOW         0
 
#define UPDATE_PERIOD   200 // 200ms
 
//------------------------------------------------------------------------------------------------------------
//
// UINPUT Handle:
//
//------------------------------------------------------------------------------------------------------------
int uInputFd = -1;
 
#define UINPUT_DEV_NODE "/dev/uinput"
 
//------------------------------------------------------------------------------------------------------------
//
// Keypad Define:
//
//------------------------------------------------------------------------------------------------------------
#define KEY1_ADC 5
#define KEY2_ADC 515
#define KEY3_ADC 680
#define KEY4_ADC 770

struct  {
    unsigned int	adc_val;       // GPIO Number
    char		code;       // Keycode
    unsigned int	status;     // current status
}   UserKey[] = {
    {   KEY1_ADC, KEY_SPACE, KEY_RELEASE},
    {   KEY2_ADC, KEY_UP, KEY_RELEASE},
    {   KEY3_ADC, KEY_DOWN, KEY_RELEASE},
    {   KEY4_ADC, KEY_ENTER, KEY_RELEASE},
};
 
#define MAX_KEY_CNT     sizeof(UserKey)/sizeof(UserKey[0])

char compareKEY(unsigned short key, unsigned short status)
{
char compare = 0;
if (key - status < 10 && key - status > -10) {
    compare = 1;
}

return compare;
}
 
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//
// Keypad Event Send Function:
//
//------------------------------------------------------------------------------------------------------------
static int send_key_event (int fd, unsigned int type, unsigned int code, unsigned int value)
{
    struct  input_event event;
 
    memset(&event, 0, sizeof(event));
 
    event.type  = type;
    event.code  = code;
    event.value = value;
 
    if(write(fd, &event, sizeof(event)) != sizeof(event))    {
        fprintf(stderr, "%s : event send error!!\n", __func__);
        return  -1;
    }
 
    return 0;
}
 
//------------------------------------------------------------------------------------------------------------
//
// Keypad Update Function:
//
//------------------------------------------------------------------------------------------------------------
static void key_update (void)
{
    int i;
 
    for(i = 0; i < MAX_KEY_CNT; i++)    {
    if (!compareKEY(analogRead(0), UserKey[i].status)) {
        UserKey[i].status = analogRead(0);
        send_key_event( uInputFd,
                EV_KEY,
                UserKey[i].code,
                !compareKEY(UserKey[i].status, UserKey[i].adc_val) ? 0 : 1);
        send_key_event( uInputFd, 
                EV_SYN,
                SYN_REPORT,
                0 );
    }
    }
}
 
//------------------------------------------------------------------------------------------------------------
//
// system init
//
//------------------------------------------------------------------------------------------------------------
int system_init(void)
{
    int i;
    struct uinput_user_dev  device;
 
    memset(&device, 0, sizeof(device));
 
    if((uInputFd = open(UINPUT_DEV_NODE, O_WRONLY)) < 0)    {
        fprintf(stderr, "%s : %s Open error!\n", __func__, UINPUT_DEV_NODE);
        return  -1;
    }
 
    // uinput device init
    strcpy(device.name, "keypad");
    device.id.bustype   = BUS_HOST;
    device.id.vendor    = 0x16B4;
    device.id.product   = 0x0701;
    device.id.version   = 0x0001;
 
    if(write(uInputFd, &device, sizeof(device)) != sizeof(device))   {
        fprintf(stderr, "%s : device init error!\n", __func__);
        goto    err_out;
    }
 
    if(ioctl(uInputFd, UI_SET_EVBIT, EV_KEY) < 0)   {
        fprintf(stderr, "%s : evbit set error!\n", __func__);
        goto    err_out;
    }

for(i = 0; i < MAX_KEY_CNT; i++) {
    if (ioctl(uInputFd, UI_SET_KEYBIT, UserKey[i].code) < 0) {
        fprintf(stderr, "%s : keybit set error!\n", __func__);
        goto	err_out;
    }
}
 
    if(ioctl(uInputFd, UI_DEV_CREATE) < 0)  {
        fprintf(stderr, "%s : dev create error!\n", __func__);
        goto    err_out;
    }
 
    return  0;
 
err_out:
    close(uInputFd);    uInputFd = -1;    
 
    return  -1;
 }
 
//------------------------------------------------------------------------------------------------------------
//
// Start Program
//
//------------------------------------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    static int timer = 0 ;
 
    wiringPiSetup ();
 
    if (system_init() < 0)
    {
        fprintf (stderr, "%s: System Init failed\n", __func__);     return -1;
    }

    for(;;)    {
        usleep(100000);
        if (millis () < timer)  continue ;
 
        timer = millis () + UPDATE_PERIOD;
 
        // All Data update
        key_update();
    }
 
    if(uInputFd)    close(uInputFd);
 
    return 0 ;
}
 
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------



