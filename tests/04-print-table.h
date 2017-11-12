// { 6,	{ "devlog",              0x15760000,	0x00a00000,	0x00000000,	}, },
#define _DEVLOG 0x00a00000
#define _CACHE 0x03b00000
#define _ONE_MB 0x100000
#define _USERDATA 0x09d00000

#define _SYSTEM (0x0ec60000 + (_CACHE - (4 * _ONE_MB)) + (_USERDATA - (128 * _ONE_MB)) + _DEVLOG )

{ 1,	{ "recovery",            0x02700000,	0x00500000,	0x00000000,	}, },
{ 2,	{ "boot",                0x02c00000,	0x00400000,	0x00000000,	}, },
{ 3,	{ "system",              0x03000000,	_SYSTEM,	0x00000000,	}, },
{ 4,	{ "cache",               0x11c60000,	(4 * _ONE_MB),	0x00000000,	}, },
{ 5,	{ "userdata",            0x16160000,	(128 * _ONE_MB),0x00000000,	}, },
{ 7,	{ "glumboot",            0x1fe60000,	0x00100000,	0x00000000,	}, },
{ 0,	{ "misc",                0x1ff60000,	0x000a0000,	0x00000000,	}, },
