#include "main.h"
#include "menu.h"

extern uint32_t preferences;
extern uint32_t is_short_pressed;
extern uint32_t is_button_pressed;
extern uint32_t Second_Downcounter;
extern uint32_t Second_NOP_Downcounter;
extern RTC_TimeTypeDef RTC_TimeStructure;
extern RTC_DateTypeDef RTC_DateStructure;
extern uint8_t counter[8];
extern uint8_t counter_of_hotp[8];
extern uint32_t Day_Of_Year_Counter;
extern uint32_t Current_Year_Copy;
extern uint8_t user_passwd[MAX_USERS][4];

// Для виведення bars на дисплей
extern uint8_t t_bar[2];

uint32_t temp_preferences;

// Зберігає вибрані поточні команди
// Вибір генерації чи налашт. елемента синхр.: 0х00F00000
// Вибір додавання чи віднімання до елемента: 0х30000000
// Вибір вимкнення дисплею після генерації пароля чи бездіяльності: 0х0F000000
// Вибір елемента синхронізації за часом 0x0000000F
uint32_t cmnds;

// Показує, що при TOTP час показу поточного пароля дуже малий
// і тому згенерувати також наступний
uint32_t is_two_times = FALSE;

// Показує, що перший раз TOTP згенерувало (викор. в поєднанні з попереднім)
uint32_t first_time_complete = FALSE;

// Зберігається сума днів місяців
const uint32_t sum_of_days_in_month[12] =
{ 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

////////////////////////////////////////////////////////////////////////////////
// Menu items: User00, ..., USER04, EXIT
void menu_lvl_1(void)
{
  // Переприсвоюємо користувача і алгоритм для того, щоб їх не змінювати,
  // тільки після генерації пароля
  //temp_preferences = preferences;
  
  // Оскільки ми вийшли з показу пароля і хочемо попасти на перший пункт
  // меню, то скидуємо налаштування
  temp_preferences = 0;
  
  // Оскільки ми вийшли з моменту, коли був відображений пароль, то тепер
  // показуємо поточного користувача
  LCD_show_user_or_exit(SHOW_CURR_ITEM);
  
  // Вказує чи введений пароль вірний
  uint32_t is_passwd_correct;
  
  // Вихід з цього циклу:
  // Пункт меню 1-го рівня EXIT - у standby режим
  // Пункт меню 3-го рівня GENPWD - у standby режим після 30 секунд
  // Через 1 хв бездіяльності - у standby режим
  while ( 1 )
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    // Меню, рівень 1 ////////////////////////////////
    switch (is_button_pressed)
    {
      // Якщо кнопка натиснута
    case TRUE:
      // Скидуємо час бездіяльності
      Second_NOP_Downcounter = LCD_ON_NOP;
      // Скидуємо прапорець натискання
      is_button_pressed = FALSE;
      
      switch (is_short_pressed)
      {
        // Якщо коротке натиснення
      case TRUE:
        // Скидуємо прапорець
        is_short_pressed = FALSE;
        // Перевіряємо чи це не EXIT
        // Якщо EXIT, йдемо у standby
        if ( (temp_preferences & USER_NUMBER) == MAX_USERS)
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
        
        // Якщо пароль введено вірно, переходимо в наступне меню
        is_passwd_correct = check_passwd();
        if (is_passwd_correct == TRUE)
        {
          menu_lvl_2();
        }
        // Відображуємо поточного користувача і переходимо в режим
        // очікування натискання кнопки
        LCD_show_user_or_exit(SHOW_CURR_ITEM);
        break;
        
        // Якщо довге натиснення, то показати наступний пункт меню
      case FALSE:
        LCD_show_user_or_exit(SHOW_NEXT_ITEM);
      break;
      
      // Невідома помилка
      default:
        ;
        break;
      }
      break;
      
      // Якщо кнопка не натиснута
    case FALSE:
      ;
      break;
      
      // Невідома помилка
    default:
      ;
      break;
    }
    //                ////////////////////////////////
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Menu items: TOTP, HOTP, RETURN
void menu_lvl_2(void)
{
  // Завдяки неї повертаємося з меню нижнього рівня на верхній
  uint32_t return_cmd = FALSE;
  
  // Виводимо поточний алгоритм
  LCD_show_algorithm_or_return(SHOW_CURR_ITEM);
  
  // Виходимо з 2-го рівня тільки при RETURN команді
  while ( return_cmd == FALSE )
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    // Меню, рівень 2 ////////////////////////////////
    switch (is_button_pressed)
    {
      // Кнопка натиснена
    case TRUE:
      // Скидуємо час бездіяльності
      Second_NOP_Downcounter = LCD_ON_NOP;
      // Скидуємо прапорець натискання
      is_button_pressed = FALSE;
      
      switch (is_short_pressed)
      {
        // Якщо коротке натиснення
      case TRUE:
        // Скидуємо прапорець
        is_short_pressed = FALSE;
        // Перевіряємо чи це не RETURN
        // Якщо так, виходимо на перший рівень
        if ( (temp_preferences & ALGORITHM_TYPE) == RETURN_OF_ALGORITHMS_MENU )
        {
          // Скидуємо вибрану команду для того, щоб при поверненні назад в це
          // меню показувати перший пункт меню, а не останній вибраний
          temp_preferences &= ~ALGORITHM_TYPE;
          return_cmd = TRUE;
          break;
        }
        // Якщо ні, то заходимо на третій рівень меню
        menu_lvl_3();
        // Відображуємо останній показаний пункт меню і переходимо в режим
        // очікування натискання кнопки
        LCD_show_algorithm_or_return(SHOW_CURR_ITEM);
        break;
        
        // Якщо довге натиснення, то показати наступний пункт меню
      case FALSE:
        LCD_show_algorithm_or_return(SHOW_NEXT_ITEM);
        break;
      default:
        ;
        break;
      }
      break;
      
      // Якщо кнопка не натиснута
    case FALSE:
      ;
      break;
      
      // Невідома помилка
    default:
      ;
      break;
    }
    //                ////////////////////////////////
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Menu items: GENPWD, SYNCHR, RETURN
void menu_lvl_3(void)
{
  // Скидуємо набір команд
  cmnds = 0;
  
  // Задати затримку перед виключенням дисплею
  Second_NOP_Downcounter = LCD_ON_NOP;
  
  // Визначаємо, який таймер вимкнення дисплею використовувати
  cmnds = (cmnds & (~SLCT_TIMER)) | SET_LCD_ON_NOP;
    
  // Завдяки неї повертаємося з меню нижнього рівня на верхній
  uint32_t return_cmd = FALSE;
  
  // Показуємо перший пункт меню
  LCD_show_menu_item_lvl_3(SHOW_CURR_ITEM);
  
  // Поки не отримаємо команду RETURN
  while ( return_cmd == FALSE )
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    // Меню, рівень 3 ////////////////////////////////
    switch (is_button_pressed)
    {
      // Якщо кнопка натиснута
    case TRUE:
      // Скидуємо час бездіяльності
      Second_NOP_Downcounter = LCD_ON_NOP;
      // Скидуємо прапорець натискання
      is_button_pressed = FALSE;
      
      switch (is_short_pressed)
      {
        // Якщо коротке натиснення
      case TRUE:
        // Скидуємо прапорець
        is_short_pressed = FALSE;
        // Перевіряємо чи це не RETURN
        // Якщо так, виходимо на попередній рівень
        if ( (cmnds & SLCT_MENU_LVL_3) == RETURN_OF_MENU_LVL_3 )
        {
          return_cmd = TRUE;
          break;
        }
        // Якщо ні, то виконуємо вибрану команду
        switch (cmnds & SLCT_MENU_LVL_3)
        {
          // Вибрано GENPWD
        case GENPWD:
          gen_password();
          break;
          
          // Вибрано SYNCHR
        case SYNCHR:
          menu_synchr();
          
          // Після виходу не пам'ятаємо останнього вибраного пункту меню
          // menu_synchr
          cmnds &= ~SYNCHR_MENU_ITEM;
          break;
          
          // Невідома помилка
        default:
          ;
          break;
        }
        // Відображуємо останній показаний пункт меню і переходимо в режим
        // очікування натискання кнопки
        LCD_show_menu_item_lvl_3(SHOW_CURR_ITEM);
        break;
        
        // Якщо довге натискання, то показати наступний пункт меню
      case FALSE:
        LCD_show_menu_item_lvl_3(SHOW_NEXT_ITEM);
        break;
        
        // Невідома помилка
      default:
        ;
        break;
      }
      break;
      
      // Якщо кнопка не натиснута
    case FALSE:
      ;
      break;
      
      // Невідома помилка
    default:
      ;
      break;
    }
    //                ////////////////////////////////
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Меню синхронізації: +, -, RETURN
void menu_synchr(void)
{ 
  // Завдяки неї повертаємося з меню нижнього рівня на верхній
  uint32_t return_cmd = FALSE;
  
  // Показуємо перший пункт меню
  LCD_show_synchr_menu_item(SHOW_CURR_ITEM);
  
  // Поки не отримаємо команду RETURN
  while ( return_cmd == FALSE )
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    switch (is_button_pressed)
    {
      // Якщо кнопка натиснута
    case TRUE:
      // Скидуємо час бездіяльності
      Second_NOP_Downcounter = LCD_ON_NOP;
      // Скидуємо прапорець натискання
      is_button_pressed = FALSE;
      
      switch (is_short_pressed)
      {
        // Якщо коротке натискання
      case TRUE:
        // Скидуємо прапорець
        is_short_pressed = FALSE;
        
        // Перевіряємо чи це не RETURN
        // Якщо так, то виходимо на попередній рівень
        if ( (cmnds & SLCT_OPER) == RETURN_OF_SYNCHR_MENU )
        {
          // Скидуємо останню показану команду
          cmnds &= ~SLCT_OPER;
          
          return_cmd = TRUE;
          break;
        }
        
        // Синхронізовуємо елемент
        synchr_elem();
        
        // Відображуємо останній показаний пункт меню і переходимо в режим
        // очікування натискання кнопки
        LCD_show_synchr_menu_item(SHOW_CURR_ITEM);
        break;
        
        // Якщо довге натискання, то показати наступний пункт меню
      case FALSE:
        LCD_show_synchr_menu_item(SHOW_NEXT_ITEM);
        break;
        
      default:
        ;
        break;
      }
      break;
      
      // Якщо кнопка не натиснута
    case FALSE:
      ;
      break;
      
    default:
      ;
      break;
    }
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Виводить на дисплей (номер поточного користувача + і)
void LCD_show_user_or_exit(uint32_t i)
{
  // Містить значення, яке відображується на дисплеї
  uint8_t curr_cmd[8];
  
  curr_cmd[6] = 0;
  curr_cmd[7] = 0;
  
  if (i == SHOW_NEXT_ITEM)
  {
    // Переходимо на користувача + і
    temp_preferences += i;
  }
  
  // Це типу виводить останній пункт меню - EXIT
  if ( (temp_preferences & USER_NUMBER) == MAX_USERS )
  {
    // Виводимо на дисплей EXIT
    curr_cmd[0] = 'E';
    curr_cmd[1] = 'X';
    curr_cmd[2] = 'I';
    curr_cmd[3] = 'T';
    curr_cmd[4] = ' ';
    curr_cmd[5] = ' ';
    LCD_GLASS_DisplayString( curr_cmd );
    
    return;
  }
  
  // Якщо номер користувача більший за максимальну кількість + 1,
  // То починаємо з нульового користувача
  if ( (temp_preferences & USER_NUMBER) > MAX_USERS )
  {
    temp_preferences -= MAX_USERS + 1;
  }
  
  // Виводимо на дисплей USERXX
  curr_cmd[0] = 'U';
  curr_cmd[1] = 'S';
  curr_cmd[2] = 'E';
  curr_cmd[3] = 'R';
  curr_cmd[4] = (temp_preferences & USER_NUMBER) / 10 + 0x30;
  curr_cmd[5] = ( (temp_preferences & USER_NUMBER) % 10 ) + 0x30;
  LCD_GLASS_DisplayString( curr_cmd );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Виводить на дисплей (алгоритм + i), може бути TOTP, HOTP або RETURN
void LCD_show_algorithm_or_return(uint32_t i)
{
  // Містить значення, яке відображується на дисплеї
  uint8_t curr_cmd[8];
  
  curr_cmd[6] = 0;
  curr_cmd[7] = 0;
  
  if (i == SHOW_NEXT_ITEM)
  {
    // Переходимо на наступний пункт меню рівня 2
    temp_preferences += MENU_LVL_2_ITEM;
  }
  
  // Якщо наступний пункт return
  if ( (temp_preferences & ALGORITHM_TYPE) == RETURN_OF_ALGORITHMS_MENU )
  {
    curr_cmd[0] = 'R';
    curr_cmd[1] = 'E';
    curr_cmd[2] = 'T';
    curr_cmd[3] = 'U';
    curr_cmd[4] = 'R';
    curr_cmd[5] = 'N';
    LCD_GLASS_DisplayString( curr_cmd );
    
    return;
  }
  
  // Переходимо return -> початок меню
  if ( (temp_preferences & ALGORITHM_TYPE) > RETURN_OF_ALGORITHMS_MENU )
  {
    temp_preferences -= RETURN_OF_ALGORITHMS_MENU + MENU_LVL_2_ITEM;
  }
  
  // Оскільки лише два алгоритми, робимо простим if\else
  // Якщо TOTP
  if ( (temp_preferences & ALGORITHM_TYPE) == TOTP_ALGORITHM)
  {
    curr_cmd[0] = 'T';
  }
  // Значить, HOTP
  else
  {
    curr_cmd[0] = 'H';
  }
  
  curr_cmd[1] = 'O';
  curr_cmd[2] = 'T';
  curr_cmd[3] = 'P';
  curr_cmd[4] = ' ';
  curr_cmd[5] = ' ';
  LCD_GLASS_DisplayString( curr_cmd );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Виводить на дисплей GENPWD, SYNCHR, RETURN
void LCD_show_menu_item_lvl_3(uint32_t i)
{
  // Містить значення, яке відображується на дисплеї
  uint8_t curr_cmd[8];
  
  curr_cmd[6] = 0;
  curr_cmd[7] = 0;
  
  if (i == SHOW_NEXT_ITEM)
  {
    cmnds += MENU_LVL_3_ITEM;
  }
  
  // Якщо наступний пункт меню RETURN
  if ( (cmnds & SLCT_MENU_LVL_3) == RETURN_OF_MENU_LVL_3)
  {
    curr_cmd[0] = 'R';
    curr_cmd[1] = 'E';
    curr_cmd[2] = 'T';
    curr_cmd[3] = 'U';
    curr_cmd[4] = 'R';
    curr_cmd[5] = 'N';
    LCD_GLASS_DisplayString( curr_cmd );
    
    return;
  }
  
  // Переходимо return -> початок меню
  if ( (cmnds & SLCT_MENU_LVL_3) > RETURN_OF_MENU_LVL_3 )
  {
    cmnds -= RETURN_OF_MENU_LVL_3 + MENU_LVL_3_ITEM;
  }
  
  // Генерація пунктів меню окрім RETURN
  switch (cmnds & SLCT_MENU_LVL_3)
  {
    // Пункт меню GENPWD
  case GENPWD:
    curr_cmd[0] = 'P';
    curr_cmd[1] = 'A';
    curr_cmd[2] = 'S';
    curr_cmd[3] = 'S';
    curr_cmd[4] = 'W';
    curr_cmd[5] = 'D';
    LCD_GLASS_DisplayString( curr_cmd );
    
    return;
    break;
    
    // Пункт меню SYNCHR
  case SYNCHR:
    curr_cmd[0] = 'S';
    curr_cmd[1] = 'Y';
    curr_cmd[2] = 'N';
    curr_cmd[3] = 'C';
    curr_cmd[4] = 'H';
    curr_cmd[5] = ' ';
    LCD_GLASS_DisplayString( curr_cmd );
    
    return;
    break;
    
    // Невідома помилка
  default:
    ;
    break;
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Відображує на дисплей меню синхронізації +, -, RETURN
void LCD_show_synchr_menu_item(uint32_t i)
{
  // Містить значення, яке відображується на дисплеї
  uint8_t curr_cmd[8];
  
  curr_cmd[6] = 0;
  curr_cmd[7] = 0;
  
  if (i == SHOW_NEXT_ITEM)
  {
    cmnds += SYNCHR_MENU_ITEM;
  }
  
  // Якщо наступний пункт меню RETURN
  if ( (cmnds & SLCT_OPER) == RETURN_OF_SYNCHR_MENU)
  {
    curr_cmd[0] = 'R';
    curr_cmd[1] = 'E';
    curr_cmd[2] = 'T';
    curr_cmd[3] = 'U';
    curr_cmd[4] = 'R';
    curr_cmd[5] = 'N';
    LCD_GLASS_DisplayString( curr_cmd );
    
    return;
  }
  
  curr_cmd[5] = ' ';
  
  // Переходимо return -> початок меню
  if ( (cmnds & SLCT_OPER) > RETURN_OF_SYNCHR_MENU )
  {
    cmnds -= RETURN_OF_SYNCHR_MENU + SYNCHR_MENU_ITEM;
  }
  
  // Генерація пунктів меню окрім RETURN
  switch (cmnds & SLCT_OPER)
  {
  case PLUS:
    curr_cmd[0] = 'P';
    curr_cmd[1] = 'L';
    curr_cmd[2] = 'U';
    curr_cmd[3] = 'S';
    curr_cmd[4] = ' ';
    break;
    
  case MINUS:
    curr_cmd[0] = 'M';
    curr_cmd[1] = 'I';
    curr_cmd[2] = 'N';
    curr_cmd[3] = 'U';
    curr_cmd[4] = 'S';
    break;
  }
  
  LCD_GLASS_DisplayString( curr_cmd );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Генерує пароль
void gen_password(void)
{
  // Якщо зараз змінюється користувач
  if ( (preferences & USER_NUMBER) != (temp_preferences & USER_NUMBER) )
  {
    // То зберігаємо поточний лічильник
    save_from_counter_of_hotp(preferences & USER_NUMBER);
    
    // І завантажуємо новий
    load_to_counter_of_hotp(temp_preferences & USER_NUMBER);
  }
  
  // Зберігаємо поточного користувача і пароль
  preferences = temp_preferences;
  RTC_WriteBackupRegister( RTC_BKP_DR3, preferences );
  
  // Визначаємо алгоритм, генеруємо пароль і відображаємо його на дисплей
  switch (preferences & ALGORITHM_TYPE)
  {
  case TOTP_ALGORITHM:
    // Прочитати поточний час з годинника реального часу
    RTC_GetTime( RTC_Format_BIN, &RTC_TimeStructure );
    
    // Задати затримку перед виключенням дисплею
    if ( (((RTC_TimeStructure.RTC_Seconds) % TOTP_TIME_STEP) == 0) )
    {
      Second_Downcounter = LCD_ON_TIME;
    }
    else
    {
      Second_Downcounter = TOTP_TIME_STEP - ((RTC_TimeStructure.RTC_Seconds) % TOTP_TIME_STEP);
    }
    
    
    // Якщо час дійсності пароля менший, ніж ми встигнемо ввести,
    // то також згенерувати наступний
    if (Second_Downcounter <= SECONDS_FOR_SECOND_TIME)
    {
      is_two_times = TRUE;
    }
    
    TOTP_Value_Calculate();
    LCD_GLASS_DisplayString( counter );    
    break;
    
  case HOTP_ALGORITHM:
    // Задати затримку перед виключенням дисплею
    Second_Downcounter = LCD_ON_TIME;
    
    HOTP_Value_Calculate();
    LCD_GLASS_DisplayString( counter );
    user_counter_up();
    save_from_counter_of_hotp(preferences & USER_NUMBER);
    break;
    
  default:
    ;
    break;
  }
  
  // Визначаємо, який таймер вимкнення дисплею використовувати
  cmnds = (cmnds & (~SLCT_TIMER)) | SET_LCD_ON_TIME;
  
  // Вмикаємо bars
  on_off_lcd_bars(Second_Downcounter);
  
  // Виходимо з циклу при умові довгого натиснення кнопки
  // або йдемо у standby через певний час
  while ( 1 )
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    // Згенерувати ще раз для TOTP
    if ( first_time_complete == TRUE )
    {
      first_time_complete = FALSE;
      
      // Задати затримку перед виключенням дисплею
      Second_Downcounter = LCD_ON_TIME;
      
      // Вмикаємо bars
      on_off_lcd_bars(Second_Downcounter);
      
      TOTP_Value_Calculate();
      LCD_GLASS_DisplayString( counter );
    }
    
    // Якщо кнопка натиснута, вийти з циклу
    if ( (is_button_pressed == TRUE) && (is_short_pressed == FALSE) )
    {
      break;
    }
  }
  
  // Скидуємо прапорець натискання кнопки
  is_button_pressed = FALSE;
  
  // Скидуємо умови подвійної генерації пароля для TOTP
  is_two_times = FALSE;
  first_time_complete = FALSE;
  
  // Задати затримку перед виключенням дисплею
  Second_NOP_Downcounter = LCD_ON_NOP;
  
  // Визначаємо, який таймер вимкнення дисплею використовувати
  cmnds = (cmnds & (~SLCT_TIMER)) | SET_LCD_ON_NOP;
  
  // Вимикаємо bar
  BAR0_OFF;
  BAR1_OFF;
  BAR2_OFF;
  BAR3_OFF;
  LCD_GLASS_DisplayString( counter );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Меню синхронізації елементів; розписано для HOTP елемента; для TOTP елемента
// тут викликаємо окрему функцію
void synchr_elem(void)
{
  // Використовується для виходу очікування натискання кнопки
  uint32_t return_cmd = FALSE;
  
  // Вибираємо алгоритм
  // Якщо HOTP
  if ( (temp_preferences & ALGORITHM_TYPE) == HOTP_ALGORITHM )
  {
    // Якщо синхронізовуємо лічильник іншого користувача
    if ( (preferences & USER_NUMBER) != (temp_preferences & USER_NUMBER) )
    {
      // Зберігаємо лічильник встановленого за налаштуваннями
      save_from_counter_of_hotp(preferences & USER_NUMBER);
      
      // Завантажуємо лічильник вибраного користувача
      load_to_counter_of_hotp(temp_preferences & USER_NUMBER);
    }
      
    // Поки не отримали команди повернення
    while (return_cmd == FALSE)
    {
      PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
      
      // Відображаємо значення лічильника
      LCD_show_counter_of_hotp();
      
      switch (is_button_pressed)
      {
        // Якщо кнопка натиснута
      case TRUE:
        // Скидуємо час бездіяльності
        Second_NOP_Downcounter = LCD_ON_NOP;
        
        // Скидуємо прапорець натиснення
        is_button_pressed = FALSE;
        
        switch (is_short_pressed)
        {
          // Якщо коротке натискання
        case TRUE:
          // Змінюємо лічильник на 1
          if ( (cmnds & SLCT_OPER) == PLUS)
          {
            user_counter_up();
          }
          else
          {
            user_counter_down();
          }
          // Відображаємо нове значення
          LCD_show_counter_of_hotp();
          break;
          
        case FALSE:
          // Якщо довге натискання, виходимо
          return_cmd = TRUE;
          break;
          
        default:
          ;
          break;
        }
        break;
        
      case FALSE:
        ;
        break;
        
      default:
        ;
        break;
      }
    }
    
    // Зберігаємо лічильник користувача
    save_from_counter_of_hotp(temp_preferences & USER_NUMBER);
    // Якщо синронізовувався лічильник іншого користувача
    if ( (preferences & USER_NUMBER) != (temp_preferences & USER_NUMBER) )
    {
      // Завантажуємо лічильник встановленого користувача
      load_to_counter_of_hotp(preferences & USER_NUMBER);
    }
  }
  // Якщо TOTP
  else
  {
    // Переходимо в окрему функцію
    synchr_time_elems();
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Відображує лічильник HOTP на дисплей
void LCD_show_counter_of_hotp(void)
{
  uint32_t i;
  // Вміст, який виводиться на дисплей
  uint8_t to_show[8];
  
  uint32_t data = 0;
  
  data = (uint32_t) counter_of_hotp[7];
  data |= (uint32_t) counter_of_hotp[6] << 8;
  data |= (uint32_t) counter_of_hotp[5] << 16;
  data |= (uint32_t) counter_of_hotp[4] << 24;
  
  for (i = 0; i < 6; i++)
  {
    to_show[5-i] = (data % 10) + 0x30;
    data /= 10;
  }
  
  /*to_show[1] = counter_of_hotp[3] + 0x30;
  to_show[2] = counter_of_hotp[4] + 0x30;
  to_show[3] = counter_of_hotp[5] + 0x30;
  to_show[4] = counter_of_hotp[6] + 0x30;
  to_show[5] = counter_of_hotp[7] + 0x30;
  to_show[6] = 0;
  to_show[7] = 0;*/
  
  LCD_GLASS_DisplayString( to_show );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Тут по черзі вибираємо часовий елемент і додаємо/віднімаємо до/від нього
// значення
void synchr_time_elems(void)
{
  // Завдяки неї повертаємося з меню нижнього рівня на верхній
  uint32_t return_cmd = FALSE;
  
  // Показати перший пункт
  LCD_show_and_modif_time_elem(SHOW_FIRST_ITEM, DO_NOTHING);
  
  // Поки не отримаємо команди виходу
  while (return_cmd == FALSE)
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    // Кнопка натиснута?
    switch (is_button_pressed)
    {
      // Натиснута
    case TRUE:
      // Скидуємо час бездіяльності
      Second_NOP_Downcounter = LCD_ON_NOP;
      // Скидуємо прапорець
      is_button_pressed = FALSE;
      
      // Коротке натискання?
      switch (is_short_pressed)
      {
        // Коротке натискання
      case TRUE:
        // Скидуємо прапорець
        is_short_pressed = FALSE;
        
        // Додавати чи віднімати?
        if ( (cmnds & SLCT_OPER) == PLUS)
        {
          LCD_show_and_modif_time_elem(SHOW_CURR_ITEM, ADD_ONE);
        }
        else if ( (cmnds & SLCT_OPER) == MINUS)
        {
          LCD_show_and_modif_time_elem(SHOW_CURR_ITEM, SUB_ONE);
        }
        
        break;
        
        // Довге натискання
      case FALSE:
        LCD_show_and_modif_time_elem(SHOW_NEXT_ITEM, DO_NOTHING);
        
        // Перевіряємо чи це не RETURN
        if ( (cmnds & SLCT_TIME_ITEM) > SECOND_ITEM)
        {
          return_cmd = TRUE;
          
          // Встановлюємо нові вибрані значення дня року і року
          set_yday_and_year();
          
          // Скидуємо вибір елементу часу
          cmnds &= ~SLCT_TIME_ITEM;
        }
        break;
        
      default:
        ;
        break;
      }
      
      break;
      // Не натиснута
    case FALSE:
      ;
      break;
      
    default:
      ;
      break;
    }
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Відображує поточний часовий елемент синхронізації і збільшує чи зменшує його
// значення на 1
void LCD_show_and_modif_time_elem(uint32_t i, uint32_t todo)
{
  // Містить значення, яке відображується на дисплеї
  uint8_t curr_cmd[8];
  
  curr_cmd[6] = 0;
  curr_cmd[7] = 0;
  
  if (i == SHOW_FIRST_ITEM)
  {
    // Прочитати значення року
    RTC_GetDate( RTC_Format_BIN, &RTC_DateStructure );
    RTC_GetTime( RTC_Format_BIN, &RTC_TimeStructure );
  }
  
  // Якщо показати наступний елемент
  if (i == SHOW_NEXT_ITEM)
  {
    cmnds += TIME_ELEM_ITEM;
  }
  
  // Якщо наступний пункт меню RETURN
  if ( (cmnds & SLCT_TIME_ITEM) > SECOND_ITEM )
  {
    // Встановлюємо виставлені значення
    RTC_SetTime( RTC_Format_BIN, &RTC_TimeStructure );  
    RTC_SetDate( RTC_Format_BIN, &RTC_DateStructure );
    
    return;
  }
  
  // Тут йде прив'язка до конкретних значень елементів, не через
  // RTC_Weekday_xx чи RTC_Month_xx
  switch (cmnds & SLCT_TIME_ITEM)
  {
  case YEAR_ITEM:
    if (todo == ADD_ONE)
    {
      RTC_DateStructure.RTC_Year = RTC_DateStructure.RTC_Year + 1;
      
      if ( RTC_DateStructure.RTC_Year > 99 )
      {
        RTC_DateStructure.RTC_Year = 99;
      }
    }
    else if (todo == SUB_ONE)
    {
      RTC_DateStructure.RTC_Year = RTC_DateStructure.RTC_Year - 1;
      
      if ( RTC_DateStructure.RTC_Year > 99 )
      {
        RTC_DateStructure.RTC_Year = 0;
      }
    }
    
    curr_cmd[0] = 'Y';
    curr_cmd[1] = 'E';
    curr_cmd[2] = 'A';
    curr_cmd[3] = 'R';
    curr_cmd[4] = RTC_DateStructure.RTC_Year / 10 + 0x30;
    curr_cmd[5] = (RTC_DateStructure.RTC_Year % 10) + 0x30;
    break;
    
  case DAY_ITEM:
    if (todo == ADD_ONE)
    {
      if ( IS_RTC_DATE( RTC_DateStructure.RTC_Date + 1 ) )
      {
        RTC_DateStructure.RTC_Date = RTC_DateStructure.RTC_Date + 1;
      }
    }
    else if (todo == SUB_ONE)
    {
      if ( IS_RTC_DATE( RTC_DateStructure.RTC_Date - 1 ) )
      {
        RTC_DateStructure.RTC_Date = RTC_DateStructure.RTC_Date - 1;
      }
    }
    
    curr_cmd[0] = 'D';
    curr_cmd[1] = 'A';
    curr_cmd[2] = 'Y';
    curr_cmd[3] = ' ';
    curr_cmd[4] = RTC_DateStructure.RTC_Date / 10 + 0x30;
    curr_cmd[5] = (RTC_DateStructure.RTC_Date % 10) + 0x30;
    break;
    
  case MONTH_ITEM:
    if (todo == ADD_ONE)
    {
      if ( IS_RTC_MONTH( RTC_DateStructure.RTC_Month + 1 ) )
      {
        RTC_DateStructure.RTC_Month = RTC_DateStructure.RTC_Month + 1;
      }
    }
    else if (todo == SUB_ONE)
    {
      if ( IS_RTC_MONTH( RTC_DateStructure.RTC_Month - 1 ) )
      {
        RTC_DateStructure.RTC_Month = RTC_DateStructure.RTC_Month - 1;
      }
    }
    
    curr_cmd[0] = 'M';
    curr_cmd[1] = 'O';
    curr_cmd[2] = 'N';
    curr_cmd[3] = ' ';
    curr_cmd[4] = RTC_DateStructure.RTC_Month / 10 + 0x30;
    curr_cmd[5] = (RTC_DateStructure.RTC_Month % 10) + 0x30;
    break;
    
  case WEEKDAY_ITEM:
    if (todo == ADD_ONE)
    {
      if ( IS_RTC_WEEKDAY( RTC_DateStructure.RTC_WeekDay + 1 ) )
      {
        RTC_DateStructure.RTC_WeekDay = RTC_DateStructure.RTC_WeekDay + 1;
      }
    }
    else if (todo == SUB_ONE)
    {
      if ( IS_RTC_WEEKDAY( RTC_DateStructure.RTC_WeekDay - 1 ) )
      {
        RTC_DateStructure.RTC_WeekDay = RTC_DateStructure.RTC_WeekDay - 1;
      }
    }
    
    curr_cmd[0] = 'W';
    curr_cmd[1] = 'D';
    curr_cmd[2] = 'A';
    curr_cmd[3] = 'Y';
    curr_cmd[4] = RTC_DateStructure.RTC_WeekDay / 10 + 0x30;
    curr_cmd[5] = (RTC_DateStructure.RTC_WeekDay % 10) + 0x30;
    break;
    
  case HOUR_ITEM:
    if (todo == ADD_ONE)
    {
      if ( IS_RTC_HOUR24( RTC_TimeStructure.RTC_Hours + 1 ) )
      {
        RTC_TimeStructure.RTC_Hours = RTC_TimeStructure.RTC_Hours + 1;
      }
    }
    else if (todo == SUB_ONE)
    {
      RTC_TimeStructure.RTC_Hours = RTC_TimeStructure.RTC_Hours - 1;
      
      if ( RTC_TimeStructure.RTC_Hours > 23)
      {
        RTC_TimeStructure.RTC_Hours = 0;
      }
    }
    
    curr_cmd[0] = 'H';
    curr_cmd[1] = 'O';
    curr_cmd[2] = 'U';
    curr_cmd[3] = 'R';
    curr_cmd[4] = RTC_TimeStructure.RTC_Hours / 10 + 0x30;
    curr_cmd[5] = (RTC_TimeStructure.RTC_Hours % 10) + 0x30;
    break;
    
  case MINUTE_ITEM:
    if (todo == ADD_ONE)
    {
      if ( IS_RTC_MINUTES( RTC_TimeStructure.RTC_Minutes + 1 ) )
      {
        RTC_TimeStructure.RTC_Minutes = RTC_TimeStructure.RTC_Minutes + 1;
      }
    }
    else if (todo == SUB_ONE)
    {
      RTC_TimeStructure.RTC_Minutes = RTC_TimeStructure.RTC_Minutes - 1;
      
      if ( RTC_TimeStructure.RTC_Minutes > 59 )
      {
        RTC_TimeStructure.RTC_Minutes = 0;
      }
    }
    
    curr_cmd[0] = 'M';
    curr_cmd[1] = 'I';
    curr_cmd[2] = 'N';
    curr_cmd[3] = ' ';
    curr_cmd[4] = RTC_TimeStructure.RTC_Minutes / 10 + 0x30;
    curr_cmd[5] = (RTC_TimeStructure.RTC_Minutes % 10) + 0x30;
    break;
    
  case SECOND_ITEM:
    if (todo == ADD_ONE)
    {
      if ( IS_RTC_SECONDS( RTC_TimeStructure.RTC_Seconds + 1 ) )
      {
        RTC_TimeStructure.RTC_Seconds = RTC_TimeStructure.RTC_Seconds + 1;
      }
    }
    else if (todo == SUB_ONE)
    {
      RTC_TimeStructure.RTC_Seconds = RTC_TimeStructure.RTC_Seconds - 1;
      
      if ( RTC_TimeStructure.RTC_Seconds > 59 )
      {
        RTC_TimeStructure.RTC_Seconds = 0;
      }
    }
    
    curr_cmd[0] = 'S';
    curr_cmd[1] = 'E';
    curr_cmd[2] = 'C';
    curr_cmd[3] = ' ';
    curr_cmd[4] = RTC_TimeStructure.RTC_Seconds / 10 + 0x30;
    curr_cmd[5] = (RTC_TimeStructure.RTC_Seconds % 10) + 0x30;
    break;
  }
  
  LCD_GLASS_DisplayString( curr_cmd );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Встановлює нові значення дня року і року після синхронізації час. елементів
void set_yday_and_year(void)
{
  uint32_t day = 0;
  
  RTC_GetDate( RTC_Format_BIN, &RTC_DateStructure );
  
  // Кількість днів до цього місяця
  if ( RTC_DateStructure.RTC_Month != 1 )
  {
    day += sum_of_days_in_month[ RTC_DateStructure.RTC_Month - 2];
  }
  
  // Кількість днів до сьогодні включно
  day += RTC_DateStructure.RTC_Date;
  
  // Якщо високосний рік
  if ( (RTC_DateStructure.RTC_Year % 4) == 0 )
  {
    day++;
  }
  
  // Оскільки від 0 до 365
  day--;
  
  // Встановлюємо обчислені значення
  Day_Of_Year_Counter = day;
  Current_Year_Copy = RTC_DateStructure.RTC_Year;
  
  // Зберігаємо в регістри
  RTC_WriteBackupRegister( RTC_BKP_DR0, Day_Of_Year_Counter );
  RTC_WriteBackupRegister( RTC_BKP_DR1, Current_Year_Copy );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Вмикає bars
void on_off_lcd_bars(uint32_t seconds)
{
  if (seconds >= 24)
  {
    BAR0_ON;
    BAR1_ON;
    BAR2_ON;
    BAR3_ON;
    LCD_GLASS_DisplayString( counter );
    return;
  }
  
  if ( (seconds <= 23) && (seconds >= 18) )
  {
    BAR0_ON;
    BAR1_ON;
    BAR2_ON;
    BAR3_OFF;
    LCD_GLASS_DisplayString( counter );
    return;
  }
  
  if ( (seconds <= 17) && (seconds >= 12) )
  {
    BAR0_ON;
    BAR1_ON;
    BAR2_OFF;
    BAR3_OFF;
    LCD_GLASS_DisplayString( counter );
    return;
  }
  
  if ( (seconds <= 11) && (seconds >= 6) )
  {
    BAR0_ON;
    BAR1_OFF;
    BAR2_OFF;
    BAR3_OFF;
    LCD_GLASS_DisplayString( counter );
    return;
  }
  
  if (seconds <= 5)
  {
    BAR0_OFF;
    BAR1_OFF;
    BAR2_OFF;
    BAR3_OFF;
    LCD_GLASS_DisplayString( counter );
    return;
  }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Тут очікуємо введення паролю, перевіряємо і видаємо TRUE або FALSE
uint32_t check_passwd(void)
{
  // Вказує чи пароль вірний
  uint32_t is_passwd_correct = TRUE;
  
  // Містить введений пароль
  uint8_t input_passwd[4] = {0, 0, 0, 0};
  
  // Містить індекс вводимої цифри
  uint32_t j = 2;
  
  // Індекс для циклу
  uint32_t i = 0;
  
  uint32_t userID = temp_preferences & USER_NUMBER;
  
  // Містить значення, яке відображується на дисплеї
  uint8_t curr_cmd[8];
  curr_cmd[0] = ' ';
  curr_cmd[1] = ' ';
  curr_cmd[2] = '0';
  curr_cmd[3] = '0';
  curr_cmd[4] = '0';
  curr_cmd[5] = '0';
  curr_cmd[6] = 0;
  curr_cmd[7] = 0;
  
  // Завдяки неї показуємо, що ввід закінчено
  uint32_t return_cmd = FALSE;
  
  // Показати пароль
  LCD_GLASS_DisplayString( curr_cmd );
  
  // Поки не отримаємо команди виходу
  while (return_cmd == FALSE)
  {
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
    
    // Кнопка натиснута?
    switch (is_button_pressed)
    {
      // Натиснута
    case TRUE:
      // Скидуємо час бездіяльності
      Second_NOP_Downcounter = LCD_ON_NOP;
      // Скидуємо прапорець
      is_button_pressed = FALSE;
      
      // Коротке натискання?
      switch (is_short_pressed)
      {
        // Коротке натискання
      case TRUE:
        // Скидуємо прапорець
        is_short_pressed = FALSE;
        // Змінюємо цифру
        curr_cmd[j] = ((curr_cmd[j] - 0x30 + 1) % 10) + 0x30;
        // Показати пароль
        LCD_GLASS_DisplayString( curr_cmd );
        break;
        
        // Довге натискання
      case FALSE:
        j++;
        // Якщо ввели всі цифри, то можна виходити з циклу
        if (j == 6)
        {
          return_cmd = TRUE;
        }
        break;
        
      default:
        ;
        break;
      }
      break;
      // Кнопка не натиснута
    case FALSE:
      ;
      break;
      
    default:
      ;
      break;
    }
  }
  
  // Зчитуємо пароль
  input_passwd[0] = curr_cmd[2] - 0x30;
  input_passwd[1] = curr_cmd[3] - 0x30;
  input_passwd[2] = curr_cmd[4] - 0x30;
  input_passwd[3] = curr_cmd[5] - 0x30;
  
  // Перевіряємо чи вірний
  do
  {
    if (user_passwd[userID][i] != input_passwd[i])
    {
      is_passwd_correct = FALSE;
    }
    
    i++;
  }
  while ( (is_passwd_correct == TRUE) && (i < 4) );
  
  if (is_passwd_correct == FALSE)
  {
    curr_cmd[0] = 'E';
    curr_cmd[1] = 'R';
    curr_cmd[2] = 'R';
    curr_cmd[3] = 'O';
    curr_cmd[4] = 'R';
    curr_cmd[5] = ' ';
    
    LCD_GLASS_DisplayString( curr_cmd );
    return_cmd = FALSE;
    // Поки не отримаємо команди виходу
    while (return_cmd == FALSE)
    {
      PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );
      
      // Кнопка натиснута?
      switch (is_button_pressed)
      {
        // Натиснута
      case TRUE:
        // Скидуємо час бездіяльності
        Second_NOP_Downcounter = LCD_ON_NOP;
        // Скидуємо прапорець
        is_button_pressed = FALSE;
        return is_passwd_correct;
        break;
        
      case FALSE:
        ;
        break;
        
      default:
        ;
        break;
      }
    }
  }
  
  return is_passwd_correct;
}
////////////////////////////////////////////////////////////////////////////////
