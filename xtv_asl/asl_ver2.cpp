#include "asl_ver2.h"

int serialize_asl_ver2(struct asl_buf sbuf, struct asl_buf* dbuf) {

	struct asl_ver2_header hdr;
	struct asl_ver2_rec rec;

	unsigned __int64 curr;

	if (set_asl_header(sbuf, &hdr) < 0)
		return -1;

	curr = hdr.f_offset;

	const char* _dlm = "\t";
	const char* _lb = "\n";

	while (true) {
		if (curr == hdr.l_offset)
			return 0;

		sbuf.offset = curr;

		for (int i = 0; i < 2; i++) {
			if ((sbuf.ptr + sbuf.offset)[i] != 0x00) {
				return -1;
			}
			sbuf.offset++;
		}
		rec.init();

		rec.len = get_int32(sbuf);
		sbuf.offset += 4;

		rec.next = get_int64(sbuf);
		sbuf.offset += 8;

		if ((rec.len == 0) && (rec.next == 0))
			return -1;

		rec.id = get_int64_str(sbuf);
		sbuf.offset += 8;

		rec.time = get_int64(sbuf);
		sbuf.offset += 8;

		rec.nano = get_int32(sbuf);
		sbuf.offset += 4;

		rec.level_num = get_int16(sbuf);
		sbuf.offset += 2;

		rec.flags = get_int16_str(sbuf);
		sbuf.offset += 2;

		rec.pid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.uid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.gid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.ruid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.rgid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.ref_pid = get_int32_str(sbuf);
		sbuf.offset += 4;

		rec.kv_count = get_int32(sbuf);
		sbuf.offset += 4;

		rec.host = get_asl_str(sbuf);
		sbuf.offset += 8;

		rec.sender = get_asl_str(sbuf);
		sbuf.offset += 8;

		rec.facility = get_asl_str(sbuf);
		sbuf.offset += 8;

		rec.message = get_asl_str(sbuf);
		sbuf.offset += 8;

		rec.ref_proc = get_asl_str(sbuf);
		sbuf.offset += 8;

		rec.session = get_asl_str(sbuf);
		sbuf.offset += 8;

		rec.kv_map = get_kv_map(sbuf, rec.kv_count);

		rec.time_nano = get_timestamp_nano(rec.time, rec.nano);
		rec.level = get_level((unsigned __int32)rec.level_num);
		rec.kvs = get_kvs(rec.kv_map);

		std::string serialized_record =
			rec.id + _dlm + rec.time_nano + _dlm +
			rec.level + _dlm + rec.flags + _dlm +
			rec.pid + _dlm + rec.uid + _dlm +
			rec.gid + _dlm + rec.ruid + _dlm +
			rec.rgid + _dlm + rec.ref_pid + _dlm +
			rec.host + _dlm + rec.sender + _dlm +
			rec.facility + _dlm + rec.message + _dlm +
			rec.ref_proc + _dlm + rec.session + _dlm +
			rec.kvs + _lb;

		if (set_rec(dbuf, serialized_record) != 0)
			return -1;

		curr = rec.next;

	}

	return 0;

}

int set_asl_header(struct asl_buf buf, struct asl_ver2_header* hdr) {

	buf.offset += _DB_HEADER_VERS_OFFSET;
	hdr->ver = get_int32(buf);
	buf.offset += 4;

	hdr->f_offset = get_int64(buf);
	buf.offset += 8;

	hdr->timestamp = get_int64(buf);
	buf.offset += 8;

	hdr->cache = get_int32(buf);
	buf.offset += 4;
	buf.offset += 1;

	hdr->l_offset = get_int64(buf);
	buf.offset += 8;

	for (int i = 0; i < 36; i++) {
		if (*(buf.ptr + buf.offset + i) != 0x00) {
			hdr->ver = -1;
			return -1;
		}
	}

	return 0;

}

std::string get_timestamp_nano(unsigned __int64 time, unsigned __int32 nano) {

	const int _len = 30;
	std::string str = "";
	char c[_len] = { 0 };

	struct tm _t;
	time_t _epoch = time;
	localtime_s(&_t, &_epoch);

	unsigned __int32 _nano = nano;
	if (snprintf((char*)c, _len * sizeof(char), "%d-%02d-%02d %02d:%02d:%02d.%09d",
		_t.tm_year + 1900, _t.tm_mon + 1, _t.tm_mday, _t.tm_hour, _t.tm_min, _t.tm_sec, nano) > 0) {

		str = std::string(c);

	}

	return str;
}


std::string get_asl_str(asl_buf buf) {

	const char _nb[8] = { 0 };
	std::string str = "";

	std::string s_str;
	unsigned __int64 len = 0;
	unsigned __int64 offset = 0;
	unsigned __int64  pos = buf.offset;

	if (((buf.ptr + buf.offset)[0] & 0x80) == 0x80) {
		len = 0x7F & (buf.ptr + buf.offset)[0];
		for (unsigned __int32 i = 0; i < len; i++)
			str = str + (char)(buf.ptr + buf.offset)[1 + i];
	}
	else if (memcmp(buf.ptr + buf.offset, _nb, 8) == 0) {
		str = "";
	}
	else {
		offset = get_int64(buf);
		buf.offset += 8;

		if (offset > buf.len) {
			return "";
		}
		if ((buf.ptr + offset)[0] != 0x00 || (buf.ptr + offset)[1] != 0x01) {
			return "";
		}
		offset += 2;
		buf.offset = offset;
		len = get_int32(buf);
		buf.offset += 4;
		char* c = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * len);
		if (c == 0)
			return "";

		memcpy_s(c, len / sizeof(c[0]), buf.ptr + buf.offset, len / sizeof(c[0]));
		str = (char*)c;
		HeapFree(heap, 0, c);
	}

	str = get_escaped_str(str);

	return str;

}

std::map<std::string, std::string> get_kv_map(asl_buf buf, unsigned __int32 kv_count) {

	std::map<std::string, std::string> kv_map;
	std::string k, v;

	kv_map.clear();

	for (unsigned __int32 i = 0; i < kv_count / 2; i++) {
		k = get_asl_str(buf);
		buf.offset += 8;
		v = get_asl_str(buf);
		buf.offset += 8;
		kv_map[k] = v;
	}

	return kv_map;

}
