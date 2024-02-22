#ifndef __ASL_VER2_H__
#define __ASL_VER2_H__

#include "asl.h"

struct asl_ver2_header {
	unsigned __int32 ver = 0;
	unsigned __int64 f_offset = 0;
	unsigned __int64 l_offset = 0;
	unsigned __int64 timestamp = 0;
	unsigned __int32 cache = 0;
};

struct asl_ver2_rec {
	unsigned __int64 len = 0;
	unsigned __int64 next = 0;
	std::string id = "";
	unsigned __int64 time = 0;
	unsigned __int32 nano = 0;
	std::string time_nano = "";
	unsigned __int16 level_num = 0;
	std::string level = "";
	std::string flags = "";
	std::string pid = "";
	std::string uid = "";
	std::string gid = "";
	std::string ruid = "";
	std::string rgid = "";
	std::string ref_pid = "";
	unsigned __int32 kv_count = 0;
	std::string host = "";
	std::string sender = "";
	std::string facility = "";
	std::string message = "";
	std::string ref_proc = "";
	std::string session = "";
	std::map<std::string, std::string> kv_map;
	std::string kvs = "";
	void init() {
		kv_map.clear();
	}
};

int serialize_asl_ver2(struct asl_buf sbuf, struct asl_buf* dbuf);
int set_asl_header(struct asl_buf buf, struct asl_ver2_header* hdr);
std::string get_timestamp_nano(unsigned __int64 time, unsigned __int32 nano);
std::string get_asl_str(asl_buf buf);
std::map<std::string, std::string> get_kv_map(asl_buf buf, unsigned __int32 kv_count);

#endif