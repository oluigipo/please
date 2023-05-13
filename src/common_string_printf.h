#ifndef COMMON_STRING_PRINTF_H
#define COMMON_STRING_PRINTF_H

#include <stdarg.h>

static uintsize StringVPrintfBuffer(char* buf, uintsize len, const char* fmt, va_list args);
static uintsize StringPrintfBuffer(char* buf, uintsize len, const char* fmt, ...);
static String StringVPrintf(char* buf, uintsize len, const char* fmt, va_list args);
static String StringPrintf(char* buf, uintsize len, const char* fmt, ...);
static uintsize StringVPrintfSize(const char* fmt, va_list args);
static uintsize StringPrintfSize(const char* fmt, ...);

#define StringVPrintfLocal(size, ...) StringVPrintf((char[size]) { 0 }, size, __VA_ARGS__)
#define StringPrintfLocal(size, ...) StringPrintf((char[size]) { 0 }, size, __VA_ARGS__)

//~ NOTE(ljre): Implementation
static uintsize StringPrintfFunc_(char* buf, uintsize buf_size, const char* restrict fmt, va_list args);

static uintsize
StringVPrintfBuffer(char* buf, uintsize len, const char* fmt, va_list args)
{
	Assert(buf && len);
	
	return StringPrintfFunc_(buf, len, fmt, args);
}

static uintsize
StringPrintfBuffer(char* buf, uintsize len, const char* fmt, ...)
{
	Assert(buf && len);
	
	va_list args;
	va_start(args, fmt);
	
	uintsize result = StringPrintfFunc_(buf, len, fmt, args);
	
	va_end(args);
	return result;
}

static String
StringVPrintf(char* buf, uintsize len, const char* fmt, va_list args)
{
	Assert(buf && len);
	
	String result = {
		StringPrintfFunc_(buf, len, fmt, args),
		(uint8*)buf,
	};
	
	return result;
}

static String
StringPrintf(char* buf, uintsize len, const char* fmt, ...)
{
	Assert(buf && len);
	
	va_list args;
	va_start(args, fmt);
	
	String result = {
		StringPrintfFunc_(buf, len, fmt, args),
		(uint8*)buf,
	};
	
	va_end(args);
	
	return result;
}

static uintsize
StringVPrintfSize(const char* fmt, va_list args)
{
	return StringPrintfFunc_(NULL, 0, fmt, args);
}

static uintsize
StringPrintfSize(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	uintsize result = StringPrintfFunc_(NULL, 0, fmt, args);
	
	va_end(args);
	return result;
}

//- NOTE(ljre): Actual implementation for snprintf alternative.
#define String_STDSP_SPECIAL 0x7000
static int32 String_stbsp__real_to_str(char const** start, uint32* len, char* out, int32* decimal_pos, float64 value, uint32 frac_digits);

static inline void
StringWriteBuf_(char** p, char* end, uintsize* count, const char* restrict buf, intsize bufsize)
{
	Assert(bufsize >= 0);
	
	if (Likely(bufsize > 0))
	{
		if (end)
		{
			bufsize = Min(end - *p, bufsize);
			MemoryCopy(*p, buf, bufsize);
			*p += bufsize;
		}
		
		*count += bufsize;
	}
}

static inline void
StringFillBuf_(char** p, char* end, uintsize* count, char fill, intsize fillsize)
{
	Assert(fillsize >= 0);
	
	if (Likely(fillsize > 0))
	{
		if (end)
		{
			fillsize = Min(end - *p, fillsize);
			MemorySet(*p, (uint8)fill, fillsize);
			*p += fillsize;
		}
		
		*count += fillsize;
	}
}

static uintsize
StringPrintfFunc_(char* buf, uintsize buf_size, const char* restrict fmt, va_list args)
{
	uintsize count = 0;
	const char* fmt_end = fmt + MemoryStrlen(fmt);
	
	SafeAssert(!buf ? buf_size == 0 : true);
	
	char* p = buf;
	char* p_end = buf + buf_size;
	
	while (*fmt)
	{
		//- Copy leading chars
		const char* percent_addr = (const char*)MemoryFindByte(fmt, '%', fmt_end - fmt);
		
		if (percent_addr)
		{
			StringWriteBuf_(&p, p_end, &count, fmt, percent_addr - fmt);
			fmt = percent_addr;
		}
		else
		{
			StringWriteBuf_(&p, p_end, &count, fmt, fmt_end - fmt);
			fmt = fmt_end;
			break;
		}
		
		if (*fmt != '%')
			break;
		++fmt;
		
		//- Parse padding
		int32 leading_padding = -1;
		int32 trailling_padding = -1;
		
		if (*fmt >= '0' && *fmt <= '9')
		{
			leading_padding = 0;
			
			do
			{
				leading_padding *= 10;
				leading_padding += *fmt - '0';
				++fmt;
			}
			while (*fmt >= '0' && *fmt <= '9');
		}
		
		if (*fmt == '.')
		{
			++fmt;
			trailling_padding = 0;
			
			if (*fmt == '*')
			{
				trailling_padding = va_arg(args, int32);
				++fmt;
			}
			else while (*fmt >= '0' && *fmt <= '9')
			{
				trailling_padding *= 10;
				trailling_padding += *fmt - '0';
				++fmt;
			}
		}
		
		//- Parse format specifier
		char tmpbuf[128] = { 0 };
		const char* write_buf = tmpbuf;
		intsize write_count = 0;
		
		bool handled_write = false;
		char prefix_char = 0;
		char padding_char = ' ';
		
		switch (*fmt++)
		{
			default: write_count = 1; tmpbuf[0] = fmt[-1]; Assert(false); break;
			case '%': write_count = 1; tmpbuf[0] = '%'; break;
			case '0': write_count = 1; tmpbuf[0] = 0; break;
			case 'c':
			{
				int32 arg = va_arg(args, int32);
				write_count = 1;
				tmpbuf[0] = (char)arg;
			} break;
			
			//case 'i':
			//case 'I':
			{
				int64 arg;
				const char* intmin;
				
				if (0) case 'i': { arg = va_arg(args, int32); intmin = "-2147483648";          }
				if (0) case 'I': { arg = va_arg(args, int64); intmin = "-9223372036854775808"; }
				
				padding_char = '0';
				
				if (arg == INT64_MIN || arg == INT32_MIN && intmin[1] == '2')
				{
					write_buf = intmin;
					write_count = MemoryStrlen(intmin);
					
					break;
				}
				else if (arg < 0)
				{
					arg = -arg;
					prefix_char = '-';
				}
				else if (arg == 0)
				{
					tmpbuf[0] = '0';
					write_count = 1;
					
					break;
				}
				
				int32 index = sizeof(tmpbuf);
				
				while (index > 0 && arg > 0)
				{
					tmpbuf[--index] = (arg % 10) + '0';
					arg /= 10;
				}
				
				write_count = sizeof(tmpbuf) - index;
				write_buf = tmpbuf + index;
			} break;
			
			//case 'u':
			//case 'U':
			//case 'z':
			{
				uint64 arg;
				
				if (0) case 'u': arg = va_arg(args, uint32);
				if (0) case 'U': arg = va_arg(args, uint64);
				if (0) case 'z': arg = va_arg(args, uintsize);
				
				padding_char = '0';
				
				if (arg == 0)
				{
					tmpbuf[0] = '0';
					write_count = 1;
					break;
				}
				
				int32 index = sizeof(tmpbuf);
				
				while (index > 0 && arg > 0)
				{
					tmpbuf[--index] = (arg % 10) + '0';
					arg /= 10;
				}
				
				write_count = sizeof(tmpbuf) - index;
				write_buf = tmpbuf + index;
			} break;
			
			//case 'x':
			//case 'X':
			//case 'p':
			{
				uint64 arg;
				
				if (0) case 'x': arg = va_arg(args, uint32);
				if (0) case 'X': arg = va_arg(args, uint64);
				if (0) case 'p': arg = va_arg(args, uintptr);
				
				padding_char = '0';
				
				if (arg == 0)
				{
					tmpbuf[0] = '0';
					write_count = 1;
					break;
				}
				
				const char* chars = "0123456789ABCDEF";
				int32 index = sizeof(tmpbuf);
				
				while (index > 0 && arg > 0)
				{
					tmpbuf[--index] = chars[arg & 0xf];
					arg >>= 4;
				}
				
				write_count = sizeof(tmpbuf) - index;
				write_buf = tmpbuf + index;
			} break;
			
			case 's':
			{
				const char* arg = va_arg(args, const char*);
				uintsize len;
				
				if (!arg)
				{
					arg = "(null)";
					len = 6;
				}
				else if (trailling_padding >= 0)
				{
					const char* found = (const char*)MemoryFindByte(arg, 0, trailling_padding);
					
					if (found)
						len = found - arg;
					else
						len = trailling_padding;
					
					trailling_padding = -1;
				}
				else
					len = MemoryStrlen(arg);
				
				write_buf = arg;
				write_count = len;
			} break;
			
			case 'S':
			{
				String arg = va_arg(args, String);
				
				if (trailling_padding >= 0)
				{
					arg.size = Min(arg.size, trailling_padding);
					trailling_padding = -1;
				}
				
				write_buf = (const char*)arg.data;
				write_count = (intsize)arg.size;
			} break;
			
			case 'f':
			{
				float64 arg = va_arg(args, float64);
				
				if (trailling_padding == -1)
					trailling_padding = 8;
				
				char* start;
				uint32 length;
				int32 decimal_pos;
				
				static_assert(sizeof(tmpbuf) > 64 + 32, "stbsp__real_to_str takes just the final 64 bytes of the buffer because we might need to add a prefix to the string");
				
				bool negative = String_stbsp__real_to_str((const char**)&start, &length, tmpbuf+sizeof(tmpbuf)-64, &decimal_pos, arg, trailling_padding);
				
				if (decimal_pos == String_STDSP_SPECIAL)
				{
					if (start[0] == 'I' && negative)
					{
						write_buf = "-Inf";
						write_count = 4;
					}
					else
					{
						write_buf = start;
						write_count = 3;
					}
					
					break;
				}
				
				padding_char = '0';
				if (negative)
					prefix_char = '-';
				
				if (decimal_pos <= 0)
				{
					start -= -decimal_pos + 2;
					length += -decimal_pos + 2;
					
					SafeAssert(start >= tmpbuf);
					
					write_buf = start;
					write_count = length;
					
					*start++ = '0';
					*start++ = '.';
					
					while (decimal_pos++ < 0)
						*start++ = '0';
				}
				else
				{
					handled_write = true;
					if (negative)
						StringFillBuf_(&p, p_end, &count, '-', 1);
					
					if ((int32)length <= decimal_pos)
					{
						if (leading_padding != -1 && decimal_pos < leading_padding)
							StringFillBuf_(&p, p_end, &count, '0', leading_padding - decimal_pos);
						
						StringWriteBuf_(&p, p_end, &count, start, length);
						StringFillBuf_(&p, p_end, &count, '0', decimal_pos - length);
						StringFillBuf_(&p, p_end, &count, '.', 1);
						StringFillBuf_(&p, p_end, &count, '0', 2);
					}
					else
					{
						if (leading_padding != -1 && (int32)length - decimal_pos < leading_padding)
							StringFillBuf_(&p, p_end, &count, '0', leading_padding - (length - decimal_pos));
						
						StringWriteBuf_(&p, p_end, &count, start, decimal_pos);
						StringFillBuf_(&p, p_end, &count, '.', 1);
						StringWriteBuf_(&p, p_end, &count, start + decimal_pos, length - decimal_pos);
					}
				}
			} break;
		}
		
		if (!handled_write)
		{
			if (prefix_char)
				StringWriteBuf_(&p, p_end, &count, &prefix_char, 1);
			if (leading_padding != -1 && write_count < leading_padding)
				StringFillBuf_(&p, p_end, &count, padding_char, leading_padding - write_count);
			if (write_count)
				StringWriteBuf_(&p, p_end, &count, write_buf, write_count);
		}
	}
	
	return count;
}

//~ NOTE(ljre): All this code is derived from stb_sprint! Licensed under Unlicense.
static inline void
String_stbsp__copyfp(void* to, const void* f)
{
	*(uint64*)to = *(uint64*)f;
}

static inline void
String_stbsp__ddmulthi(float64* restrict oh, float64* restrict ol, float64 xh, float64 yh)
{
	float64 ahi = 0, alo, bhi = 0, blo;
	int64 bt;
	
	*oh = xh * yh;
	String_stbsp__copyfp(&bt, &xh);
	bt &= ((~(uint64)0) << 27);
	String_stbsp__copyfp(&ahi, &bt);
	alo = xh - ahi;
	String_stbsp__copyfp(&bt, &yh);
	bt &= ((~(uint64)0) << 27);
	String_stbsp__copyfp(&bhi, &bt);
	blo = yh - bhi;
	*ol = ((ahi * bhi - *oh) + ahi * blo + alo * bhi) + alo * blo;
}

static inline void
String_stbsp__ddmultlos(float64* restrict oh, float64* restrict ol, float64 xh, float64 yl)
{
	*ol = *ol + (xh * yl);
}

static inline void
String_stbsp__ddmultlo(float64* restrict oh, float64* restrict ol, float64 xh, float64 xl, float64 yh, float64 yl)
{
	*ol = *ol + (xh * yl + xl * yh);
}

static inline void
String_stbsp__ddrenorm(float64* restrict oh, float64* restrict ol)
{
	float64 s;
	s = *oh + *ol;
	*ol = *ol - (s - *oh);
	*oh = s;
}

static inline void
String_stbsp__ddtoS64(int64* restrict ob, float64 xh, float64 xl)
{
	float64 ahi = 0, alo, vh, t;
	*ob = (int64)xh;
	vh = (float64)*ob;
	ahi = (xh - vh);
	t = (ahi - xh);
	alo = (xh - (ahi - t)) - (vh + t);
	*ob += (int64)(ahi + alo + xl);
}

static void
String_stbsp__raise_to_power10(float64* restrict ohi, float64* restrict olo, float64 d, int32 power) // power can be -323 to +350
{
	static float64 const stbsp__bot[23] = {
		1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
		1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
	};
	static float64 const stbsp__negbot[22] = {
		1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
		1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
	};
	static float64 const stbsp__negboterr[22] = {
		-5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
		4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
		-3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
		2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
	};
	static float64 const stbsp__top[13] = {
		1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
	};
	static float64 const stbsp__negtop[13] = {
		1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
	};
	static float64 const stbsp__toperr[13] = {
		8388608,
		6.8601809640529717e+028,
		-7.253143638152921e+052,
		-4.3377296974619174e+075,
		-1.5559416129466825e+098,
		-3.2841562489204913e+121,
		-3.7745893248228135e+144,
		-1.7356668416969134e+167,
		-3.8893577551088374e+190,
		-9.9566444326005119e+213,
		6.3641293062232429e+236,
		-5.2069140800249813e+259,
		-5.2504760255204387e+282
	};
	static float64 const stbsp__negtoperr[13] = {
		3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
		-5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
		7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
		8.0970921678014997e-317
	};
	
	float64 ph, pl;
	if ((power >= 0) && (power <= 22)) {
		String_stbsp__ddmulthi(&ph, &pl, d, stbsp__bot[power]);
	} else {
		int32 e, et, eb;
		double p2h, p2l;
		
		e = power;
		if (power < 0)
			e = -e;
		et = (e * 0x2c9) >> 14; /* %23 */
		if (et > 13)
			et = 13;
		eb = e - (et * 23);
		
		ph = d;
		pl = 0.0;
		if (power < 0) {
			if (eb) {
				--eb;
				String_stbsp__ddmulthi(&ph, &pl, d, stbsp__negbot[eb]);
				String_stbsp__ddmultlos(&ph, &pl, d, stbsp__negboterr[eb]);
			}
			if (et) {
				String_stbsp__ddrenorm(&ph, &pl);
				--et;
				String_stbsp__ddmulthi(&p2h, &p2l, ph, stbsp__negtop[et]);
				String_stbsp__ddmultlo(&p2h, &p2l, ph, pl, stbsp__negtop[et], stbsp__negtoperr[et]);
				ph = p2h;
				pl = p2l;
			}
		} else {
			if (eb) {
				e = eb;
				if (eb > 22)
					eb = 22;
				e -= eb;
				String_stbsp__ddmulthi(&ph, &pl, d, stbsp__bot[eb]);
				if (e) {
					String_stbsp__ddrenorm(&ph, &pl);
					String_stbsp__ddmulthi(&p2h, &p2l, ph, stbsp__bot[e]);
					String_stbsp__ddmultlos(&p2h, &p2l, stbsp__bot[e], pl);
					ph = p2h;
					pl = p2l;
				}
			}
			if (et) {
				String_stbsp__ddrenorm(&ph, &pl);
				--et;
				String_stbsp__ddmulthi(&p2h, &p2l, ph, stbsp__top[et]);
				String_stbsp__ddmultlo(&p2h, &p2l, ph, pl, stbsp__top[et], stbsp__toperr[et]);
				ph = p2h;
				pl = p2l;
			}
		}
	}
	String_stbsp__ddrenorm(&ph, &pl);
	*ohi = ph;
	*olo = pl;
}

static int32
String_stbsp__real_to_str(char const** start, uint32* len, char out[64], int32* decimal_pos, float64 value, uint32 frac_digits)
{
	static uint64 const stbsp__powten[20] = {
		1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000,
		100000000,
		1000000000,
		10000000000ULL,
		100000000000ULL,
		1000000000000ULL,
		10000000000000ULL,
		100000000000000ULL,
		1000000000000000ULL,
		10000000000000000ULL,
		100000000000000000ULL,
		1000000000000000000ULL,
		10000000000000000000ULL
	};
	
	static const struct
	{
		alignas(2) char pair[201];
	}
	stbsp__digitpair =
	{
		"00010203040506070809101112131415161718192021222324"
			"25262728293031323334353637383940414243444546474849"
			"50515253545556575859606162636465666768697071727374"
			"75767778798081828384858687888990919293949596979899"
	};
	
	float64 d;
	int64 bits = 0;
	int32 expo, e, ng, tens;
	
	d = value;
	String_stbsp__copyfp(&bits, &d);
	
	expo = (int32)((bits >> 52) & 2047);
	ng = (int32)((uint64) bits >> 63);
	if (ng)
		d = -d;
	
	if (expo == 2047) // is nan or inf?
	{
		*start = (bits & ((((uint64)1) << 52) - 1)) ? "NaN" : "Inf";
		*decimal_pos = String_STDSP_SPECIAL;
		*len = 3;
		return ng;
	}
	
	if (expo == 0) // is zero or denormal
	{
		if (((uint64) bits << 1) == 0) // do zero
		{
			*decimal_pos = 1;
			*start = out;
			out[0] = '0';
			*len = 1;
			return ng;
		}
		// find the right expo for denormals
		{
			int64 v = ((uint64)1) << 51;
			while ((bits & v) == 0) {
				--expo;
				v >>= 1;
			}
		}
	}
	
	// find the decimal exponent as well as the decimal bits of the value
	{
		float64 ph, pl;
		
		// log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
		tens = expo - 1023;
		tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);
		
		// move the significant bits into position and stick them into an int
		String_stbsp__raise_to_power10(&ph, &pl, d, 18 - tens);
		
		// get full as much precision from double-double as possible
		String_stbsp__ddtoS64(&bits, ph, pl);
		
		// check if we undershot
		if (((uint64)bits) >= 1000000000000000000ULL/*stbsp__tento19th*/)
			++tens;
	}
	
	// now do the rounding in integer land
	frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
	if ((frac_digits < 24)) {
		uint32 dg = 1;
		if ((uint64)bits >= stbsp__powten[9])
			dg = 10;
		while ((uint64)bits >= stbsp__powten[dg]) {
			++dg;
			if (dg == 20)
				goto noround;
		}
		if (frac_digits < dg) {
			uint64 r;
			// add 0.5 at the right position and round
			e = dg - frac_digits;
			if ((uint32)e >= 24)
				goto noround;
			r = stbsp__powten[e];
			bits = bits + (r / 2);
			if ((uint64)bits >= stbsp__powten[dg])
				++tens;
			bits /= r;
		}
		noround:;
	}
	
	// kill long trailing runs of zeros
	if (bits) {
		uint32 n;
		for (;;) {
			if (bits <= 0xffffffff)
				break;
			if (bits % 1000)
				goto donez;
			bits /= 1000;
		}
		n = (uint32)bits;
		while ((n % 1000) == 0)
			n /= 1000;
		bits = n;
		donez:;
	}
	
	// convert to string
	out += 64;
	e = 0;
	for (;;) {
		uint32 n;
		char* o = out - 8;
		// do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
		if (bits >= 100000000) {
			n = (uint32)(bits % 100000000);
			bits /= 100000000;
		} else {
			n = (uint32)bits;
			bits = 0;
		}
		while (n) {
			out -= 2;
			*(uint16*)out = *(uint16*)&stbsp__digitpair.pair[(n % 100) * 2];
			n /= 100;
			e += 2;
		}
		if (bits == 0) {
			if ((e) && (out[0] == '0')) {
				++out;
				--e;
			}
			break;
		}
		while (out != o) {
			*--out = '0';
			++e;
		}
	}
	
	*decimal_pos = tens;
	*start = out;
	*len = e;
	return ng;
}

#undef String_STDSP_SPECIAL

#endif //COMMON_STRING_PRINTF_H