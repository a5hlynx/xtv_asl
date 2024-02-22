#ifndef __ASL_LEGACY_H__
#define __ASL_LEGACY_H__

#include "asl.h"

#define _DB_TYPE_EMPTY 0
#define _DB_TYPE_HEADER 1
#define _DB_TYPE_MESSAGE 2
#define _DB_TYPE_KVLIST 3
#define _DB_TYPE_STRING 4
#define _DB_TYPE_STRCONT 5

struct asl_legacy_header {
	unsigned __int32 ver = 0;
	unsigned __int64 max_id = 0;
};

struct asl_legacy_rec {
	unsigned char type = 0;
	unsigned __int32 next = 0;
	std::string id = "";
	std::string ruid = "";
	std::string rgid = "";
	unsigned __int64 time_ui64 = 0;
	std::string time = "";
	std::string host = "";
	std::string sender = "";
	std::string facility = "";
	unsigned __int32 level_num = 0;
	std::string level = "";
	std::string pid = "";
	std::string uid = "";
	std::string gid = "";
	std::string message = "";
	std::string flags = "";
	unsigned __int32 kv_count = 0;
	std::map<std::string, std::string> kv_map;
	std::string kvs = "";
	void init() {
		kv_map.clear();
	}
};

int serialize_asl_legacy(struct asl_buf sbuf, struct asl_buf* dbuf);
int set_asl_header(struct asl_buf buf, struct asl_legacy_header* hdr);
void set_ref_maps(struct asl_buf sbuf,
	std::map<unsigned __int64, unsigned __int64>* ref_msg,
	std::map<unsigned __int64, unsigned __int64>* ref_str);
std::map<std::string, std::string> get_kv_map(asl_buf buf,
	std::map<unsigned __int64, unsigned __int64> ref_str,
	std::map<std::string, std::string> kv_map);
std::string get_timestamp(unsigned __int64 time);
std::string get_asl_str(asl_buf buf, std::map<unsigned __int64, unsigned __int64> ref_str);
std::string get_str(asl_buf buf, unsigned __int32 len);
std::string get_str_cont(asl_buf buf, unsigned __int32 len);

#endif