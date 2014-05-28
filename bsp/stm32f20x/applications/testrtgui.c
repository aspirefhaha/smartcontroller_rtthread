#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>	   
#include <rtgui/rtgui_system.h>	   
#include <rtgui/widgets/window.h>

#if 1
#include <rtgui/rtgui_app.h>
#include <rtgui/widgets/notebook.h>

struct rtgui_notebook *the_notebook;

static rt_bool_t demo_handle_key(struct rtgui_object *object, struct rtgui_event *event)
{
    struct rtgui_event_kbd *ekbd = (struct rtgui_event_kbd *)event;

    if (ekbd->type == RTGUI_KEYUP)
    {
        if (ekbd->key == RTGUIK_RIGHT)
        {
            demo_view_next(RT_NULL, RT_NULL);
            return RT_TRUE;
        }
        else if (ekbd->key == RTGUIK_LEFT)
        {
            demo_view_prev(RT_NULL, RT_NULL);
            return RT_TRUE;
        }
    }
    return RT_TRUE;
}

struct rtgui_win *main_win;
static void application_entry(void *parameter)
{
    struct rtgui_app *app;
    struct rtgui_rect rect;

    app = rtgui_app_create("gui_demo");
    if (app == RT_NULL)
        return;

    /* create a full screen window */
    rtgui_graphic_driver_get_rect(rtgui_graphic_driver_get_default(), &rect);

    main_win = rtgui_win_create(RT_NULL, "demo_win", &rect,
                                RTGUI_WIN_STYLE_NO_BORDER | RTGUI_WIN_STYLE_NO_TITLE);
    if (main_win == RT_NULL)
    {
        rtgui_app_destroy(app);
        return;
    }

    rtgui_win_set_onkey(main_win, demo_handle_key);

    /* create a no title notebook that we can switch demo on it easily. */
    //the_notebook = rtgui_notebook_create(&rect, RTGUI_NOTEBOOK_NOTAB);
    //if (the_notebook == RT_NULL)
    //{
    //    rtgui_win_destroy(main_win);
    //    rtgui_app_destroy(app);
    //    return;
    //}

    //rtgui_container_add_child(RTGUI_CONTAINER(main_win), RTGUI_WIDGET(the_notebook));
	/* 初始化各个例子的视图 */
    //demo_view_benchmark();
	//demo_view_window();
	//demo_view_label();
    //demo_view_button();
	rtgui_win_show(main_win, RT_FALSE);

    /* 执行工作台事件循环 */
    rtgui_app_run(app);

    rtgui_app_destroy(app);
}
void msg()
{
    static rt_bool_t inited = RT_FALSE;

    if (inited == RT_FALSE) /* 避免重复初始化而做的保护 */
    {
        rt_thread_t tid;

        tid = rt_thread_create("wb",
                               application_entry, RT_NULL,
                               2048 * 2, 25, 10);

        if (tid != RT_NULL)
            rt_thread_startup(tid);

        inited = RT_TRUE;
    }
}


#else
#include <rtgui/widgets/label.h> 

/* 因为要跨函数访问，所以把下面这些作为全局变量 */
 static struct rtgui_timer *timer;
 static struct rtgui_label* label;
 static struct rtgui_win* msgbox;
 static char label_text[256]={0};
 static int cnt = 5;
 
/* 显示对话框后的定时器超时回调函数，和RT-Thread中的timer不相同，这个超时函数的执行上下文是显示对话框线程的上下文 */
 void diag_close(struct rtgui_timer* timer, void* parameter)
 {
         rt_sprintf(label_text, "closed then %d second!", cnt);
         
        /* 重新设置标签文本 */
         rtgui_label_set_text(label, label_text);
         /* 调用update函数更新显示 */
         rtgui_widget_update(RTGUI_WIDGET(label));
         if (cnt == 0)
         {
                 /* 当cnt减到零时，删除对话框 */
                 rtgui_win_destroy(msgbox);
                 rtgui_timer_stop(timer);
                 /* 删除定时器 */
                 rtgui_timer_destory(timer);
         }
 
        cnt --;
 }
 
void msg()
 {
         rt_mq_t mq;
         rt_thread_t tid;
         rt_uint32_t user_data;
         struct rtgui_rect rect = {50, 50, 200, 200};
 
        tid = rt_thread_self();
         if (tid == RT_NULL) return; /* can't use in none-scheduler environement */
         user_data = tid->user_data;
 
        /* create gui message queue */
         mq = rt_mq_create("msgbox", 256, 4, RT_IPC_FLAG_FIFO);
         /* register message queue on current thread */
         rtgui_thread_register(rt_thread_self(), mq);
 
        msgbox = rtgui_win_create(RT_NULL, "Information", &rect, RTGUI_WIN_STYLE_DEFAULT);
         if (msgbox != RT_NULL)
         {
                 struct rtgui_box* box = rtgui_box_create(RTGUI_VERTICAL, RT_NULL);
 
                cnt = 5;
                 rt_sprintf(label_text, "closed then %d second!", cnt);
                 label = rtgui_label_create(label_text);
 
                rtgui_win_set_box(msgbox, box);
                 RTGUI_WIDGET(label)->align = RTGUI_ALIGN_CENTER_HORIZONTAL |
                         RTGUI_ALIGN_CENTER_VERTICAL;
                 rtgui_widget_set_miniwidth(RTGUI_WIDGET(label),130);
                 rtgui_box_append(box, RTGUI_WIDGET(label));
                 rtgui_box_layout(box);
 
                rtgui_win_show(msgbox);
         }
 
        /* 多了个定时创建的动作，超时时执行的是diag_close函数 */
         timer = rtgui_timer_create(200, RT_TIMER_FLAG_PERIODIC,
                 diag_close, RT_NULL);
         rtgui_timer_start(timer);
 
        rtgui_win_event_loop(msgbox);
 
        rtgui_thread_deregister(rt_thread_self());
         /* remove RTGUI message queue */
         rt_mq_delete(mq);
 
        /* recover user data */
         tid->user_data = user_data;
 }
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(msg, msg on gui) 

#endif

#endif
