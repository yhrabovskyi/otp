// Includes
#include "main.h"
#include "menu.h"

/******************************************************************************/
// Криптографічні змінні   
// Масив для зберігання НМАС-Value
uint8_t digest[SHA1_HASH_BYTES];        

// Масив для зберігання 8-байтної мітки часу TF
uint8_t counter[8];

// Масив для зберігання лічильника HOTP
uint8_t counter_of_hotp[8];
        
// Секретний ключ
uint8_t hmac_key[MAX_USERS][20];

// Містить пароль для вибрання нового користувача
uint8_t user_passwd[MAX_USERS][4];

/******************************************************************************/
// Структури для ініціалізації модулів мікроконтролера (RTC, EXTI, NVIC)
RTC_TimeTypeDef  RTC_TimeStructure;
RTC_DateTypeDef  RTC_DateStructure;
RTC_InitTypeDef  RTC_InitStructure;
RTC_AlarmTypeDef RTC_AlarmStructure;

EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

// Лічильник секунд після натискання кнопки генерації ОП
uint32_t  Second_Downcounter;

// Лічильник дня року (1 січня - 0)
uint32_t Day_Of_Year_Counter;

// Копія поточного року
uint32_t Current_Year_Copy;

// Поточний стан ТОТР-токена
uint32_t State = AFTER_POR_PDR_RESET;

// Зберігає поточного користувача і алгоритм генерації
// Користувач: 0x000000FF
// Алгоритм: 0x000F0000
uint32_t preferences;

// Визначає чи коротко була натиснута кнопка
// TRUE, FALSE
uint32_t is_short_pressed;

// Визначає чи була кнопка натиснута
uint32_t is_button_pressed;

// Лічильник секунд бездіяльності
uint32_t Second_NOP_Downcounter;

extern uint32_t temp_preferences;
extern uint32_t cmnds;
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main( void )
{
    // Дозвлити доступ до RTC
    PWR_RTCAccessCmd( ENABLE );    
    
    // Дозвлити відладку в режимах STOP i STANDBY
    DBGMCU_Config( DBGMCU_STOP | DBGMCU_STANDBY, ENABLE );
    
    // ВИЗНАЧАЄМО СТАН СИСТЕМИ
    
    // Перевіряємо, чи відбувся вихід з режиму STANDBY або POR/PDR
    if ( PWR_GetFlagStatus( PWR_FLAG_SB ) == RESET )
    {
        State = AFTER_POR_PDR_RESET;   
        
        // Можливий варіант в режимі відладки
        if ( RTC_ReadBackupRegister( RTC_BKP_DR2 ) == 0x55AA33EE )
        { State = AFTER_POR_PDR_RESET_DEBUG; }        
    }    
    else    // Відбувся вихід з режиму STANDBY
    {
        State = AFTER_STANDBY_RESET; 
        
        // Якщо при виході відбувся збій
        if ( RTC_ReadBackupRegister( RTC_BKP_DR2 ) != 0x55AA33EE )
        { State = AFTER_FAIL; }        
        
        // Очистити прапорець STANDBY
        PWR_ClearFlag( PWR_FLAG_SB );

        // Відбувся вихід внаслідок RTC_ALARMA - початок нового дня
        if ( RTC_GetITStatus( RTC_IT_ALRA ) != RESET )
        {
            State = AFTER_RTC_ALARMA; 
            
            // Скинути прапорець RTC AlarmA 
            RTC_ClearITPendingBit( RTC_IT_ALRA );            
        }
        else
        {
            if( PWR_GetFlagStatus( PWR_FLAG_WU ) != RESET )
            {
                // Відбувся вихід з STANDBY через натискання кнопки
                State = AFTER_OTP_BUTTON_WAKEUP;
            
                // Очистити прапорець WakeUp
                PWR_ClearFlag( PWR_FLAG_WU );
            }           
        }
    }
        
    // ВИКОНУЄМО НЕОБХІДНІ ДІЇ
      
    // Налаштування тактових сигналів для ТОТР-токена
    RCC_Configuration();    

    // Якщо відбулося перше включення ТОТР-токена
    if( State == AFTER_POR_PDR_RESET )
    { 
        RTC_Configuration();
        
        // Виставляємо користувача 00, алгоритм TOTP
        preferences = TOTP_ALGORITHM + 0;
        RTC_WriteBackupRegister( RTC_BKP_DR3, preferences );
        
        // Скидуємо лічильники HOTP
        reset_counters_of_hotp();
    }    
    // Якщо почався новий день
    else if ( State == AFTER_RTC_ALARMA )
    {
        // Прочитати день року і поточний рік з Backup-регістрів
        Day_Of_Year_Counter = RTC_ReadBackupRegister( RTC_BKP_DR0 );
        Current_Year_Copy   = RTC_ReadBackupRegister( RTC_BKP_DR1 );                               
      
        // Прочитати значення року
        RTC_GetDate( RTC_Format_BIN, &RTC_DateStructure );
                
        // Якщо почався новий рік - обнулити лічильник днів року і встановити 
        // нове значення року
        if( Current_Year_Copy != RTC_DateStructure.RTC_Year )
        { 
            Day_Of_Year_Counter = 0; 
            Current_Year_Copy   = RTC_DateStructure.RTC_Year;
            
            RTC_WriteBackupRegister( RTC_BKP_DR0, Day_Of_Year_Counter );
            RTC_WriteBackupRegister( RTC_BKP_DR1, Current_Year_Copy );                        
         }
         else
         {
            Day_Of_Year_Counter++; 
            RTC_WriteBackupRegister( RTC_BKP_DR0, Day_Of_Year_Counter );
         }
    }
    
    // Ініціалізація портів вводу-виводу GPIO
    // Модифіковано, 2014-03-01
    Init_GPIOs();    
        
    if ( State != AFTER_OTP_BUTTON_WAKEUP )    
    {
        // Заборонити пробудження Wakeup кожну секунду від RTC
        RTC_WakeUpCmd( DISABLE );     
      
        EXTI_ClearITPendingBit( EXTI_Line20 ); 
        RTC_ClearITPendingBit(  RTC_IT_WUT );        
      
        // Дозволити пробудження від WKUP pin 1
        PWR_WakeUpPinCmd( PWR_WakeUpPin_1, ENABLE );    
  
        PWR_EnterSTANDBYMode();
    }   
   
    // Встановити внутрішній регулятор напруги на 1.8 В
    PWR_VoltageScalingConfig( PWR_VoltageScaling_Range1 );
  
    // Чекати, поки регулятор напруги не буде готовий
    while ( PWR_GetFlagStatus( PWR_FLAG_VOS ) != RESET )
    {};
    
    // Ініціалізувати LCD
    LCD_GLASS_Init();
    
    // Ініціалізувати секретні ключі
    secret_keys_init();
    // Ініціалізувати паролі для зміни користувача
    users_passwd_init();
    
    // Налаштування переривання пробудження від RTC кожну секунду    
    EXTI_InitStructure.EXTI_Line    = EXTI_Line20;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );
  
    // Дозволити переривання RTC Wakeup 
    NVIC_InitStructure.NVIC_IRQChannel                   = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &NVIC_InitStructure );      
    
    // Налаштування кнопки і її переривання -> Init_GPIOs()
    // 2014-03-01
    
    // Скидуємо час бездіяльності
    Second_NOP_Downcounter = LCD_ON_NOP;
    
    // Дозволити General interrupts 
    enableGlobalInterrupts();    
    
    // Включити дисплей
    LCD_Cmd( ENABLE );
    
    // Дозволити пробудження Wakeup кожну секунду від RTC
    RTC_WakeUpCmd( ENABLE );
    
    // Читаємо налаштування (користувач і алгоритм)
    preferences = RTC_ReadBackupRegister( RTC_BKP_DR3 );
    temp_preferences = preferences;
    
    // Читаємо лічильник
    load_to_counter_of_hotp(preferences & USER_NUMBER);
    
    is_short_pressed = FALSE;
    is_button_pressed = FALSE;
    
    // Генеруємо, виводимо і т.д.
    gen_password();
    
    // Задати затримку перед виключенням дисплею
    Second_NOP_Downcounter = LCD_ON_NOP;
    
    // Визначаємо, який таймер вимкнення дисплею використовувати
    cmnds = (cmnds & (~SLCT_TIMER)) | SET_LCD_ON_NOP;
    
    //is_long_pressed = FALSE;
    is_short_pressed = FALSE;
    is_button_pressed = FALSE;
    
    // Переходимо в меню
    menu_lvl_1();
}		
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Скидаємо лічильники користувачів алгоритму HOTP
void reset_counters_of_hotp(void)
{
  RTC_WriteBackupRegister( RTC_BKP_DR10, 0 );
  RTC_WriteBackupRegister( RTC_BKP_DR11, 0 );
  RTC_WriteBackupRegister( RTC_BKP_DR12, 1010 );
  RTC_WriteBackupRegister( RTC_BKP_DR13, 0 );
  RTC_WriteBackupRegister( RTC_BKP_DR14, 1000010 );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Завантажити з бекап-регістра у лічильник значення лічильника користувача user
void load_to_counter_of_hotp(uint32_t user)
{
  uint32_t data;
  
  // Вибираємо регістр
  switch (user)
  {
  case 0:
    data = RTC_ReadBackupRegister( RTC_BKP_DR10 );
    break;
    
  case 1:
    data = RTC_ReadBackupRegister( RTC_BKP_DR11 );
    break;
    
  case 2:
    data = RTC_ReadBackupRegister( RTC_BKP_DR12 );
    break;
    
  case 3:
    data = RTC_ReadBackupRegister( RTC_BKP_DR13 );
    break;
    
  case 4:
    data = RTC_ReadBackupRegister( RTC_BKP_DR14 );
    break;
  }
  
  /*// Заносимо прочитані дані окремо кожною цифрою
  counter_of_hotp[0] = data / 10000000UL;
  data = data - counter_of_hotp[0] * 10000000UL;
  
  counter_of_hotp[1] = data / 1000000UL;
  data = data - counter_of_hotp[1] * 1000000UL;
  
  counter_of_hotp[2] = data / 100000UL;
  data = data - counter_of_hotp[2] * 100000UL;
  
  counter_of_hotp[3] = data / 10000UL;
  data = data - counter_of_hotp[3] * 10000UL;
  
  counter_of_hotp[4] = data / 1000UL;
  data = data - counter_of_hotp[4] * 1000UL;
  
  counter_of_hotp[5] = data / 100UL;
  data = data - counter_of_hotp[5] * 100UL;
  
  counter_of_hotp[6] = data / 10UL;
  data = data - counter_of_hotp[6] * 10UL;
  
  counter_of_hotp[7] = data;*/
  counter_of_hotp[7] = data & 0x000000FF;
  counter_of_hotp[6] = (data & 0x0000FF00) >> 8;
  counter_of_hotp[5] = (data & 0x00FF0000) >> 16;
  counter_of_hotp[4] = (data & 0xFF000000) >> 24;
  counter_of_hotp[3] = 0;
  counter_of_hotp[2] = 0;
  counter_of_hotp[1] = 0;
  counter_of_hotp[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Зберігає лічильник користувача у бекап-регістр
void save_from_counter_of_hotp(uint32_t user)
{
  uint32_t data;
  
  data = 0;
  /*
  data += counter_of_hotp[0] * 10000000UL;
  data += counter_of_hotp[1] * 1000000UL;
  data += counter_of_hotp[2] * 100000UL;
  data += counter_of_hotp[3] * 10000UL;
  data += counter_of_hotp[4] * 1000UL;
  data += counter_of_hotp[5] * 100UL;
  data += counter_of_hotp[6] * 10UL;
  data += counter_of_hotp[7];*/
  data = (uint32_t) counter_of_hotp[7];
  data |= (uint32_t) counter_of_hotp[6] << 8;
  data |= (uint32_t) counter_of_hotp[5] << 16;
  data |= (uint32_t) counter_of_hotp[4] << 24;
  
  // Вибираємо регістр
  switch (user)
  {
  case 0:
    RTC_WriteBackupRegister( RTC_BKP_DR10, data );
    break;
    
  case 1:
    RTC_WriteBackupRegister( RTC_BKP_DR11, data );
    break;
    
  case 2:
    RTC_WriteBackupRegister( RTC_BKP_DR12, data );
    break;
    
  case 3:
    RTC_WriteBackupRegister( RTC_BKP_DR13, data );
    break;
    
  case 4:
    RTC_WriteBackupRegister( RTC_BKP_DR14, data );
    break;
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void secret_keys_init(void)
{
  uint32_t i;
  
  // 12345678901234567890
  for (i = 1; i <= 20; i++)
  {
    hmac_key[0][i-1] = (i % 10) + 0x30;
  }
  
  // abcdefghijklmnopqrst
  for (i = 0; i < 20; i++)
  {
    hmac_key[1][i] = i + 0x61;
  }
  
  // ABCDEFGHIJKLMNOPQRST
  for (i = 0; i < 20; i++)
  {
    hmac_key[2][i] = i + 0x41;
  }
  
  // 12345678901234567890
  for (i = 1; i <= 20; i++)
  {
    hmac_key[3][i-1] = (i % 10) + 0x30;
  }
  
  // *+,-./0123456789:;<=
  for (i = 0; i < 20; i++)
  {
    hmac_key[4][i] = i + 0x2A;
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void users_passwd_init(void)
{
  uint32_t i;
  
  for (i = 0; i < 4; i++)
  {
    user_passwd[0][i] = i;
  }
  
  for (i = 0; i < 4; i++)
  {
    user_passwd[1][i] = i + 1;
  }
  
  for (i = 0; i < 4; i++)
  {
    user_passwd[2][i] = i + 2;
  }
  
  for (i = 0; i < 4; i++)
  {
    user_passwd[3][i] = i + 3;
  }
  
  for (i = 0; i < 4; i++)
  {
    user_passwd[4][i] = i + 4;
  }
}
////////////////////////////////////////////////////////////////////////////////
