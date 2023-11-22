#ifndef KEY_INPUT_H
#define KEY_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_gpio.h"
#include <stdbool.h>

/* 针对不同硬件平台 */
#define KEY_STATE en_pin_state_t

/* 扫描周期 */
#define KEYSACN_TIMEBASE 10 
#define DESHAKE_SLICE 1
#define SHORT_PRESS_PERIOD 3
#define LONG_PRESS_PERIOD 25

/* 硬件电平 */
#define KEY_ON 0
#define KEY_OFF 1

/* 逻辑控制/模拟量调节 */
#define DIG 0
#define ANA 1

typedef enum {
    KEY_NONE 	= 0,
    KEY_SHORT 	= 2,
    KEY_LONG 	= 3,
	KEY_DOUBLE	= 5,
}key_val_t;

typedef void (*key_static_handler)(key_val_t);

typedef enum
{
  KEY_UNPRESSED    	= 1,		/* 按键未按下 */
  KEY_PROB_PRESSED 	= 0,		/* 短时按下一次 */
  KEY_PRESSED 		= 2,		/* 短按成功 */
  KEY_LONGPRESSED 	= 3,		/* 长按成功 */
  KEY_PROB_DOUBLECLICK = 4,		/* 短时按下两次 */
  KEY_DOUBELCLICK 	= 5,		/* 双击成功 */
}key_state_t;

typedef struct stKey_event
{
	uint32_t prio;
	key_val_t key_val;
	struct stKey_event *evt_next;
}key_event_t;

typedef struct stKey_dev
{
	io_HandlerType key_io;		/* io底层操作（读写等） */

	bool ctrDorA;				/* 模拟量控制 */
	char shortPressCnt;			/* 短时间内短按计数 */
	unsigned int hold_tick;		/* 按键按下持续时间 */
	unsigned int timeout_tick;	/* 各状态超时检测 */

	key_state_t key_state;		/* 按键瞬时状态 */
	key_static_handler static_hand;
	struct stKey_dev *dev_next;
	key_event_t *evt_Index;
}key_dev_t;

typedef struct key_operations_struct
{
	void (* init)(key_dev_t *,key_static_handler);
	void (* scan)(void);
	void (* indiv_handler)(key_dev_t *);
	void (* glob_handler)(void);
	void (* upload)(key_dev_t *);
}key_ops_t;

extern key_dev_t key1,key2,key3,key4,key5,key6;//.......key_n
extern key_ops_t key_ops;

/* 按键驱动框架基本数据结构如图，分为设备链表和事件链表
*|-------------			|-------------
*|key1        |			|key2    	 |
*|(key_dev_t) |-------> |(key_dev_t) |-------->.........
*|            |			|			 |
*|            |			|			 |
*--------------			--------------
		|                      |
		|					   |
		|					   |
	   \ /					  \	/
 |--------------        |--------------
 |key1_event1  |		|key2_event1  |
 |(key_event_t)|	    |(key_event_t)|
 |			   |	    |			  |
 |			   |	    |			  |
 ---------------		---------------
		|					   |
		|                      |
		|                      |
	   \ /                    \ /
 |--------------        |--------------
 |key1_event2  |		|key2_event2  |
 |(key_event_t)|	    |(key_event_t)|
 |			   |	    |			  |
 |			   |	    |			  |
 ---------------		---------------
 		|					   |
		|                      |
		|                      |
	   \ /                    \ /
		.                      .
		.                      .
		.                      .
		.                      .
		.                      .
		.                      .			
 按键驱动框架基本数据结构如图 */

#ifdef __cplusplus
}
#endif
#endif

