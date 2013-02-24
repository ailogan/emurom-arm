/*
  emurom.c

  Andrew Logan
  2/17/13

  Re-implementation of the AVR-based Atari cartridge emulator I've
  been working on for a 80MHz ARM processor from TI.

  This targets the LM4F120H5QR chip, which is a Blizzard-class Arm Cortex-M4 processor from TI.

  Pin assignments are:

  | Atari | port | pin   | Use? | notes                                                                                 |
  | D0    | PB0  | J1.3  |      | ---NOT 5V TOLERANT---                                                                 |
  | D1    | PB1  | J1.4  |      | ---NOT 5V TOLERANT---                                                                 |
  | D2    | PB2  | J2.2  |      |                                                                                       |
  | D3    | PB3  | J4.3  |      |                                                                                       |
  | D4    | PB4  | J1.7  |      |                                                                                       |
  | D5    | PB5  | J1.2  |      |                                                                                       |
  | D6    | PB6  | J2.7  |      | Tied to PD0.  What the hell TI.                                                       |
  | D7    | PB7  | J2.6  |      | Tied to PD1.                                                                          |

  | A0    | PA2  | J2.10 |      |                                                                                       |
  | A1    | PA3  | J2.9  |      |                                                                                       |
  | A2    | PA4  | J2.8  |      |                                                                                       |
  | A3    | PA5  | J1.8  |      |                                                                                       |
  | A4    | PA6  | J1.9  |      |                                                                                       |
  | A5    | PA7  | J1.10 |      |                                                                                       |

  | A6    | PE0  | J2.3  |      |                                                                                       |
  | A7    | PE1  | J3.7  |      |                                                                                       |
  | A8    | PE2  | J3.8  |      |                                                                                       |
  | A9    | PE3  | J3.9  |      |                                                                                       |
  | A10   | PE4  | J1.5  |      |                                                                                       |
  | A11   | PE5  | J1.6  |      |                                                                                       |

  | A12   | PC4  | J4.4  |      |                                                                                       |

  From http://www.atariage.com/2600/faq/index.html#pinouts:

  (Looking at the bottom of the cartridge -- i.e. edge connectors first)
                          Top
   D3   D4   D5   D6   D7   A12  A10  A11  A9   A8  +5V   SGND
  --1- --2- --3- --4- --5- --6- --7- --8- --9- -10- -11- -12-
   GND  D2   D1   D0   A0   A1   A2   A3   A4   A5   A6   A7
                          Bottom
*/

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"

//#include "romfile.h"
//#include "romfile-noop.h"
#include "romfile-si.h"

//PORTB
#define D0  GPIO_PIN_0
#define D1  GPIO_PIN_1
#define D2  GPIO_PIN_2
#define D3  GPIO_PIN_3
#define D4  GPIO_PIN_4
#define D5  GPIO_PIN_5
#define D6  GPIO_PIN_6
#define D7  GPIO_PIN_7

//PORTA
#define A0  GPIO_PIN_2
#define A1  GPIO_PIN_3
#define A2  GPIO_PIN_4
#define A3  GPIO_PIN_5
#define A4  GPIO_PIN_6
#define A5  GPIO_PIN_7

//PORTE
#define A6  GPIO_PIN_0
#define A7  GPIO_PIN_1
#define A8  GPIO_PIN_2
#define A9  GPIO_PIN_3
#define A10 GPIO_PIN_4
#define A11 GPIO_PIN_5

//PORTC
#define A12 GPIO_PIN_4

//PORTF
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

#define USR_SW1   GPIO_PIN_4
#define USR_SW2   GPIO_PIN_0

void leds_off(){
  GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 0);
  GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, 0);
  GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 0);
}

void blink_bit(int bit){
  if(bit == 0){
    //Green LED (bit off)
    GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, GREEN_LED);
  }
  else{
    //Blue LED (bit on)
    GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, BLUE_LED);
  }

  //Wait for a quarter-second
  SysCtlDelay(80000000 / 3 / 4);

  leds_off();
}   

void blink_long(long addr){
  //Turn on the RED LED
  GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, RED_LED);
  
  //Wait for a half-second
  SysCtlDelay(80000000 / 3 / 2);

  //Turn off all of the LEDs
  leds_off();

  for(int i = 11; i >= 0; i--){
    int bit = ((addr >> i) & 0x01);
    blink_bit(bit);

    //Wait for a quarter-second
    SysCtlDelay(80000000 / 3 / 4);
  }

  //RED LED
  GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, RED_LED);
  
  //Wait for a half-second
  SysCtlDelay(80000000 / 3 / 2);

  //Turn off all LEDs
  leds_off();
}

long get_addr(){
  //Read the address on the input lines
  long addr_low  = GPIOPinRead(GPIO_PORTA_BASE, A0|A1|A2|A3|A4 |A5 ); //PORT2-PORT7
  long addr_high = GPIOPinRead(GPIO_PORTE_BASE, A6|A7|A8|A9|A10|A11); //PORT0-PORT5
  
  //Put it together into an address
  long addr = (addr_high << 6) | (addr_low >> 2);
  
  return addr;
}

void trace_mode(){
  long a12_state;
  long test_address = 0xFFFC;
  
  //Spin while A12 is low
  do{
    a12_state = GPIOPinRead(GPIO_PORTC_BASE, A12);
  }while((a12_state & A12) == 0);

  //Get the first address
  long addr = get_addr();

  //Turn on the blue LED to show we're in trace mode.
  GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, BLUE_LED);

  //And now we go address hunting.
  if((addr & 0x7F) > (test_address & 0x7F)){
    //Address is greater
    GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, GREEN_LED | BLUE_LED);
  }
  else if((addr & 0x7F) == (test_address & 0x7F)){
    //Keep the blue LED on
    GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, BLUE_LED);
  }
  else{
    //Or it's lower
    GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, RED_LED | BLUE_LED);
  }

  //Wait for a half-second
  SysCtlDelay(80000000 / 3 / 2);
  
  for(;;){
    blink_long(addr);

    //Wait for a half-second
    SysCtlDelay(80000000 / 3 / 2);
  }
} 

void debug_mode(){
  //Turn on the red LED to show we're in debug mode.
  GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, RED_LED);

  long prev_addr = -1;
  
  for(;;){
    //Grab the state of A12
    long a12_state = GPIOPinRead(GPIO_PORTC_BASE, A12);

    //Grab the address.
    long addr = get_addr();

    //Mask the address
    unsigned char low_addr = addr & 0xFF;

    //Throw the address out the data lines
    GPIOPinWrite(GPIO_PORTB_BASE, D0|D1|D2|D3|D4|D5|D6|D7, low_addr);

    //is A12 active?
    if((a12_state & A12) == A12){
      //Has the address changed while A12 was active?
      if((prev_addr != -1) && (addr != prev_addr)){
	//then turn on the green LED
	GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, RED_LED | GREEN_LED);
      }
      prev_addr = addr;
    }
    //A12 is low
    else{
      //Reset the previous address
      prev_addr = -1;
    }
  }
}  

int main(){
  //Set the system clock to run at 80MHz  (400MHz PLL / 2 / 2.5 = 80MHz)
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

  //Set up the GPIO ports

  //Data out on PORTB
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, D0|D1|D2|D3|D4|D5|D6|D7);

  //Address in on PORTA, PORTE and PORTC

  //A0-A5 are PORTA2-PORTA7
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, A0|A1|A2|A3|A4|A5);

  //A6-A11 are PORTE0-PORTE5
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, A6|A7|A8|A9|A10|A11);

  //A12 is PORTC4
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, A12);

  //spin while A12 is low
  
  long a12_state = 0;
  long count = 0;

  do{
    a12_state = GPIOPinRead(GPIO_PORTC_BASE, A12);
    count++;
  }while((a12_state & A12) == 0);
  
  //long addr = get_addr();

  //Set up the LEDs on PORTF
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

  for(;;){
    //blink_long(addr);

    //Wait for a half-second
    //SysCtlDelay(80000000 / 3 / 2);
    
    blink_long(count);

    //Wait for a half-second
    SysCtlDelay(80000000 / 3 / 2);
  }

  //Don't bother setting up the switches, just go into trace mode.
  trace_mode();

  //
  // Unlock PF0 so we can change it to a GPIO input
  // Once we have enabled (unlocked) the commit register then re-lock it
  // to prevent further changes.  PF0 is muxed with NMI thus a special case.
  //
  // The fact that PF0 (and PD7) default to NMI is potentially a bug in Rev A. of this microcontroller:
  // http://e2e.ti.com/support/microcontrollers/stellaris_arm/f/471/t/176893.aspx
  //
  HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
  HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
  HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
  
  //
  // Set each of the button GPIO pins as an input with a pull-up.
  //
  GPIODirModeSet(GPIO_PORTF_BASE, USR_SW1 | USR_SW2, GPIO_DIR_MODE_IN);
  
  // 2mA pull-up resistor on a push-pull pin.
  GPIOPadConfigSet(GPIO_PORTF_BASE, USR_SW1 | USR_SW2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  
  //Get the status of the switches as we start up
  long switch_states = GPIOPinRead(GPIO_PORTF_BASE, USR_SW1|USR_SW2);
  
  //According to the schematic the switches are active-low, so go into debug mode if the switch is pushed.
  if((switch_states & USR_SW1) == 0){ //Mask out the bit we care about.
    debug_mode(); 
  } 

  //According to the schematic the switches are active-low, so go into trace mode if switch 2 is pushed.
  if((switch_states & USR_SW2) == 0){ //Mask out the bit we care about.
    trace_mode();
  } 

  //Show that we're in emulator mode.
  GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, (switch_states | GREEN_LED));

  //Otherwise we run forever
  for(;;){
    long a12_state = GPIOPinRead(GPIO_PORTC_BASE, A12);
    long addr = get_addr();

    /*Only return a result if a12 is high*/
    if((a12_state & A12) == A12){
      //Get the value of the byte at that location in the program ROM
      unsigned char data = get_byte(addr);

      //Stuff the result down the data lines
      GPIOPinWrite(GPIO_PORTB_BASE, D0|D1|D2|D3|D4|D5|D6|D7, data);
    }


    /* /\*This is the algorithm proposed by the Harmony Cart developer: */
    /*   http://www.atariage.com/forums/blog/107/entry-6412-perfect-harmony/ *\/ */
    
    /* long new_addr = 0; */
    /* do{ */
    /*   new_addr = get_addr(); */
    /* }while(addr == new_addr); */

    /*
    do{
      //Wait until A12 goes low
      a12_state = GPIOPinRead(GPIO_PORTC_BASE, A12);
    }while((a12_state & A12) == A12);
    */
  }
}
