#ifndef _RECT_H_
#define _RECT_H_

struct rect {
	float x;
	float y;
	float w;
	float h;
	float d;
	
	bool test_rect(const rect &b);
};

#endif // !_RECT_H_
