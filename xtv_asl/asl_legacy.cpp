#include "asl_legacy.h"

int serialize_asl_legacy(struct asl_buf sbuf, struct asl_buf* dbuf) {

	struct asl_legacy_header hdr;
	struct asl_legacy_rec rec;
	std::map<unsigned __int64, unsigned __int64> ref_msg;
	std::map<unsigned __int64, unsigned __int64> ref_str;

	if (set_asl_header(sbuf, &hdr) < 0)
		return -1;

	sbuf.offset = 80;
	set_ref_maps(sbuf, &ref_msg, &ref_str);

	if (hdr.ver != _DB_VERSION_LEGACY_1)
		return -1;

	const char* _dlm = "\t";
	const char* _lb = "\n";

	for (auto it = ref_msg.begin(); it != ref_msg.end(); it++) {
		rec.init();
		sbuf.offset = it->second;
		rec.type = get_byte(sbuf);
		sbuf.offset += 1;

		rec.next = get_int32(sbuf);
		sbuf.offset += 4;

		rec.id = get_int64_str(sbuf);
		sbuf.offset += 8;

		rec.ruid = get_int32_str(sbuf);;
		sbuf.offset += 4;

		rec.rgid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.time_ui64 = get_int64(sbuf);
		sbuf.offset += 8;

		rec.host = get_asl_str(sbuf, ref_str);
		sbuf.offset += 8;

		rec.sender = get_asl_str(sbuf, ref_str);
		sbuf.offset += 8;

		rec.facility = get_asl_str(sbuf, ref_str);
		sbuf.offset += 8;

		rec.level_num = get_int32(sbuf);
		sbuf.offset += 4;

		rec.pid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.uid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.gid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.message = get_asl_str(sbuf, ref_str);
		sbuf.offset += 8;

		rec.flags = get_int16_str(sbuf);
		sbuf.offset += 2;

		if (rec.next != 0) {
			sbuf.offset = rec.next * 80;
			std::map<std::string, std::string> kv_map;
			kv_map.clear();
			rec.kv_map = get_kv_map(sbuf, ref_str, kv_map);
		}

		rec.time = get_timestamp(rec.time_ui64);
		rec.level = get_level(rec.level_num);
		rec.kvs = get_kvs(rec.kv_map);

		std::string serialized_record =
			rec.id + _dlm + rec.ruid + _dlm +
			rec.rgid + _dlm + rec.time + _dlm +
			rec.host + _dlm + rec.sender + _dlm +
			rec.facility + _dlm + rec.level + _dlm +
			rec.pid + _dlm + rec.uid + _dlm +
			rec.gid + _dlm + rec.message + _dlm +
			rec.flags + _dlm + rec.kvs + _lb;

		if (set_rec(dbuf, serialized_record) != 0)
			return -1;
	}

	return 0;

}

int set_asl_header(struct asl_buf buf, struct asl_legacy_header* hdr) {

	buf.offset += _DB_HEADER_VERS_OFFSET;
	hdr->ver = get_int32(buf);
	buf.offset += 4;

	hdr->max_id = get_int64(buf);
	buf.offset += 8;

	for (int i = 0; i < 56; i++) {
		if (*(buf.ptr + buf.offset + i) != 0x00) {
			hdr->ver = -1;
			return -1;
		}
	}

	return 0;

}

void set_ref_maps(struct asl_buf buf,
	std::map<unsigned __int64, unsigned __int64>* ref_msg,
	std::map<unsigned __int64, unsigned __int64>* ref_str) {

	unsigned __int64 _id = 0;
	unsigned __int64 _offset = 0;

	while (buf.offset < buf.len) {
		unsigned char type = get_byte(buf);
		buf.offset += 5;
		_id = get_int64(buf);
		buf.offset -= 5;
		_offset = buf.offset;
		switch (type) {
		case _DB_TYPE_MESSAGE:
			(*ref_msg)[_id] = _offset;
			break;
		case _DB_TYPE_STRING:
			(*ref_str)[_id] = _offset;
			break;
		default:
			break;
		}
		buf.offset += 80;
	}

}

std::map<std::string, std::string> get_kv_map(asl_buf buf,
	std::map<unsigned __int64, unsigned __int64> ref_str,
	std::map<std::string, std::string> kv_map) {

	unsigned char type = 0;
	unsigned __int32 next = 0;
	unsigned __int32 cnt = 0;
	unsigned __int64 offset = 0;

	type = get_byte(buf);
	buf.offset += 1;
	next = get_int32(buf);
	buf.offset += 4;
	cnt = get_int32(buf);
	buf.offset += 4;
	if (type != _DB_TYPE_KVLIST)
		return kv_map;

	if (cnt > 0) {
		for (unsigned int i = 0; i < cnt; i++) {
			std::string k = get_asl_str(buf, ref_str);
			buf.offset += 8;
			std::string v = get_asl_str(buf, ref_str);
			buf.offset += 8;
			kv_map[k] = v;
		}
	}
	if (next != 0) {
		buf.offset = next * 80;
		kv_map = get_kv_map(buf, ref_str, kv_map);
	}

	return kv_map;

}

std::string get_timestamp(unsigned __int64 time) {

	const int _len = 20;
	std::string str = "";
	char c[_len] = { 0 };
	struct tm _t;
	time_t _epoch = time;
	localtime_s(&_t, &_epoch);

	if (snprintf((char*)c, _len * sizeof(char), "%d-%02d-%02d %02d:%02d:%02d",
		_t.tm_year + 1900, _t.tm_mon + 1, _t.tm_mday, _t.tm_hour, _t.tm_min, _t.tm_sec) > 0) {
		str = std::string(c);
	}

	return str;

}


std::string get_asl_str(asl_buf buf, std::map<unsigned __int64, unsigned __int64> ref_str) {

	std::string str;
	unsigned __int32 len = 0;
	unsigned __int64 v = get_int64(buf);

	if (ref_str.find(v) == ref_str.end()) {
		if (((buf.ptr + buf.offset)[0] & 0x80) == 0x80) {
			len = 0x7F & (buf.ptr + buf.offset)[0];
			for (unsigned int i = 0; i < len; i++)
				str = str + (char)(buf.ptr + buf.offset)[1 + i];
		}
		else {
			str = "";
		}
	}
	else {
		unsigned __int64 pos = ref_str[v];
		buf.offset = pos;
		unsigned char type = get_byte(buf);
		buf.offset += 1;
		unsigned __int32 next = get_int32(buf);
		buf.offset += 4;
		unsigned __int64 id = get_int64(buf);
		buf.offset += 8;
		unsigned __int32 ref_count = get_int32(buf);
		buf.offset += 4;
		if (!(type == _DB_TYPE_STRING) && (id == v))
			return "";

		/* skip hash */
		buf.offset += 4;

		len = get_int32(buf);
		buf.offset += 4;

		if (len > 55) {
			str = get_str(buf, 55);
			len = len - 55;
		}
		else {
			str = get_str(buf, len);
			len = 0;
		}
		if (next != 0) {
			buf.offset = next * 80;
			str = str + get_str_cont(buf, len);
		}
	}

	str = get_escaped_str(str);
	return str;

}

std::string get_str(asl_buf buf, unsigned __int32 len) {

	std::string str = "";
	char* c = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * (len + 1));
	if (c == 0)
		return str;

	memcpy_s(c, len / sizeof(c[0]), (buf.ptr + buf.offset), len / sizeof(c[0]));
	str = (char*)c;
	HeapFree(heap, 0, c);

	str = get_escaped_str(str);

	return str;

}

std::string get_str_cont(asl_buf buf, unsigned __int32 len) {

	std::string str = "";
	unsigned char type = get_byte(buf);
	buf.offset += 1;
	unsigned __int32 next = get_int32(buf);
	buf.offset += 4;

	if (len > 75) {
		str = get_str(buf, 75);
		len = len - 75;
	}
	else {
		str = get_str(buf, len);
		len = 0;
	}

	if (next != 0) {
		buf.offset = next * 80;
		str = str + get_str_cont(buf, len);
	}

	return str;

}