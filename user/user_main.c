#include <esp8266.h>
#include <config.h>
#include <syslog.h>
#include <bmp180.h>
#include <osapi.h>

static volatile os_timer_t measure_timer;

void ICACHE_FLASH_ATTR measure_task(void *arg)
{
	syslog(2, 2, "USER", "Temp: %d.", (int16_t) bmp180_get_temp());
	syslog(2, 2, "USER", "Pressure: %d Pa.",
		   (uint32_t) bmp180_get_pressure(3));
}


// initialize the custom stuff that goes beyond esp-link
void app_init()
{
	if (bmp180_init()) {
		syslog(2, 2, "USER", "BMP180 sensor initialized successfully.");
		// setup timer (5000ms, repeating)
		os_timer_setfn(&measure_timer, (os_timer_func_t *) measure_task,
					   NULL);
		os_timer_arm(&measure_timer, 5000, 1);
	} else {
		syslog(2, 2, "USER", "Cannot initialize BMP180 sensor.");
	}
}
