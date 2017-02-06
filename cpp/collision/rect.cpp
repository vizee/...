#include "rect.h"
#include "common.h"
#include "v2f.h"

bool rect::test_rect(const rect &b)
{
	auto cp = v2f{b.x - x, b.y - y}.rotate(-d);
	auto cd = d - b.d;
	auto hbw = b.w / 2;
	auto hbh = b.h / 2;
	auto lt = v2f{-hbw, hbh}.rotate(cd);
	auto rt = v2f{hbw, hbh}.rotate(cd);
	v2f vs[4]{
		{cp.x + lt.x, cp.y + lt.y},
		{cp.x + rt.x, cp.y + rt.y},
		{cp.x - rt.x, cp.y - rt.y},
		{cp.x - lt.x, cp.y - lt.y}
	};
	auto hw = w / 2;
	auto hh = h / 2;
	for (auto &v : vs) {
		if (v.x >= -hw && v.x <= hw && v.y >= -hh && v.y <= hh) {
			return true;
		}
	}
	return false;
}
