#include "md5p.h"

#define MD5P_XX(a, b, r, M, s, t)	(a = b + md5_rol(a + r + M + t, s))

void MD5p::GetSum(unsigned char * message, unsigned long length, unsigned char result[16])
{
	MD5p md5;
	unsigned int p = 0;
	for(p = 0; p + 64 <= length; p += 64) {
		md5.Update((unsigned long *)&message[p]);
	}
	md5.Final(message + p, length - p);
	md5.Output(result);
}

MD5p::MD5p()
{
	_chain[0] = md5_a;
	_chain[1] = md5_b;
	_chain[2] = md5_c;
	_chain[3] = md5_d;
	_length[0] = 0;
	_length[1] = 0;
}

void MD5p::Update(unsigned long M[16])
{
	unsigned long a = _chain[0];
	unsigned long b = _chain[1];
	unsigned long c = _chain[2];
	unsigned long d = _chain[3];
	MD5P_XX(a, b, md5_f(b, c, d), M[ 0],  7, 0xd76aa478);
	MD5P_XX(d, a, md5_f(a, b, c), M[ 1], 12, 0xe8c7b756);
	MD5P_XX(c, d, md5_f(d, a, b), M[ 2], 17, 0x242070db);
	MD5P_XX(b, c, md5_f(c, d, a), M[ 3], 22, 0xc1bdceee);
	MD5P_XX(a, b, md5_f(b, c, d), M[ 4],  7, 0xf57c0faf);
	MD5P_XX(d, a, md5_f(a, b, c), M[ 5], 12, 0x4787c62a);
	MD5P_XX(c, d, md5_f(d, a, b), M[ 6], 17, 0xa8304613);
	MD5P_XX(b, c, md5_f(c, d, a), M[ 7], 22, 0xfd469501);
	MD5P_XX(a, b, md5_f(b, c, d), M[ 8],  7, 0x698098d8);
	MD5P_XX(d, a, md5_f(a, b, c), M[ 9], 12, 0x8b44f7af);
	MD5P_XX(c, d, md5_f(d, a, b), M[10], 17, 0xffff5bb1);
	MD5P_XX(b, c, md5_f(c, d, a), M[11], 22, 0x895cd7be);
	MD5P_XX(a, b, md5_f(b, c, d), M[12],  7, 0x6b901122);
	MD5P_XX(d, a, md5_f(a, b, c), M[13], 12, 0xfd987193);
	MD5P_XX(c, d, md5_f(d, a, b), M[14], 17, 0xa679438e);
	MD5P_XX(b, c, md5_f(c, d, a), M[15], 22, 0x49b40821);
	MD5P_XX(a, b, md5_g(b, c, d), M[ 1],  5, 0xf61e2562);
	MD5P_XX(d, a, md5_g(a, b, c), M[ 6],  9, 0xc040b340);
	MD5P_XX(c, d, md5_g(d, a, b), M[11], 14, 0x265e5a51);
	MD5P_XX(b, c, md5_g(c, d, a), M[ 0], 20, 0xe9b6c7aa);
	MD5P_XX(a, b, md5_g(b, c, d), M[ 5],  5, 0xd62f105d);
	MD5P_XX(d, a, md5_g(a, b, c), M[10],  9,  0x2441453);
	MD5P_XX(c, d, md5_g(d, a, b), M[15], 14, 0xd8a1e681);
	MD5P_XX(b, c, md5_g(c, d, a), M[ 4], 20, 0xe7d3fbc8);
	MD5P_XX(a, b, md5_g(b, c, d), M[ 9],  5, 0x21e1cde6);
	MD5P_XX(d, a, md5_g(a, b, c), M[14],  9, 0xc33707d6);
	MD5P_XX(c, d, md5_g(d, a, b), M[ 3], 14, 0xf4d50d87);
	MD5P_XX(b, c, md5_g(c, d, a), M[ 8], 20, 0x455a14ed);
	MD5P_XX(a, b, md5_g(b, c, d), M[13],  5, 0xa9e3e905);
	MD5P_XX(d, a, md5_g(a, b, c), M[ 2],  9, 0xfcefa3f8);
	MD5P_XX(c, d, md5_g(d, a, b), M[ 7], 14, 0x676f02d9);
	MD5P_XX(b, c, md5_g(c, d, a), M[12], 20, 0x8d2a4c8a);
	MD5P_XX(a, b, md5_h(b, c, d), M[ 5],  4, 0xfffa3942);
	MD5P_XX(d, a, md5_h(a, b, c), M[ 8], 11, 0x8771f681);
	MD5P_XX(c, d, md5_h(d, a, b), M[11], 16, 0x6d9d6122);
	MD5P_XX(b, c, md5_h(c, d, a), M[14], 23, 0xfde5380c);
	MD5P_XX(a, b, md5_h(b, c, d), M[ 1],  4, 0xa4beea44);
	MD5P_XX(d, a, md5_h(a, b, c), M[ 4], 11, 0x4bdecfa9);
	MD5P_XX(c, d, md5_h(d, a, b), M[ 7], 16, 0xf6bb4b60);
	MD5P_XX(b, c, md5_h(c, d, a), M[10], 23, 0xbebfbc70);
	MD5P_XX(a, b, md5_h(b, c, d), M[13],  4, 0x289b7ec6);
	MD5P_XX(d, a, md5_h(a, b, c), M[ 0], 11, 0xeaa127fa);
	MD5P_XX(c, d, md5_h(d, a, b), M[ 3], 16, 0xd4ef3085);
	MD5P_XX(b, c, md5_h(c, d, a), M[ 6], 23,  0x4881d05);
	MD5P_XX(a, b, md5_h(b, c, d), M[ 9],  4, 0xd9d4d039);
	MD5P_XX(d, a, md5_h(a, b, c), M[12], 11, 0xe6db99e5);
	MD5P_XX(c, d, md5_h(d, a, b), M[15], 16, 0x1fa27cf8);
	MD5P_XX(b, c, md5_h(c, d, a), M[ 2], 23, 0xc4ac5665);
	MD5P_XX(a, b, md5_i(b, c, d), M[ 0],  6, 0xf4292244);
	MD5P_XX(d, a, md5_i(a, b, c), M[ 7], 10, 0x432aff97);
	MD5P_XX(c, d, md5_i(d, a, b), M[14], 15, 0xab9423a7);
	MD5P_XX(b, c, md5_i(c, d, a), M[ 5], 21, 0xfc93a039);
	MD5P_XX(a, b, md5_i(b, c, d), M[12],  6, 0x655b59c3);
	MD5P_XX(d, a, md5_i(a, b, c), M[ 3], 10, 0x8f0ccc92);
	MD5P_XX(c, d, md5_i(d, a, b), M[10], 15, 0xffeff47d);
	MD5P_XX(b, c, md5_i(c, d, a), M[ 1], 21, 0x85845dd1);
	MD5P_XX(a, b, md5_i(b, c, d), M[ 8],  6, 0x6fa87e4f);
	MD5P_XX(d, a, md5_i(a, b, c), M[15], 10, 0xfe2ce6e0);
	MD5P_XX(c, d, md5_i(d, a, b), M[ 6], 15, 0xa3014314);
	MD5P_XX(b, c, md5_i(c, d, a), M[13], 21, 0x4e0811a1);
	MD5P_XX(a, b, md5_i(b, c, d), M[ 4],  6, 0xf7537e82);
	MD5P_XX(d, a, md5_i(a, b, c), M[11], 10, 0xbd3af235);
	MD5P_XX(c, d, md5_i(d, a, b), M[ 2], 15, 0x2ad7d2bb);
	MD5P_XX(b, c, md5_i(c, d, a), M[ 9], 21, 0xeb86d391);
	_chain[0] += a;
	_chain[1] += b;
	_chain[2] += c;
	_chain[3] += d;
	addLength(64);
}

void MD5p::Final(unsigned char * tail, unsigned long length)
{
	unsigned char padding[128];

	addLength(length);
	int l = _length[0] & 0x3f;
	int p = 0;
	for (p = 0; p < l; p++) {
		padding[p] = tail[p];
	}
	int n = (l >= 56? 120: 56) - l;
	if (n > 0) {
		padding[p++] = 0x80;
		while (--n > 0) {
			padding[p++] = 0;
		}
	}
	((unsigned long *)(padding + p))[0] = _length[0] << 3;
	((unsigned long *)(padding + p))[1] = _length[1] << 3 | _length[0] >> 29;
	Update((unsigned long *)padding);
	if(l >= 56) {
		Update((unsigned long *)&padding[64]);
	}
}

void MD5p::Output(unsigned char result[16])
{
	for(int i = 0; i < 4; i++) {
		((unsigned long *)result)[i] = _chain[i];
	}
}
