#ifndef _MD5P_H_
#define _MD5P_H_

#ifndef ULONG_MAX
static_assert(sizeof(unsigned long) == 4, "undefined ULONG_MAX");
#define ULONG_MAX	0xffffffffUL
#endif

class MD5p
{
public:
	static void GetSum(unsigned char * message, unsigned long length, unsigned char result[16]);

private:
	static const unsigned long md5_a = 0x67452301;
	static const unsigned long md5_b = 0xefcdab89;
	static const unsigned long md5_c = 0x98badcfe;
	static const unsigned long md5_d = 0x10325476;

public:
	MD5p();
	
	void Update(unsigned long M[16]);
	void Final(unsigned char * tail, unsigned long length);
	void Output(unsigned char result[16]);
	
private:
	inline unsigned long md5_f(unsigned long x, unsigned long y, unsigned long z) {
		return (x & y) | ((~x) & z);
	}
	
	inline unsigned long md5_g(unsigned long x, unsigned long y, unsigned long z) {
		return (x & z) | (y & (~z));
	}
	
	inline unsigned long md5_h(unsigned long x, unsigned long y, unsigned long z) {
		return x ^ y ^ z;
	}
	
	inline unsigned long md5_i(unsigned long x, unsigned long y, unsigned long z) {
		return y ^ (x | (~z));
	}
	
	inline unsigned long md5_rol(unsigned long n, unsigned long c) {
		return (n << c) | (n >> (32 - c));
	}

	inline void addLength(unsigned long length) {
		if (ULONG_MAX - _length[0] < length) {
			_length[1]++;
		}
		_length[0] += length;
	}
	
private:
	unsigned long _chain[4];
	unsigned long _length[2];
	
};

#endif // !_MD5P_H_
