#define GPIO_HIGH   1
#define GPIO_LOW    0

/*
*   --------------------------------------------------------------------  
*   gpio_init
*   --------------------------------------------------------------------
*   Initializes all the GPIO used on the board and sets appropriate
*   interrupts etc
*/
void gpio_init(void);

/*
*   --------------------------------------------------------------------  
*   flash_pin
*   --------------------------------------------------------------------
*   Flashes an LED on the selected pin
*/
void flash_pin(gpio_num_t pin, uint16_t delay);

/*
*   --------------------------------------------------------------------  
*   setPin
*   --------------------------------------------------------------------
*   Sets an output pin to the desired state
*/
void setPin(gpio_num_t pin, bool state);