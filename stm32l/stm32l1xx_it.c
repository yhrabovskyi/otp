extern uint32_t  Second_Downcounter;
extern uint8_t counter[8];

// Для виведення "стану" на дисплеї
extern uint8_t t_bar[2];

extern uint8_t is_short_pressed;
extern uint8_t is_button_pressed;
extern uint32_t cmnds;
extern uint32_t Second_NOP_Downcounter;
extern uint32_t is_two_times;
extern uint32_t first_time_complete;
extern uint8_t for_update[8];

void UserButtonHandler(void);

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// Підпрограма обробки переривання пробудження від RTC кожну 1 секунду
void RTC_WKUP_IRQHandler ( void )
{
    uint32_t seconds;
    
    // Визначаємо, який таймер ввімкнено
    // Якщо таймер при генерації пароля
    if ( (cmnds & SLCT_TIMER) == SET_LCD_ON_TIME )
    {
      // Секунда пройшла
      Second_Downcounter--;
      seconds = Second_Downcounter;
      on_off_lcd_bars(seconds);
    }
    
    // Якщо таймер при бездіяльності
    if ( (cmnds & SLCT_TIMER) == SET_LCD_ON_NOP )
    {
      // Секунда пройшла
      Second_NOP_Downcounter--;
      seconds = Second_NOP_Downcounter;
    }
    
    // Якщо час минув і потрібно ще раз згенерувати TOTP
    if ( (seconds == 0) && (is_two_times == TRUE) )
    {
      is_two_times = FALSE;
      
      first_time_complete = TRUE;
      
      // Очистити прапорці переривання від ГРЧ
      EXTI_ClearITPendingBit( EXTI_Line20 ); 
      RTC_ClearITPendingBit( RTC_IT_WUT );   
      
      return;
    }
    
    // Якщо час минув
    if( seconds == 0 )
    {     
        // Заборонити відлік секунд
        RTC_WakeUpCmd( DISABLE );
      
        // Виключити дисплей
        LCD_Cmd( DISABLE );    
        
        // Очистити прапорці переривання від ГРЧ
        EXTI_ClearITPendingBit( EXTI_Line20 ); 
        RTC_ClearITPendingBit( RTC_IT_WUT );        
        
        // Дозволити пробудження від WKUP pin 1
        PWR_WakeUpPinCmd( PWR_WakeUpPin_1, ENABLE );    
        
        PWR_EnterSTANDBYMode();	
    }
    
    // Очистити прапорці переривання від ГРЧ
    EXTI_ClearITPendingBit( EXTI_Line20 ); 
    RTC_ClearITPendingBit( RTC_IT_WUT );   
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Підпрограма обробки переривання натискання кнопки у ввімкненому режимі
void EXTI0_IRQHandler( void )
{
  /* Disable general interrupts */
  disableGlobalInterrupts();
  
  // Визначаємо як саме натиснуто
  UserButtonHandler();
  
  // Позначаємо, що кнопка натиснута
  is_button_pressed = TRUE;
  
  EXTI_ClearITPendingBit(EXTI_Line0);
  enableGlobalInterrupts();
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Визначення тривалості натискання кнопки (звичайне або довге натискання)
void UserButtonHandler (void)
{ 
  uint32_t i=0;
  
  // Якщо кнопка натиснена більше однієї секунди (приблизно)
  while ((USERBUTTON_GPIO_PORT->IDR & USERBUTTON_GPIO_PIN) == 1 )
  {
    i++;
    if (i == 0x0040000)
    {
      // Довго натиснута
      is_short_pressed = FALSE;
      return;
    }
  }
  
  // Коротко натиснута
  is_short_pressed = TRUE;
  return;
}
