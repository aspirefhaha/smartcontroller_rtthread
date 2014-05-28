#include "led.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include <rtthread.h>

#define TENGHUA_LED_1_PORT		(4)
#define TENGHUA_LED_1_PIN		(22)
#define TENGHUA_LED_1_MASK		(1 << TENGHUA_LED_1_PIN)

#define TENGHUA_LED_2_PORT		(4)
#define TENGHUA_LED_2_PIN		(23)
#define TENGHUA_LED_2_MASK		(1 << TENGHUA_LED_2_PIN)

#define TENGHUA_LED_3_PORT		(4)
#define TENGHUA_LED_3_PIN		(26)
#define TENGHUA_LED_3_MASK		(1 << TENGHUA_LED_3_PIN)

#define TENGHUA_LED_4_PORT		(4)
#define TENGHUA_LED_4_PIN		(27)
#define TENGHUA_LED_4_MASK		(1 << TENGHUA_LED_4_PIN)

ALIGN(RT_ALIGN_SIZE)
static char thread_led_stack[256];
struct rt_thread thread_led;

void rt_hw_led_init(void)
{
	PINSEL_ConfigPin (TENGHUA_LED_1_PORT, TENGHUA_LED_1_PIN, 0);
	PINSEL_ConfigPin (TENGHUA_LED_2_PORT, TENGHUA_LED_2_PIN, 0);
	PINSEL_ConfigPin (TENGHUA_LED_3_PORT, TENGHUA_LED_3_PIN, 0);
	PINSEL_ConfigPin (TENGHUA_LED_4_PORT, TENGHUA_LED_4_PIN, 0);
	GPIO_SetDir(TENGHUA_LED_1_PORT, TENGHUA_LED_1_MASK, GPIO_DIRECTION_OUTPUT);
    GPIO_SetDir(TENGHUA_LED_2_PORT, TENGHUA_LED_2_MASK, GPIO_DIRECTION_OUTPUT);
	GPIO_SetDir(TENGHUA_LED_3_PORT, TENGHUA_LED_3_MASK, GPIO_DIRECTION_OUTPUT);
    GPIO_SetDir(TENGHUA_LED_4_PORT, TENGHUA_LED_4_MASK, GPIO_DIRECTION_OUTPUT);
	rt_thread_init(&thread_led,
			"led",
			rt_thread_entry_led,
			RT_NULL,
			&thread_led_stack[0],
			sizeof(thread_led_stack),11,5);
	rt_thread_startup(&thread_led);
}

void rt_hw_led_on(int led)
{
	switch(led){
	case 0:
		GPIO_OutputValue(TENGHUA_LED_1_PORT,TENGHUA_LED_1_MASK, 1);
		break;
	case 1:
		GPIO_OutputValue(TENGHUA_LED_2_PORT,TENGHUA_LED_2_MASK, 1);
		break;
	case 2:
		GPIO_OutputValue(TENGHUA_LED_3_PORT,TENGHUA_LED_3_MASK, 1);
		break;
	case 3:
		GPIO_OutputValue(TENGHUA_LED_4_PORT,TENGHUA_LED_4_MASK, 1);
		break;
	}
}

void rt_hw_led_off(int led)
{
	switch(led){
	case 0:
		GPIO_OutputValue(TENGHUA_LED_1_PORT,TENGHUA_LED_1_MASK, 0);
		break;
	case 1:
		GPIO_OutputValue(TENGHUA_LED_2_PORT,TENGHUA_LED_2_MASK, 0);
		break;
	case 2:
		GPIO_OutputValue(TENGHUA_LED_3_PORT,TENGHUA_LED_3_MASK, 0);
		break;
	case 3:
		GPIO_OutputValue(TENGHUA_LED_4_PORT,TENGHUA_LED_4_MASK, 0);
		break;
	}
}

void led_off_all(void)
{
	GPIO_OutputValue(TENGHUA_LED_1_PORT,TENGHUA_LED_1_MASK,0);
	GPIO_OutputValue(TENGHUA_LED_2_PORT,TENGHUA_LED_2_MASK,0);
	GPIO_OutputValue(TENGHUA_LED_3_PORT,TENGHUA_LED_3_MASK,0);
	GPIO_OutputValue(TENGHUA_LED_4_PORT,TENGHUA_LED_4_MASK,0);
}

void led_on_all(void)
{
  	GPIO_OutputValue(TENGHUA_LED_1_PORT,TENGHUA_LED_1_MASK,1);
	GPIO_OutputValue(TENGHUA_LED_2_PORT,TENGHUA_LED_2_MASK,1);
	GPIO_OutputValue(TENGHUA_LED_3_PORT,TENGHUA_LED_3_MASK,1);
	GPIO_OutputValue(TENGHUA_LED_4_PORT,TENGHUA_LED_4_MASK,1);
}

void rt_thread_entry_led(void* parameter)
{
    unsigned int count=0;
	led_on_all();
	rt_thread_delay(RT_TICK_PER_SECOND);
	led_off_all();
    while (1)
    {
        /* led on */
#ifndef RT_USING_FINSH
        rt_kprintf("led on,count : %d\r\n",count);
#endif
        count++;
        rt_hw_led_on(count%4);
        /* sleep 0.5 second and switch to other thread */
        rt_thread_delay(RT_TICK_PER_SECOND/2);

        /* led off */
#ifndef RT_USING_FINSH
        rt_kprintf("led off\r\n");
#endif
        rt_hw_led_off(count%4);
        rt_thread_delay(RT_TICK_PER_SECOND/2);
    }
}
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(rt_hw_led_on, turn on led);
FINSH_FUNCTION_EXPORT(rt_hw_led_off, turn off led);
#endif
