#include "o3.h"
#include "gpio.h"
#include "systick.h"

/**************************************************************************//**
 * @brief Konverterer nummer til string
 * Konverterer et nummer mellom 0 og 99 til string
 *****************************************************************************/
void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;

	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

/**************************************************************************//**
 * @brief Konverterer 3 tall til en timestamp-string
 * timestamp-argumentet må være et array med plass til (minst) 7 elementer.
 * Det kan deklareres i funksjonen som kaller som "char timestamp[7];"
 * Kallet blir dermed:
 * char timestamp[7];
 * time_to_string(timestamp, h, m, s);
 *****************************************************************************/
void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

//global variables

#define set_seconds 0
#define set_minutes 1
#define set_hours 2
#define count_down 3
#define alarm 4

#define LED_PORT GPIO_PORT_E
#define PB_PORT GPIO_PORT_B
#define LED_PIN 2
#define PB0_PIN 9
#define PB1_PIN 10

struct time_t {
	int seconds;
	int minutes;
	int hours;
};

static int state = set_seconds;
static struct time_t time = {0};
static char str[8] = "0000000\0";


struct gpio_port_t {
	word CTRL;
	word MODEL;
	word MODEH;
	word DOUT;
	word DOUTSET;
	word DOUTCLR;
	word DOUTTGL;
	word DOUTIN;
	word PINLOCKN;
};

volatile struct gpio_t {
	struct gpio_port_t ports[6];
	word __unused[10];
	word EXTIPSELL;
	word EXTIPSELH;
	word EXTIRISE;
	word EXTIFALL;
	word IEN;
	word IF;
	word IFS;
	word IFC;
	word ROUTE;
	word INSENSE;
	word LOCK;
	word CTRL;
	word CMD;
	word EM4WUEN;
	word EM4WUPOL;
	word EM4WUCAUSE;
} *GPIO = (struct gpio_t*)GPIO_BASE;

volatile struct systick_t {
	word CTRL;
	word LOAD;
	word VAL;
	word CALIB;
} *SYSTICK = (struct systick_t*)SYSTICK_BASE;


void set_LED(int i){
	if (i == 1)
		GPIO->ports[LED_PORT].DOUTSET = 1 << LED_PIN;
	else
		GPIO->ports[LED_PORT].DOUTCLR = 1 << LED_PIN;
}

void update_display(){
	time_to_string(str, time.hours, time.minutes, time.seconds);
	lcd_write(str);
}

void setup(){

	//systick setup
	SYSTICK->LOAD = FREQUENCY;
	SYSTICK->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;

	//set LED to output
	GPIO->ports[LED_PORT].MODEL = ((~(0b1111<<LED_PIN*4))&GPIO->ports[LED_PORT].MODEL)|(GPIO_MODE_OUTPUT<<LED_PIN*4);

	//input mode PB0/PB1
	GPIO->ports[PB_PORT].MODEH = ((~(0b1111<<4))&GPIO->ports[PB_PORT].MODEH)|(GPIO_MODE_INPUT<<4);
	GPIO->ports[PB_PORT].MODEH = ((~(0b1111<<8))&GPIO->ports[PB_PORT].MODEH)|(GPIO_MODE_INPUT<<8);

	//set select port B PB0_PIN/PB1_PIN
	GPIO->EXTIPSELH = ((~(0b1111<<4))&GPIO->EXTIPSELH)|(0b0001<<4);
	GPIO->EXTIPSELH = ((~(0b1111<<8))&GPIO->EXTIPSELH)|(0b0001<<8);

	//set falling edge trigger PB0_PIN/PB1_PIN
	GPIO->EXTIFALL |= 1 << PB0_PIN;
	GPIO->EXTIFALL |= 1 << PB1_PIN;

	//set interrupt enable PB0_PIN/PB1_PIN
	GPIO->IEN |= 1 << PB0_PIN;
	GPIO->IEN |= 1 << PB1_PIN;

	//clear IF
	GPIO->IFC = GPIO->IFC|(1<<PB0_PIN);
	GPIO->IFC = GPIO->IFC|(1<<PB1_PIN);
}

void start() {
	SYSTICK->VAL = SYSTICK->LOAD;
	SYSTICK->CTRL |= SysTick_CTRL_ENABLE_Msk;		//start clock
	}

void stop() {
	SYSTICK->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);		//stop clock
	}

void add_hours(){
	time.hours++;
	}

void add_minutes(){
	time.minutes++;
	if (time.minutes >= 60){
		time.minutes = 0;
		add_hours();
		}
	}

void add_seconds(){
	time.seconds++;
	if (time.seconds >= 60){
		time.seconds = 0;
		add_minutes();
		}
	}

void GPIO_EVEN_IRQHandler(){
	//IRQ handler for PB1. Switching between states
	switch (state) {
		case set_seconds: {
			state = set_minutes;
		} break;
		case set_minutes: {
			state = set_hours;
		} break;
		case set_hours: {
			state = count_down;
			start();
		} break;
		case count_down: break;
		case alarm: {
			state = set_seconds;
			set_LED(0);
		} break;
	}
	GPIO->IFC = 1 << PB1_PIN;
}

void GPIO_ODD_IRQHandler(){
	//IRQ handler for PB0. Adds s/m/h
	switch (state) {
		case set_seconds: {
			add_seconds();
			update_display();
		} break;
		case set_minutes: {
			add_minutes();
			update_display();
		} break;
		case set_hours: {
			add_hours();
			update_display();
		} break;
		case count_down: break;
		case alarm: break;
			set_LED(1);
	}

	GPIO->IFC = 1 << PB0_PIN;
}

void SysTick_Handler(){
	if (state == count_down) {
		if (time.seconds <= 0) {
			if (time.minutes <= 0) {
				if (time.hours <= 0) {
					state = alarm;
					stop();
					set_LED(1);
					update_display();
					return;
				}
				time.hours--;
				time.minutes = 61;
			}
			time.minutes--;
			time.seconds = 61;
		}
		time.seconds--;
		update_display();
	}
}

int main(void) {
    init();
    setup();
    update_display();

    for(;;);
    return 0;
}


