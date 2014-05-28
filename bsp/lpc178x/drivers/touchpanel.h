
#define DMA_TP_CHANNEL 3
/* Terminal Counter flag for Channel 0 */
extern __IO uint32_t Channel_TP_TC;

/* Error Counter flag for Channel 0 */
extern __IO uint32_t Channel_TP_Err;

int has_touched(void);
void get_x_y(int * x ,int * y);
void tp_init(void);
void tp_entry(void * parameter);
