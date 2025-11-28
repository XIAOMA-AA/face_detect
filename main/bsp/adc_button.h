#ifndef __ADC_BUTTON__
#define __ADC_BUTTON__
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {

        BSP_BUTTON_VOLUP = 0,
        BSP_BUTTON_VOLDOWN,
        BSP_BUTTON_SET,
        BSP_BUTTON_PLAY,
        BSP_BUTTON_MUTE,
        BSP_BUTTON_REC,
        BSP_BUTTON_MAIN,
        BSP_BUTTON_NUM,
    } bsp_button_t;

void adc_button_init(void);

#ifdef __cplusplus
}
#endif

#endif

