#ifndef _TENGHUA_LED_H

void rt_hw_led_init(void);

void rt_hw_led_on(int led);

void rt_hw_led_off(int led);

void led_off_all(void);

void led_on_all(void);

void rt_thread_entry_led(void* parameter);

#endif
