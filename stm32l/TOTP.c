#include "TOTP.h"

extern uint8_t counter_of_hotp[8];

////////////////////////////////////////////////////////////////////////////////
void user_counter_up(void)
{
  counter_of_hotp[7] += 1;
  
  if (counter_of_hotp[7] == 0)
  {
    counter_of_hotp[6] += 1;
    
    if (counter_of_hotp[6] == 0)
    {
      counter_of_hotp[5] += 1;
      
      if (counter_of_hotp[5] == 0)
      {
        counter_of_hotp[4] += 1;
      }
    }
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void user_counter_down(void)
{
  uint32_t data;
  
  data = 0;
  
  data = (uint32_t) counter_of_hotp[7];
  data |= (uint32_t) counter_of_hotp[6] << 8;
  data |= (uint32_t) counter_of_hotp[5] << 16;
  data |= (uint32_t) counter_of_hotp[4] << 24;
  
  // Якщо нуль, то виходимо
  if (data == 0)
  {
    return;
  }
  
  data--;
  
  counter_of_hotp[7] = data & 0x000000FF;
  counter_of_hotp[6] = (data & 0x0000FF00) >> 8;
  counter_of_hotp[5] = (data & 0x00FF0000) >> 16;
  counter_of_hotp[4] = (data & 0xFF000000) >> 24;
}
////////////////////////////////////////////////////////////////////////////////
