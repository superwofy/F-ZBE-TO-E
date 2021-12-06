// This sketch creates the two missing messages required to use an f series ZBE (KCAN1) in an E6,7,8,9X car.
// Using a slightly streamlined version of Cory's library https://github.com/coryjfowler/MCP_CAN_lib
// Credit to Trevor for providing insight into 0x273 and 0x274 http://www.loopybunny.co.uk/CarPC/k_can.html
// Hardware used: CANBED V1 http://docs.longan-labs.cc/1030008/ (32U4+MCP2515+MCP2551, LEDs removed)
// Installed between ZBE and car. Powered on 30G.

// *Should* work with:
// 6582 9267955 - 4-pin MEDIA button
// 6131 9253944 - 4-pin CD button
// 6582 9206444 - 10-pin CD button
// 6582 9212449 - 10-pin CD button

// Tested with 4-pin controller P/N 9267955 Date 19.04.13


#include "src/mcp_can.h"
#include <avr/power.h>

/*********************************************************************************************************************************************************************
  Adjustment section. Configure board here
*********************************************************************************************************************************************************************/

#pragma GCC optimize ("-O3")                                                                    // Max compiler optimisation level.
MCP_CAN CAN0(17);                                                                               // CS pin. Adapt to your board
#define CAN0_INT 7                                                                              // INT pin. Adapt to your board
#define DEBUG_MODE 0                                                                            // Toggle serial debug messages
const int MCP2515_CLOCK = 1;                                                                    // Set 1 for 16MHZ or 2 for 8MHZ

/*********************************************************************************************************************************************************************
*********************************************************************************************************************************************************************/


long unsigned int rxId;
unsigned char rxBuf[8], len = 0;
byte f_wakeup[] = {0x00, 0x00, 0x00, 0x00, 0x57, 0x2F, 0x00, 0x60};                             // Network management kombi, F-series
byte zbe_response[] = {0xE1, 0x9D, 0, 0xFF};
#if DEBUG_MODE
  char dbgString[128];
#endif


void setup() 
{
  disable_peripherals();
  #if DEBUG_MODE
    Serial.begin(115200);
    while(!Serial);                                                                             // 32U4, wait until virtual port initialized
  #endif
  
  while (CAN_OK != CAN0.begin(MCP_STDEXT, CAN_100KBPS, MCP2515_CLOCK)){
    #if DEBUG_MODE
      Serial.println("Error Initializing MCP2515. Re-trying.");
    #endif
    delay(5000);
  }

  #if DEBUG_MODE
    Serial.println("MCP2515 Initialized successfully.");
  #endif 

  CAN0.init_Mask(0, 0, 0x07FF0000);                                                             // Mask matches: 07FF (standard ID) and all bytes
  CAN0.init_Mask(1, 0, 0x07FF0000);                                                             // Mask matches: 07FF (standard ID) and all bytes
  CAN0.init_Filt(0, 0, 0x04E20000);                                                             // Filter CIC Network management (sent when CIC is on)                 
  CAN0.init_Filt(1, 0, 0x02730000);                                                             // Filter CIC status. 

  CAN0.setMode(MCP_NORMAL);                                                                     // Normal Mode
  pinMode(CAN0_INT, INPUT);                                                                     // Configuring pin for (INT)erupt
}

void loop()
{
  if(!digitalRead(CAN0_INT)) {                                                                  // If INT pin is pulled low, read receive buffer
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    if (rxId == 0x273){
      zbe_response[2] = rxBuf[7];
      CAN0.sendMsgBuf(0x277, 4, zbe_response);                                                  // Acknowledge must be sent three times
      CAN0.sendMsgBuf(0x277, 4, zbe_response);
      CAN0.sendMsgBuf(0x277, 4, zbe_response);
      #if DEBUG_MODE
        sprintf(dbgString, "Sent ZBE response to CIC with counter: 0x%X\n", rxBuf[7]);
        Serial.print(dbgString);
      #endif
    } else {                                                                                    // As per filters if not 0x273 rxId must be 0x4E2
        CAN0.sendMsgBuf(0x560, 8, f_wakeup);
        #if DEBUG_MODE
          Serial.println("Sent wake-up message");
        #endif
    }
  } 
}

void disable_peripherals()
{
  power_usart0_disable();                                                                       // Disable UART
  power_usart1_disable();                                                                       
  power_twi_disable();                                                                          // Disable I2C
  power_timer1_disable();                                                                       // Disable unused timers. 0 still on.
  power_timer2_disable();
  power_timer3_disable();
  ADCSRA = 0;
  power_adc_disable();                                                                          // Disable Analog to Digital converter
}
