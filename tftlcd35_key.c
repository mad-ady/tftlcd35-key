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
static unsigned int KEY_RELEASE = 1022;
 
#define HIGH        1
#define LOW         0
#define LONG        2
 
#define UPDATE_PERIOD   200 // 200ms

#define PLATFORM_C1   1
#define PLATFORM_XU4  2
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
static unsigned int KEY1_ADC = 5;
static unsigned int KEY2_ADC = 515;
static unsigned int KEY3_ADC = 680;
static unsigned int KEY4_ADC = 770;

static unsigned int ANALOGREAD_INPUT = 0;

typedef struct  {
    unsigned int	adc_val;       // GPIO Number
    char		code;       // Keycode
    unsigned int	status;     // current status
    unsigned int	count;	    // how many times has the value persisted
}   UserKeyStruct;

static UserKeyStruct UserKey[4];
 
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
    printf("Reading...\n");
    //itterate through all defined keys 
    for(i = 0; i < MAX_KEY_CNT; i++)    {
      unsigned short value = analogRead(ANALOGREAD_INPUT);
      
      if (!compareKEY(value, UserKey[i].status)) {
        //keys differ, need to update the pressed key
	printf("UserKey[%d]: old value: %d, new value: %d, old count %d\n", i, UserKey[i].status, value, UserKey[i].count);
	UserKey[i].status = value;
	UserKey[i].count = 0;

	//see if the key was pressed, released, or held
	unsigned int keystatus = LOW;
	if(! compareKEY(UserKey[i].status, UserKey[i].adc_val)){
		keystatus = LOW;
	}
	else{
		keystatus = HIGH;
	}

	printf("UserKey[%d]: keystatus: %d\n", i, keystatus);

        send_key_event( uInputFd,
                EV_KEY,
                UserKey[i].code,
                keystatus);
        send_key_event( uInputFd, 
                EV_SYN,
                SYN_REPORT,
                0 );
      }
      else{
	UserKey[i].count++;
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

//Detect platform based on /proc/cpuinfo
int detect_platform(){
    FILE *fp;
    int result = 0;
    
    if((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
        return -1;
    }
    printf("Opened /proc/cpuinfo\n");
    char* word = malloc(120); //bytes per word
    
    while(fscanf(fp, "%s", word) != EOF){
        if(strcmp(word, "ODROID-C1") == 0){
            result = PLATFORM_C1;
        }
        if(strcmp(word, "ODROID-C2") == 0){
            result = PLATFORM_C1;
        }
        if(strcmp(word, "ODROID-XU4") == 0){
            result = PLATFORM_XU4;
        }
        if(strcmp(word, "EXYNOS") == 0){
            result = PLATFORM_XU4;
        }
    }
    free(word);
    fclose(fp);
    return result; 
}

//------------------------------------------------------------------------------------------------------------
//
// Start Program
//
//------------------------------------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    printf("Starting\n");
    static int timer = 0 ;
    
    /*UserKey[0] = {.adc_val=KEY1_ADC, .code=KEY_F13, .status=KEY_RELEASE, .count=0};
    UserKey[1] = {   KEY2_ADC, KEY_F14, KEY_RELEASE, 0};
    {   KEY3_ADC, KEY_F15, KEY_RELEASE, 0},
    {   KEY4_ADC, KEY_F16, KEY_RELEASE, 0},*/
    

    // Do platform detection based on /proc/cpuinfo
    int platform = detect_platform();
    //initialize with the correct defaults
    if(platform == PLATFORM_C1){
        printf("Starting on platform C0/C1/C2\n");
        ANALOGREAD_INPUT = 0;
    else if(platform == PLATFORM_XU4){
        printf("Starting on platform XU3/XU4\n");
        ANALOGREAD_INPUT = 1;
    }
    
    wiringPiSetup ();
    
    return 0;
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



