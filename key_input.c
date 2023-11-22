/******************************************************************************************
* @file         : key_input.c
* @Description  : A key input driver framework
* @autor        : Jinyicheng
* @emil:        : 2907487307@qq.com
* @version      : 1.0
* @date         : 2023/08/31    
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 * ******************************************************************************************/
#include "../include/key_input.h"
#include <string.h>
#include <stdlib.h>

key_dev_t key0 = {
	.key_io.io_obj = {
						.IO_PortSel = PortD,	//用户可配置
						.IO_PinSel = Pin03,	//用户可配置
					},
};
key_dev_t key1 = {
	.key_io.io_obj = {	
						.IO_PortSel = PortD,	//用户可配置
						.IO_PinSel = Pin04,	//用户可配置
					},
};
key_dev_t key2 = {
	.key_io.io_obj = {
						.IO_PortSel = PortD,	//用户可配置
						.IO_PinSel = Pin05,	//用户可配置
					},
};
key_dev_t key3 = {
	.key_io.io_obj = {	
						.IO_PortSel = PortD,	//用户可配置
						.IO_PinSel = Pin06,	//用户可配置
					},
};
/* keyinput总数 */
static uint32_t pressed_cnt = 1;

/* 头节点 */
static key_dev_t * key_cbhead = NULL;

/**********************************************************************
 * 函数名称： key_stc_Init
 * 功能描述： 初始化key_dev
 * 输入参数： key_dev
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static void key_stc_Init(key_dev_t *key_dev)
{
	key_dev->dev_next = NULL;
	key_dev->evt_Index = NULL;
	key_dev->key_io.InitHandler = xs_GpioInit;
	key_dev->key_io.GetbitHandler = xs_GpioGetBit;
	key_dev->key_io.InitHandler(&key_dev->key_io);
	key_dev->key_state = KEY_UNPRESSED;
}

/**********************************************************************
 * 函数名称： key_Init
 * 功能描述： 注册按键
 * 输入参数： key_dev,key_handler
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void key_Init(key_dev_t *key_dev, key_static_handler key_handler)
{
	if(NULL == key_cbhead)
	{
		key_cbhead = key_dev;
		key_dev->static_hand = key_handler;
		key_stc_Init(key_dev);
		return;
	}
	
	key_dev_t *p_temp = key_cbhead;
	key_dev_t *p_Index = p_temp;
	/* 遍历链表，找到尾部节点 */
	for(;NULL != p_Index;p_Index = p_temp->dev_next)
	{
		p_temp = p_Index;
	}
	/* 在尾部插入key设备节点 */
	p_temp->dev_next = key_dev;
	
	key_dev->static_hand = key_handler;
	key_stc_Init(key_dev);
}

/**********************************************************************
 * 函数名称： key_getvalue
 * 功能描述： 读取输入电平
 * 输入参数： key_dev
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static KEY_STATE key_getvalue(key_dev_t *key_dev)
{
	KEY_STATE val;
	
	/* 读取按键输入电平 */
	val = key_dev->key_io.GetbitHandler(&key_dev->key_io);
	
	return val;
}

/**********************************************************************
 * 函数名称： key_getvalue
 * 功能描述： 以链表形式记录键值
 * 输入参数： key_dev，key_val
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static void key_evt_record(key_dev_t *key_dev,key_val_t key_val)
{
	/* 若没有事件列表位空，则插入第一个 */
	if(NULL == key_dev->evt_Index)
	{
		key_event_t * key_evthead = (key_event_t *)tAllocHeapforeach(sizeof(key_event_t));
		if(NULL == key_evthead)
		{
			return;
		}
		key_evthead->key_val = key_val;
		key_evthead->evt_next = NULL;
		pressed_cnt++;
		key_evthead->prio = pressed_cnt;
		key_dev->evt_Index = key_evthead;
		return;
	}

	/* 在事件列表尾部添加事件，由于是单向链表所以时间复杂度较高 */
	key_event_t *key_evt = (key_event_t *)tAllocHeapforeach(sizeof(key_event_t));
	if(NULL == key_evt)
	{
		return;
	}
	key_event_t *p_temp = key_cbhead->evt_Index;
	key_event_t *p_Index = p_temp;
	for(;NULL != p_Index;p_Index = p_temp->evt_next)
	{
		p_temp = p_Index;
	}
	p_temp->evt_next = key_evt;
	p_temp->evt_next->key_val = key_val;
	p_temp->evt_next->evt_next = NULL;
	pressed_cnt++;
	p_temp->evt_next->prio = pressed_cnt;
}

/**********************************************************************
 * 函数名称： key_scan
 * 功能描述： 周期扫描按键键值
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void key_scan(void)
{
	key_dev_t *p_temp = key_cbhead;
	key_dev_t *p_Index = p_temp;
	KEY_STATE key_instState;
	
	for(;NULL != p_Index;p_Index = p_temp->dev_next)
	{
		p_temp = p_Index;

		/* 读取IO瞬时电平 */
		key_instState = key_getvalue(p_Index);
		
		/* 若瞬时电平等于按下电平，按下持续时间+1周期 */
		if(KEY_ON == key_instState)
			p_Index->hold_tick += KEYSACN_TIMEBASE;

		switch(p_Index->key_state)
		{
			case KEY_UNPRESSED: 
				if(KEY_UNPRESSED == key_instState)
				{
					p_Index->key_state = KEY_UNPRESSED;
					continue;               
				}
				else if(key_instState == KEY_PROB_PRESSED)
				{
					p_Index->key_state = KEY_PROB_PRESSED;
				}
				break; 
			case KEY_PROB_PRESSED:
				if(p_Index->shortPressCnt == 0)
				{
					if(KEY_UNPRESSED == key_instState)
					{
						p_Index->key_state = KEY_UNPRESSED;
						continue;                           
					}
					else
					{
						if(p_Index->hold_tick >= DESHAKE_SLICE * KEYSACN_TIMEBASE + SHORT_PRESS_PERIOD * KEYSACN_TIMEBASE)
						{
							p_Index->key_state = KEY_PRESSED;
						}
					}
				}
				else
				{
					if(KEY_UNPRESSED == key_instState)
					{
						p_Index->key_state = KEY_PRESSED;
						key_evt_record(p_Index,KEY_PRESSED);
						continue;                           
					}
					else
					{
						if(p_Index->hold_tick >= DESHAKE_SLICE * KEYSACN_TIMEBASE + SHORT_PRESS_PERIOD * KEYSACN_TIMEBASE)
						{
							/* 双击成功，老铁666！ */
							p_Index->key_state = KEY_DOUBELCLICK;
							p_Index->hold_tick = 0;
							p_Index->shortPressCnt = 0;
							key_evt_record(p_Index,KEY_DOUBELCLICK);
							continue;
						}
					}
				}
				break;
			case KEY_PRESSED:
				if(KEY_UNPRESSED == key_instState)
				{
					p_Index->key_state = KEY_PROB_DOUBLECLICK;
					p_Index->hold_tick = 0;
					p_Index->timeout_tick = 0;
				}
				else if(p_Index->hold_tick >= LONG_PRESS_PERIOD * KEYSACN_TIMEBASE)
				{
					p_Index->key_state = KEY_LONGPRESSED;
				}
				break;
			case KEY_PROB_DOUBLECLICK:
				if(KEY_UNPRESSED == key_instState)
				{
					/* 未按下，则超时时间累加 */
					p_Index->timeout_tick += KEYSACN_TIMEBASE;

					/* 双击失败，老铁不给力呀！ */
					if(p_Index->timeout_tick >= 200)
					{
						p_Index->key_state = KEY_UNPRESSED;
						p_Index->hold_tick = 0;
						p_Index->timeout_tick = 0;
						key_evt_record(p_Index,KEY_PRESSED);
						continue;
					}
				}
				else
				{
					p_Index->key_state = KEY_PROB_PRESSED;
					p_Index->shortPressCnt = 1;
				}
				break;
			case KEY_DOUBELCLICK:
				/* 什么都不做，等待松开 */
				if(KEY_UNPRESSED == key_instState)
				{
					p_Index->key_state = KEY_UNPRESSED;
				}
				break;
			case KEY_LONGPRESSED:
				/* 是否松开 */
				if(DIG == p_Index->ctrDorA)
				{
					if(KEY_UNPRESSED == key_instState)
					{
						p_Index->key_state = KEY_UNPRESSED;p_Index->hold_tick = 0;
						key_evt_record(p_Index,KEY_LONGPRESSED);
						continue;
					}
				}
				else if(ANA == p_Index->ctrDorA)
				{
					//模拟量累加，注意控制速度，这里是20ms累加1，可以将模拟量分辨率缩小至0.5或0.1再进行累加
					//模拟量累加code comes here
					/*......*/
					//模拟量累加code end here
					if(KEY_UNPRESSED == key_instState)
					{
						p_Index->key_state = KEY_UNPRESSED;
						p_Index->hold_tick = 0;
					}
				}
				break;
			default:
				break;
		}
	}
}

/**********************************************************************
 * 函数名称： key_handle_static
 * 功能描述： 固定逻辑控制
 * 输入参数： key_dev
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void key_handle_static(key_dev_t *key_dev)
{
	if(NULL == key_dev->static_hand)
		return;
	if(NULL == key_dev->evt_Index)
		return;

	/* 回调处理,优先处理第一个事件节点，输入参数click类型 */
	key_dev->static_hand(key_dev->evt_Index->key_val);
	
	unsigned char * p = (unsigned char *)key_dev->evt_Index;

	/* 删除第一个事件节点 */
	key_dev->evt_Index = key_dev->evt_Index->evt_next;

	/* 释放 */
	tFreeHeapforeach(p);
}

/**********************************************************************
 * 函数名称： key_handle_dynamic
 * 功能描述： 动态控制，如界面操作
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void key_handle_dynamic(void)
{
	if(NULL == key_cbhead)
		return;
	key_dev_t *key_index = key_cbhead;
	
	int min = key_index->evt_Index->prio;
	key_dev_t *key_to_handle = key_cbhead;
	
	for(int i = 0; key_index != NULL; i++)
	{
		if(i >= 1)
			key_index = key_index->dev_next;
		if(min > key_index->evt_Index->prio)
		{
			key_to_handle = key_index;
			min = key_index->evt_Index->prio;
		}
	}
	key_handle_static(key_to_handle);
}

/**********************************************************************
 * 函数名称： key_upload
 * 功能描述： 注销按键
 * 输入参数： key_dev
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void key_upload(key_dev_t *key_dev)
{
	if(NULL == key_dev)
		return;
	key_event_t * evt_to_del = key_dev->evt_Index;
	for(int i = 0;evt_to_del != NULL; i++)
	{
		if(i >= 1)
			evt_to_del = evt_to_del->evt_next;
		tFreeHeapforeach(evt_to_del);
	}
	key_dev_t * dev_to_del = key_dev;
	tFreeHeapforeach(dev_to_del);
}

/* key operations collection */
key_ops_t key_ops = {
	.init = key_Init,
	.scan = key_scan,
	.indiv_handler = key_handle_static,
	.glob_handler = key_handle_dynamic,
	.upload = key_upload
};