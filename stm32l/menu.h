// Виділяє номер користувача
#define USER_NUMBER             ( 0x000000FF )

// Виділяє алгоритм
#define ALGORITHM_TYPE          ( 0x000F0000 )

// Виділяє пункт меню третього рівня
#define SLCT_MENU_LVL_3         ( 0x00F00000 )

// Виділяє значення помилки
#define ERRO_NUMBER             ( 0x000000FF )

// Виділяє, яке значення використовувати для таймера, щоб піти у standby
#define SLCT_TIMER              ( 0x0F000000 )

// Виділяє операції додавання, віднімання
#define SLCT_OPER               ( 0x30000000 )

// Алгоритми
#define TOTP_ALGORITHM          ( 0x00000000 )
#define HOTP_ALGORITHM          ( 0x00010000 )

// Значення повернення на рівні меню алгоритмів
#define RETURN_OF_ALGORITHMS_MENU               ( 0x00020000 )

// Значення повернення на рівні меню третього рівня
#define RETURN_OF_MENU_LVL_3                    ( 0x00200000 )

//
#define RETURN_OF_SYNCHR_MENU                   ( 0x20000000 )

// Значення пункту меню на і-му рівні
#define MENU_LVL_2_ITEM         ( 0x00010000 )
#define MENU_LVL_3_ITEM         ( 0x00100000 )

// Значення пункту меню - вибору + чи -
#define SYNCHR_MENU_ITEM        ( 0x10000000 )

// Значення дій
#define PLUS                    ( 0x00000000 )
#define MINUS                   ( 0x10000000 )

// Значення пункту меню - генерації пароля
#define GENPWD                  ( 0x00000000 )

// Значення пункту меню - меню синхронізації
#define SYNCHR                  ( 0x00100000 )

// Значення таймерів для виключення мікроконтролера
#define SET_LCD_ON_TIME         ( 0x01000000 )
#define SET_LCD_ON_NOP          ( 0x02000000 )

// Значення чверті часу існування TOTP (для відображення bars)
#define QUARTER_STEP_TOTP       ( 6 )

// Показати наступний пункт меню
#define SHOW_NEXT_ITEM          ( 0x00000001 )

// Показати поточний пункт меню
#define SHOW_CURR_ITEM          ( 0x00000000 )

// Показати перший пункт меню
#define SHOW_FIRST_ITEM         ( 0x00000002 )

// Максимальна кількість користувачів (може бути від 0 до 99 (1 -- 100)
#define MAX_USERS               ( 5 )

////////////////////////////////////////////////////////////////////////////////
// Значення для меню синхронізації за часом ////////////////////////////////////

// Вибір елемента синхронізації за часом
#define SLCT_TIME_ITEM          ( 0x0000000F )

// Значення пункту меню синхронізації елемента за часом
#define TIME_ELEM_ITEM          ( 0x00000001 )

// Значення пунктів меню
#define YEAR_ITEM               ( 0x00000000 )
#define MONTH_ITEM              ( 0x00000001 )
#define DAY_ITEM                ( 0x00000002 )
#define WEEKDAY_ITEM            ( 0x00000003 )
#define HOUR_ITEM               ( 0x00000004 )
#define MINUTE_ITEM             ( 0x00000005 )
// Використовується також для RETURN, бо останнє значення
#define SECOND_ITEM             ( 0x00000006 )

// Дії при синхронізації часового елемента
#define DO_NOTHING              ( 0x00000000 )
#define ADD_ONE                 ( 0x00000001 )
#define SUB_ONE                 ( 0x00000002 )
////////////////////////////////////////////////////////////////////////////////

// Логічні значення
//#define TRUE                    ( 1 )
//#define FALSE                   ( 0 )

// Кількість секунд до вимкнення через бездіяльність
#define LCD_ON_NOP              ( 60 )

// Максимальна кількість секунд, при якій потрібно ще раз згенерувати TOTP
#define SECONDS_FOR_SECOND_TIME ( 5 )

void menu_lvl_1(void);
void menu_lvl_2(void);
void menu_lvl_3(void);
void menu_synchr(void);
void LCD_show_user_or_exit(uint32_t i);
void LCD_show_algorithm_or_return(uint32_t i);
void LCD_show_menu_item_lvl_3(uint32_t i);
void LCD_show_synchr_menu_item(uint32_t i);
void gen_password(void);
void add_to_synchr_elem(void);
void sub_from_synchr_elem(void);
void LCD_show_counter_of_hotp(void);
void synchr_elem(void);
void synchr_time_elems(void);
void LCD_show_and_modif_time_elem(uint32_t i, uint32_t todo);
void set_yday_and_year(void);
void on_off_lcd_bars(uint32_t seconds);
uint32_t check_passwd(void);
