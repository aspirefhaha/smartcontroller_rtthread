#include <stdio.h>
#include <rtthread.h>

#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>	   
#include <rtgui/rtgui_system.h>	   
#include <rtgui/widgets/window.h>
#include <rtgui/rtgui_app.h>
#include <rtgui/widgets/notebook.h>	
#include <rtgui/widgets/label.h>
#include <rtgui/widgets/button.h>
#include <rtgui/widgets/staticline.h>
#include <rtgui/widgets/box.h>
#include "gui_example/block_panel.h"
#include "main_win.h"
#include "config_init.h"
#include <finsh.h>

#define MAINTITLE	"��ӭʹ��XX��Ϣ�ɼ�ϵͳ"
#define OPERATETITLE	"�ڵ��������"
#define AD0MODULE		"���ģ�ģ�飰��"
#define AD1MODULE		"���ģ�ģ�飱��"
#define AD2MODULE		"���ģ�ģ�飲��"
#define DA0MODULE		"�ģ��ö���ģ�飰"
#define DA1MODULE		"�ģ��ö���ģ�飱"
#define VMARGIN RTGUI_BORDER_DEFAULT_WIDTH
#define HMARGIN	RTGUI_BORDER_DEFAULT_WIDTH
#define TOP_HEIGHT RTGUI_DEFAULT_FONT_SIZE + 2 * VMARGIN
#define BOTTOM_HEIGHT (RTGUI_WIDGET_DEFAULT_MARGIN * 2 + RTGUI_BORDER_DEFAULT_WIDTH * 2 + RTGUI_DEFAULT_FONT_SIZE * 3 / 2)
#define MBUTTON_HEIGHT	(RTGUI_WIDGET_DEFAULT_MARGIN   + RTGUI_BORDER_DEFAULT_WIDTH * 2 + RTGUI_DEFAULT_FONT_SIZE )
#define BUTTON_MARGIN	6


struct rtgui_win *main_win;
struct rtgui_notebook * main_notebook;
struct rtgui_notebook * nodebook = RT_NULL;
struct rtgui_label      *ope_label = RT_NULL;
struct rtgui_label 		*adc0_value = RT_NULL;
struct rtgui_label 		*adc1_value = RT_NULL;
struct rtgui_label 		*adc2_value = RT_NULL;
struct rtgui_label 		*dac0_value = RT_NULL;
struct rtgui_label 		*dac1_value	= RT_NULL;

int dac0val = 0 ;
int dac1val = 0;
m_node_t pCurNode = RT_NULL;

void main_view_prev(struct rtgui_object *object, struct rtgui_event *event)
{
    rt_int16_t cur_idx = rtgui_notebook_get_current_index(nodebook);
	//printf("curindex %d,totoal %d\n",cur_idx,rtgui_notebook_get_count(nodebook));

    if (cur_idx == 0)
        rtgui_notebook_set_current_by_index(nodebook,
                                            rtgui_notebook_get_count(nodebook) - 1);
    else
        rtgui_notebook_set_current_by_index(nodebook,
                                            --cur_idx);
}

long adc0set(int value)
{
	char strval[12]={0};
	sprintf(strval," %03d ",value);
	rtgui_label_set_text(adc0_value,strval);
	return 0;
}

long adc1set(int value)
{
	char strval[12]={0};
	sprintf(strval," %03d ",value);
	rtgui_label_set_text(adc1_value,strval);
	return 0;
}
long adc2set(int value)
{
	char strval[12]={0};
	sprintf(strval," %03d ",value);
	rtgui_label_set_text(adc2_value,strval);
	return 0;
}

#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(adc0set,set adc0 module value);
FINSH_FUNCTION_EXPORT(adc1set,set adc1 module value);
FINSH_FUNCTION_EXPORT(adc2set,set adc2 module value);
#endif

void opera_0_plus(struct rtgui_object* object,struct rtgui_event * event)
{
	char strval[12]={0};

 	dac0val++;
	if(dac0val>255)
		dac0val = 0;
	sprintf(strval," %03d ",dac0val);
	rtgui_label_set_text(dac0_value,strval);
}
void opera_1_plus(struct rtgui_object* object,struct rtgui_event * event)
{
	char strval[12]={0};
 	dac1val++;
	if(dac1val>255)
		dac1val = 0;
	sprintf(strval," %03d ",dac1val);
	rtgui_label_set_text(dac1_value,strval);
}
void opera_0_minus(struct rtgui_object* object,struct rtgui_event * event)
{
	char strval[12]={0};
 	dac0val--;
	if(dac0val<0)
		dac0val = 255;
	sprintf(strval," %03d ",dac0val);
	rtgui_label_set_text(dac0_value,strval);
}
void opera_1_minus(struct rtgui_object* object,struct rtgui_event * event)
{
	char strval[12]={0};
 	dac1val--; 
	if(dac1val<0)
		dac1val = 255;
	sprintf(strval," %03d ",dac1val);
	rtgui_label_set_text(dac1_value,strval);
} 

void opera_view_back(struct rtgui_object * object,struct rtgui_event * event)
{
   rtgui_notebook_set_current_by_index(main_notebook,0);
}

void opera_view_1(struct rtgui_object * object,struct rtgui_event * event)
{
 	printf("operator 1\n");
}

void opera_view_2(struct rtgui_object * object,struct rtgui_event * event)
{
	printf("operator 2\n");
}

void main_view_next(struct rtgui_object *object, struct rtgui_event *event)
{
    rt_int16_t cur_idx = rtgui_notebook_get_current_index(nodebook);
	//printf("curindex %d,total %d\n",cur_idx,rtgui_notebook_get_count(nodebook));

    if (cur_idx == rtgui_notebook_get_count(nodebook) - 1)
        rtgui_notebook_set_current_by_index(nodebook,
                                            0);
    else
        rtgui_notebook_set_current_by_index(nodebook,
                                            ++cur_idx);
}

void node_click_handle(struct rtgui_object * object,struct rtgui_event * event)
{
	rtgui_button_t 	* cb = RTGUI_BUTTON(object);
	char opetext[64]={0};
	m_node_t pnode = (m_node_t)rtgui_button_get_userdata(cb); 
	RT_ASSERT(pnode);
	pCurNode = pnode;
	printf("click node ip %d\n",pnode->ip);
	if(ope_label){
		sprintf(opetext, "�ڵ�%02d��������  ",pnode->ip);
		rtgui_label_set_text(ope_label,opetext);
	}
	rtgui_notebook_set_current_by_index(main_notebook,1);
}

/* ����������ڷ�����ʾ��ͼ�Ķ���������� */
static void view_get_rect(rtgui_container_t *container, rtgui_rect_t *rect)
{
    RT_ASSERT(container != RT_NULL);
    RT_ASSERT(rect != RT_NULL);

    rtgui_widget_get_rect(RTGUI_WIDGET(container), rect);
    rtgui_widget_rect_to_device(RTGUI_WIDGET(container), rect);
    /* ȥ����ʾ������·���ť������ */
    //rect->y1 += 45;
    //rect->y2 -= 35;
}

static void client_get_rect(rtgui_container_t *container, rtgui_rect_t *rect)
{
    view_get_rect(container, rect);
    /* ȥ����ʾ������·���ť������ */
    rect->y1 += RTGUI_DEFAULT_FONT_SIZE + VMARGIN * 8;
    rect->y2 -= BOTTOM_HEIGHT + VMARGIN ;
}

static void bottom_get_rect(rtgui_container_t *container, rtgui_rect_t *rect)
{

    view_get_rect(container, rect);
    /* ȥ����ʾ������·���ť������ */
    rect->y1 = rect->y2 - BOTTOM_HEIGHT;
}

void arrange_adda_views(rtgui_container_t * container)
{
	rtgui_rect_t rect,fontrect;	
	rtgui_button_t * plusbtn,*minusbtn ;
	rtgui_label_t * label; 
	rtgui_panel_t	* dacpanel;
	int aheight ,dheight ,theight,mwidth;

	client_get_rect(container,&rect); 
	//total height
	theight = rect.y2 - rect.y1;
	//ad height
	aheight = (theight - 6 * VMARGIN - 2 * BUTTON_MARGIN) / 5 ;
	dheight = (theight - 6 * VMARGIN - 1 * BUTTON_MARGIN) / 3 ;
	//mnode width
	mwidth = (rect.x2 - rect.x1 - 3 * HMARGIN - BUTTON_MARGIN)/2;

	//ADC0
	client_get_rect(container,&rect);
	rect.x1 += HMARGIN;
	rect.y1 += 4 * VMARGIN;
	rtgui_font_get_metrics(rtgui_font_default(), AD0MODULE, &fontrect);
	rect.x2 = rect.x1 + (fontrect.x2 - fontrect.x1);
	rect.y2 = rect.y1 + (fontrect.y2 - fontrect.y1) + VMARGIN ;

	label = rtgui_label_create(AD0MODULE);
	rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(label));
	adc0_value = rtgui_label_create("xxxxxxx");
	rect.x1 = rect.x2 ;
	rect.x2 = rect.x1 + RTGUI_DEFAULT_FONT_SIZE * 7  ;
	rtgui_widget_set_rect(RTGUI_WIDGET(adc0_value),&rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(adc0_value));

	//ADC1
	client_get_rect(container,&rect); 
	rect.x1 += HMARGIN;
	rect.y1 += 4 * VMARGIN + aheight + VMARGIN;
	rtgui_font_get_metrics(rtgui_font_default(), AD1MODULE, &fontrect);
	rect.x2 = rect.x1 + (fontrect.x2 - fontrect.x1);
	rect.y2 = rect.y1 + (fontrect.y2 - fontrect.y1) + VMARGIN ;

	label = rtgui_label_create(AD1MODULE);
	rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(label));
	adc1_value = rtgui_label_create("xxxxxxx");
	rect.x1 = rect.x2 ;
	rect.x2 = rect.x1 + RTGUI_DEFAULT_FONT_SIZE * 7  ;
	rtgui_widget_set_rect(RTGUI_WIDGET(adc1_value),&rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(adc1_value));

	//ADC2
	client_get_rect(container,&rect); 
	rect.x1 += HMARGIN;
	rect.y1 += 4 * VMARGIN + 2*aheight + 2*VMARGIN;
	rtgui_font_get_metrics(rtgui_font_default(), AD2MODULE, &fontrect);
	rect.x2 = rect.x1 + (fontrect.x2 - fontrect.x1);
	rect.y2 = rect.y1 + (fontrect.y2 - fontrect.y1) + VMARGIN ;

	label = rtgui_label_create(AD2MODULE);
	rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(label));
	adc2_value = rtgui_label_create("xxxxxxx");
	rect.x1 = rect.x2 ;
	rect.x2 = rect.x1 + RTGUI_DEFAULT_FONT_SIZE * 7  ;
	rtgui_widget_set_rect(RTGUI_WIDGET(adc2_value),&rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(adc2_value));

	//DAC0
	client_get_rect(container,&rect);
	rect.x1 += 5 * HMARGIN ;
	rect.y1 += 8 * VMARGIN + 3 * aheight ;
	rtgui_font_get_metrics(rtgui_font_default(), DA0MODULE, &fontrect);
	rect.x2 = rect.x1 + (fontrect.x2 - fontrect.x1) + 2 * HMARGIN;
	rect.y2 = rect.y1 + dheight;

	dacpanel = rtgui_panel_create(RTGUI_BORDER_STATIC);
	rtgui_widget_set_rect(RTGUI_WIDGET(dacpanel),&rect);

	label = rtgui_label_create(DA0MODULE);
	rtgui_container_add_child(container,RTGUI_WIDGET(dacpanel));
	rect.x1 += HMARGIN;
	rect.x2 -= HMARGIN;
	rect.y1 += 2*VMARGIN;
	rect.y2 =  rect.y1 + fontrect.y2 - fontrect.y1;
	rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect);
	rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(label));

	{
		int oldright=rect.x2;
		//plus button
		plusbtn = rtgui_button_create("��");
		rect.x2 = rect.x1 + 2 * BUTTON_MARGIN +  RTGUI_DEFAULT_FONT_SIZE;
		rect.y1 = rect.y2 + 2 * VMARGIN;
		rect.y2 += MBUTTON_HEIGHT;
		rtgui_widget_set_rect(RTGUI_WIDGET(plusbtn),&rect);
		rtgui_button_set_onbutton(plusbtn,opera_0_plus);
		rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(plusbtn));
	
		//dac0 value
		dac0_value = rtgui_label_create(" 000 ");
		rect.x1 = rect.x2 + 5 * HMARGIN;
		rect.x2 = oldright - (2 * BUTTON_MARGIN +  RTGUI_DEFAULT_FONT_SIZE + VMARGIN);
		rtgui_widget_set_rect(RTGUI_WIDGET(dac0_value),&rect);
		rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(dac0_value));

		//minus button
		minusbtn = rtgui_button_create("��");
		rect.x2 = oldright;
		rect.x1 = rect.x2 -(2 * BUTTON_MARGIN +  RTGUI_DEFAULT_FONT_SIZE);
		rtgui_widget_set_rect(RTGUI_WIDGET(minusbtn),&rect);
		rtgui_button_set_onbutton(minusbtn,opera_0_minus);
		rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(minusbtn));
	}


	//DAC1
	client_get_rect(container,&rect);
	rect.x1 += mwidth + 5 * HMARGIN;
	rect.y1 += 8 * VMARGIN + 3*aheight ;
	rtgui_font_get_metrics(rtgui_font_default(), DA1MODULE, &fontrect);
	rect.x2 = rect.x1 + (fontrect.x2 - fontrect.x1) + 2 * HMARGIN;
	rect.y2 = rect.y1 + dheight;

	dacpanel = rtgui_panel_create(RTGUI_BORDER_STATIC);
	rtgui_widget_set_rect(RTGUI_WIDGET(dacpanel),&rect);
	
	label = rtgui_label_create(DA1MODULE);
	rtgui_container_add_child(container,RTGUI_WIDGET(dacpanel));
	rect.x1 += HMARGIN;
	rect.x2 -= HMARGIN;
	rect.y1 += 2 * VMARGIN;
	rect.y2 = fontrect.y2 - fontrect.y1 + rect.y1;
	rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect);
	rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(label));

	{
		int oldright=rect.x2;
		//plus button
		plusbtn = rtgui_button_create("��");
		rect.x2 = rect.x1 + 2 * BUTTON_MARGIN +  RTGUI_DEFAULT_FONT_SIZE;
		rect.y1 = rect.y2 + 2 * VMARGIN;
		rect.y2 += MBUTTON_HEIGHT;
		rtgui_widget_set_rect(RTGUI_WIDGET(plusbtn),&rect);
		rtgui_button_set_onbutton(plusbtn,opera_1_plus);
		rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(plusbtn));
	
		//dac1 value
		dac1_value = rtgui_label_create(" 000 ");
		rect.x1 = rect.x2 + 5 * HMARGIN;
		rect.x2 = oldright - (2 * BUTTON_MARGIN +  RTGUI_DEFAULT_FONT_SIZE + VMARGIN);
		rtgui_widget_set_rect(RTGUI_WIDGET(dac1_value),&rect);
		rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(dac1_value));

		//minus button
		minusbtn = rtgui_button_create("��");
		rect.x2 = oldright;
		rect.x1 = rect.x2 -(2 * BUTTON_MARGIN +  RTGUI_DEFAULT_FONT_SIZE);
		rtgui_widget_set_rect(RTGUI_WIDGET(minusbtn),&rect);
		rtgui_button_set_onbutton(minusbtn,opera_1_minus);
		rtgui_container_add_child(RTGUI_CONTAINER(dacpanel),RTGUI_WIDGET(minusbtn));
	}

} 

void operate_view(const char *title,struct rtgui_notebook * main_notebook)
{
	struct rtgui_container  *container;
    struct rtgui_staticline *line;
    struct rtgui_button     *next_btn, *prev_btn;
    struct rtgui_rect       rect,fontrect;
	int viewwidth = 0 , btnwidth = 0;
	container = rtgui_container_create();
	if( container == RT_NULL)
		return;

	rtgui_notebook_add(main_notebook, title, RTGUI_WIDGET(container));

    /* �����ͼ��λ����Ϣ(�ڼ��뵽 notebook ��ʱ��notebook ���Զ����� container
     * �Ĵ�С) */
    view_get_rect(container, &rect);
	viewwidth = rect.x2 - rect.x1;
	rtgui_font_get_metrics(rtgui_font_default(), title, &fontrect);
	btnwidth = (viewwidth - 2 * HMARGIN - 3 * (BUTTON_MARGIN+HMARGIN))/4;
	rect.x1 += (viewwidth-(fontrect.x2 - fontrect.x1)) /2;

    rect.y1 += VMARGIN * 3;
    rect.x2 = rect.x1 + fontrect.x2 - fontrect.x1;
    rect.y2 = rect.y1 + fontrect.y2 - fontrect.y1 + VMARGIN;

    /* ���������õı�ǩ */
    ope_label = rtgui_label_create(title);
    /* ���ñ�ǩλ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(ope_label), &rect);
    /* ��ӱ�ǩ����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(ope_label));
	
	client_get_rect(container, &rect);
	
    rect.y2 = rect.y1 + VMARGIN;
    /* ����һ��ˮƽ�� staticline �� */
    line = rtgui_staticline_create(RTGUI_HORIZONTAL);
    /* ���þ�̬�ߵ�λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(line), &rect);
    /* ��Ӿ�̬�ߵ���ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(line));
	/* ����һ���׶�ˮƽ�� staticline �� */
	client_get_rect(container,&rect);
	rect.y1 = rect.y2 - VMARGIN;
	line = rtgui_staticline_create(RTGUI_HORIZONTAL);
	rtgui_widget_set_rect(RTGUI_WIDGET(line),&rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(line)); 


    /* ��õ׶˵�λ����Ϣ */
   	bottom_get_rect(container, &rect);
	rect.y1 += RTGUI_WIDGET_DEFAULT_MARGIN;
	rect.x1 += HMARGIN;
    rect.x2 = rect.x1 + btnwidth;
    rect.y2 = rect.y1 + BOTTOM_HEIGHT - 2 * RTGUI_WIDGET_DEFAULT_MARGIN ;
    
    /* ����"����"��ť */
    next_btn = rtgui_button_create("����");
    /* ����onbutton������demo_view_next���� */
    rtgui_button_set_onbutton(next_btn, opera_view_back);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(next_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(next_btn));

    /* �����ͼ��λ����Ϣ */
    rect.x1 += (btnwidth + HMARGIN + BUTTON_MARGIN)*2;
    rect.x2 = rect.x1 + btnwidth;
    /* ����"����"��ť */
    prev_btn = rtgui_button_create("������");
    /* ����onbutton������demo_view_prev���� */
    rtgui_button_set_onbutton(prev_btn, opera_view_1);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(prev_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(prev_btn));

	rect.x1 += btnwidth + HMARGIN + BUTTON_MARGIN;
    rect.x2 = rect.x1 + btnwidth;
    /* ����"����"��ť */
    prev_btn = rtgui_button_create("������");
    /* ����onbutton������demo_view_prev���� */
    rtgui_button_set_onbutton(prev_btn, opera_view_2);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(prev_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(prev_btn));

    /* ����AD��DA */
	arrange_adda_views(container);
    return;

}


rtgui_container_t *main_view(const char *title,struct rtgui_notebook * main_notebook)
{
    struct rtgui_container  *container;
    struct rtgui_label      *label;
    struct rtgui_staticline *line;
    struct rtgui_button     *next_btn, *prev_btn;
    struct rtgui_rect       rect,fontrect;
	int viewwidth = 0 , btnwidth = 0;

    container = rtgui_container_create();
    if (container == RT_NULL)
        return RT_NULL;

    rtgui_notebook_add(main_notebook, title, RTGUI_WIDGET(container));

    /* �����ͼ��λ����Ϣ(�ڼ��뵽 notebook ��ʱ��notebook ���Զ����� container
     * �Ĵ�С) */
    view_get_rect(container, &rect);
	viewwidth = rect.x2 - rect.x1;
	rtgui_font_get_metrics(rtgui_font_default(), title, &fontrect);
	btnwidth = (viewwidth - 2 * HMARGIN - 3 * (BUTTON_MARGIN+HMARGIN))/4;
	rect.x1 += (viewwidth-(fontrect.x2 - fontrect.x1)) /2;

    rect.y1 += VMARGIN * 3;
    rect.x2 = rect.x1 + fontrect.x2 - fontrect.x1;
    rect.y2 = rect.y1 + fontrect.y2 - fontrect.y1 + VMARGIN;

    /* ���������õı�ǩ */
    label = rtgui_label_create(title);
    /* ���ñ�ǩλ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect);
    /* ��ӱ�ǩ����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(label));
	
	client_get_rect(container, &rect);
	
    rect.y2 = rect.y1 + VMARGIN;
    /* ����һ��ˮƽ�� staticline �� */
    line = rtgui_staticline_create(RTGUI_HORIZONTAL);
    /* ���þ�̬�ߵ�λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(line), &rect);
    /* ��Ӿ�̬�ߵ���ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(line));
	/* ����һ���׶�ˮƽ�� staticline �� */
	client_get_rect(container,&rect);
	rect.y1 = rect.y2 - VMARGIN;
	line = rtgui_staticline_create(RTGUI_HORIZONTAL);
	rtgui_widget_set_rect(RTGUI_WIDGET(line),&rect);
	rtgui_container_add_child(container,RTGUI_WIDGET(line)); 


    /* ��õ׶˵�λ����Ϣ */
   	bottom_get_rect(container, &rect);
	rect.y1 += RTGUI_WIDGET_DEFAULT_MARGIN;
	rect.x1 += HMARGIN;
    rect.x2 = rect.x1 + btnwidth;
    rect.y2 = rect.y1 + BOTTOM_HEIGHT - 2 * RTGUI_WIDGET_DEFAULT_MARGIN ;
    
    /* ����"����"��ť */
    next_btn = rtgui_button_create("����");
    /* ����onbutton������demo_view_next���� */
    //rtgui_button_set_onbutton(next_btn, demo_view_next);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(next_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(next_btn));

    /* �����ͼ��λ����Ϣ */
    //rtgui_widget_get_rect(RTGUI_WIDGET(container), &rect);
    //rtgui_widget_rect_to_device(RTGUI_WIDGET(container), &rect);
    rect.x1 += btnwidth + HMARGIN + BUTTON_MARGIN;
    rect.x2 = rect.x1 + btnwidth;
    /* ����"����"��ť */
    prev_btn = rtgui_button_create("����");
    /* ����onbutton������demo_view_prev���� */
    //rtgui_button_set_onbutton(prev_btn, demo_view_prev);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(prev_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(prev_btn));

	rect.x1 += btnwidth + HMARGIN + BUTTON_MARGIN;
    rect.x2 = rect.x1 + btnwidth;
    /* ����"����"��ť */
    prev_btn = rtgui_button_create("��һҳ");
    /* ����onbutton������demo_view_prev���� */
    rtgui_button_set_onbutton(prev_btn, main_view_prev);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(prev_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(prev_btn));

	rect.x1 += btnwidth + HMARGIN + BUTTON_MARGIN;
    rect.x2 = rect.x1 + btnwidth;
    /* ����"����"��ť */
    prev_btn = rtgui_button_create("��һҳ");
    /* ����onbutton������demo_view_prev���� */
    rtgui_button_set_onbutton(prev_btn, main_view_next);
    /* ���ð�ť��λ����Ϣ */
    rtgui_widget_set_rect(RTGUI_WIDGET(prev_btn), &rect);
    /* ��Ӱ�ť����ͼ�� */
    rtgui_container_add_child(container, RTGUI_WIDGET(prev_btn));

    /* ���ش�������ͼ */
    return container;
}

void arrange_node_views(struct rtgui_notebook * nodebook,rtgui_rect_t * nodes_rect)
{
	rtgui_container_t * container;
	m_node_t pcur = g_pRootNode;
	char conname[14]={0};
	int pageindex = 0 ;
	//total height
	int theight = nodes_rect->y2 - nodes_rect->y1;
	//mnode height
	int mheight = (theight - 6 * VMARGIN - 2 * BUTTON_MARGIN) / 3 ;
	//mnode width
	int mwidth = (nodes_rect->x2 - nodes_rect->x1 - 3 * HMARGIN - BUTTON_MARGIN)/2;
	rtgui_rect_t rect = *nodes_rect;
	rtgui_button_t * button ;
	rect.x1 += HMARGIN;
	sprintf(conname,"page%02d",	pageindex);
	do{
		int nodecount = 0;
		container = rtgui_container_create();
		if (container == RT_NULL)
        	return;
		rect.y1 =  2*VMARGIN;
		while(nodecount<6 && (pcur = next_node(pcur))){
			char btntext[24]={0};
			sprintf(btntext,"ID%02d:IP1:IP0",pcur->ip);
			button = rtgui_button_create(btntext);
			rtgui_button_set_userdata(button,(void*)pcur);
			
			rect.x2 = rect.x1 + mwidth;
			rect.y2 = rect.y1 + mheight; 	
			rtgui_widget_set_rect(RTGUI_WIDGET(button), &rect);
		    rtgui_container_add_child(RTGUI_CONTAINER(container), RTGUI_WIDGET(button));

			rect.x1 = rect.x2 + HMARGIN + BUTTON_MARGIN;
			if(rect.x1 >= nodes_rect->x2){
			 	rect.x1 = nodes_rect->x1 + HMARGIN;
				rect.y1 = rect.y2 + VMARGIN + BUTTON_MARGIN;
			}
			else{
				
			}
			nodecount++;
		    /* ����onbuttonΪdemo_win_onbutton���� */
		    rtgui_button_set_onbutton(button, node_click_handle); 	
		}
		if(nodecount==0){  //�����˿�ҳ
			if(g_pRootNode==RT_NULL)
				rtgui_notebook_add(nodebook, conname, RTGUI_WIDGET(container));	
		   break;
		}
		else{
    		rtgui_notebook_add(nodebook, conname, RTGUI_WIDGET(container));
			sprintf(conname,"page%02d",++pageindex);
		}
	}while(pcur);
} 

rtgui_container_t *main_view_window(struct rtgui_notebook * main_notebook)
{
    rtgui_rect_t  rect;
    rtgui_container_t *container;
    //rtgui_button_t *button;

    /* ����һ����ʾ�õ���ͼ */
    container = main_view(MAINTITLE,main_notebook);

    client_get_rect(container, &rect);
	rect.y1 += VMARGIN;
	rect.y2 -= VMARGIN;
	nodebook = rtgui_notebook_create(&rect, RTGUI_NOTEBOOK_NOTAB);
    if (nodebook != RT_NULL)
    {
        rtgui_container_add_child(container, RTGUI_WIDGET(nodebook));
		arrange_node_views(nodebook,&rect);
    }
	operate_view(OPERATETITLE,main_notebook);

    return container;
}


void showgui(void * parameter)
{
	struct rtgui_app *app;
    struct rtgui_rect rect;
    app = rtgui_app_create("guiapp");
    if (app == RT_NULL)
        return;

    /* create a full screen window */
    rtgui_graphic_driver_get_rect(rtgui_graphic_driver_get_default(), &rect);

    main_win = rtgui_win_create(RT_NULL, "mainwin", &rect,
                                RTGUI_WIN_STYLE_NO_BORDER | RTGUI_WIN_STYLE_NO_TITLE);
    if (main_win == RT_NULL)
    {
        rtgui_app_destroy(app);
        return;
    }


    /* create a no title notebook that we can switch demo on it easily. */
    main_notebook = rtgui_notebook_create(&rect, RTGUI_NOTEBOOK_NOTAB);
    if (main_notebook == RT_NULL)
    {
        rtgui_win_destroy(main_win);
        rtgui_app_destroy(app);
        return;
    }

    rtgui_container_add_child(RTGUI_CONTAINER(main_win), RTGUI_WIDGET(main_notebook));
	/* ��ʼ���������ӵ���ͼ */
	main_view_window(main_notebook);
	
	rtgui_win_show(main_win, RT_FALSE);

    /* ִ�й���̨�¼�ѭ�� */
    rtgui_app_run(app);

    rtgui_app_destroy(app);

}

#endif
