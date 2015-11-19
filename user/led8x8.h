#ifndef  _LED8X8_H_
#define  _LED8X8_H_

extern unsigned char led_matrix[8];	/* 8x8 matrix buffer */

void led8x8_update(void);
void led8x8_setpattern(unsigned char data[8]);
void led8x8_num(int n);

#endif /* _LED8X8_H_ */
