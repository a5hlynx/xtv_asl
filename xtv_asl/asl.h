#ifndef __ASL_H__
#define __ASL_H__

#include<windows.h>
#include<string>
#include<map>
#include<ctime>

#define _DB_VERSION_LEGACY_1 1
#define _DB_VERSION_2 2

#define _ASL_LEVEL_EMERG 0
#define _ASL_LEVEL_ALERT 1
#define _ASL_LEVEL_CRIT 2
#define _ASL_LEVEL_ERR 3
#define _ASL_LEVEL_WARNING 4
#define _ASL_LEVEL_NOTICE 5
#define _ASL_LEVEL_INFO 6
#define _ASL_LEVEL_DEBUG 7

#define _ASL_DB_COOKIE "ASL DB"
#define _ASL_DB_COOKIE_LEN  6
#define _DB_HEADER_VERS_OFFSET 12

extern HANDLE heap;
extern unsigned __int64 h_ext;

struct asl_buf {
	char* ptr = 0;
	unsigned __int64 offset = 0;
	unsigned __int64 len = 0;
};

unsigned char get_byte(asl_buf buf);
unsigned __int16 get_int16(asl_buf buf);
unsigned __int32 get_int32(asl_buf buf);
unsigned __int64 get_int64(asl_buf buf);
std::string get_int16_str(asl_buf buf);
std::string get_int32_str(asl_buf buf);
std::string get_int64_str(asl_buf buf);
std::string get_level(unsigned __int32 num);
std::string get_kvs(std::map<std::string, std::string> kv_map);
std::string get_escaped_str(std::string str);
int set_rec(struct asl_buf* dbuf, std::string rec);
int get_version(char* buf);
std::string shift_jisfy(std::string src);
int extend_heap(struct asl_buf* buf, unsigned __int64 extend);

#endif