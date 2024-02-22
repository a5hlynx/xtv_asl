#include"asl.h"

unsigned char get_byte(asl_buf buf) {

	unsigned char uc;
	uc = (unsigned char)(buf.ptr + buf.offset)[0];
	return uc;

}

unsigned __int16 get_int16(asl_buf buf) {

	unsigned char uc;
	unsigned _int16 ui16 = 0;
	for (int i = 0; i < 2; i++) {
		uc = (unsigned char)(buf.ptr + buf.offset)[i];
		ui16 += (unsigned __int16)((unsigned __int16)uc << ((1 - i) * 8));
	}

	return ui16;

}

unsigned __int32 get_int32(asl_buf buf) {

	unsigned char uc;
	unsigned _int32 ui32 = 0;
	for (int i = 0; i < 4; i++) {
		uc = (unsigned char)(buf.ptr + buf.offset)[i];
		ui32 += (unsigned __int32)((unsigned __int32)uc << ((3 - i) * 8));
	}

	return ui32;

}

unsigned __int64 get_int64(asl_buf buf) {

	unsigned char uc;
	unsigned __int64 ui64 = 0;
	for (int i = 0; i < 8; i++) {
		uc = (unsigned char)(buf.ptr + buf.offset)[i];
		ui64 += (unsigned __int64)((unsigned __int64)uc << ((7 - i) * 8));
	}

	return ui64;

}

std::string get_int16_str(asl_buf buf) {

	const int _len = 10;
	std::string str = "";
	char c[_len] = { 0 };

	unsigned __int16 ui16 = get_int16(buf);
	if (snprintf((char*)c, _len * sizeof(char), "%hd", ui16) > 0) {
		str = std::string(c);
	}

	return str;

}

std::string get_int32_str(asl_buf buf) {

	const int _len = 15;
	std::string str = "";
	char c[_len] = { 0 };

	unsigned __int32 ui32 = get_int32(buf);
	if (snprintf((char*)c, _len * sizeof(char), "%d", ui32) > 0) {
		str = std::string(c);
	}

	return str;

}

std::string get_int64_str(asl_buf buf) {

	const int _len = 30;
	std::string str = "";
	char c[_len] = { 0 };

	unsigned __int64 ui64 = get_int64(buf);
	if (snprintf((char*)c, _len * sizeof(char), "%lld", ui64) > 0) {
		str = std::string(c);
	}

	return str;

}

std::string get_level(unsigned __int32 num) {

	const int _len = 10;
	std::string str = "";
	char c[_len] = { 0 };

	switch (num) {
	case _ASL_LEVEL_EMERG:
		strncpy_s(c, _len, "Emergency", _len);
		break;
	case _ASL_LEVEL_ALERT:
		strncpy_s(c, _len, "Alert", _len);
		break;
	case _ASL_LEVEL_CRIT:
		strncpy_s(c, _len, "Critical", _len);
		break;
	case _ASL_LEVEL_ERR:
		strncpy_s(c, _len, "Error", _len);
		break;
	case _ASL_LEVEL_WARNING:
		strncpy_s(c, _len, "Warning", _len);
		break;
	case _ASL_LEVEL_NOTICE:
		strncpy_s(c, _len, "Notice", _len);
		break;
	case _ASL_LEVEL_INFO:
		strncpy_s(c, _len, "Info", _len);
		break;
	case _ASL_LEVEL_DEBUG:
		strncpy_s(c, _len, "Debug", _len);
		break;
	default:
		return str;
	}

	str = std::string(c);

	return str;

}

std::string get_kvs(std::map<std::string, std::string> kv_map) {

	std::string _qt = "\'";
	std::string str = "";
	int cnt = 0;

	for (auto it = kv_map.begin(); it != kv_map.end(); it++) {
		if (cnt > 0)
			str = str + ", ";
		str = str + _qt + it->first + _qt + ":" + _qt + it->second + _qt;
		cnt++;
	}

	if (str.length() > 0)
		str = "{" + str + "}";

	return str;

}

std::string get_escaped_str(std::string str) {

	std::string _lb = "\n";
	std::string _ws = "\\n";
	std::string::size_type _pos = 0;

	while ((_pos = str.find(_lb, _pos)) != std::string::npos) {
		str.replace(_pos, _lb.length(), _ws);
		_pos += _ws.length();
	}

	return str;

}

int set_rec(struct asl_buf* buf, std::string rec) {

	std::string val = shift_jisfy(rec);
	unsigned __int64 len = (unsigned __int64)val.length();
	while ((buf->offset + sizeof(char) * (len + 1)) > buf->len) {
		if (extend_heap(buf, sizeof(char) * (h_ext)) != 0) {
			return -1;
		}
	}
	buf->offset += snprintf((char*)(buf->ptr + buf->offset), buf->len * sizeof(char), "%s", val.c_str());

	return 0;

}

int get_version(char* buf) {

	int ver = 0;
	if (memcmp(buf, _ASL_DB_COOKIE, _ASL_DB_COOKIE_LEN) != 0) {
		return -1;
	}
	ver = (unsigned char)(buf + _DB_HEADER_VERS_OFFSET + 3)[0];

	return ver;

}

std::string shift_jisfy(std::string src) {

	int w_len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)(src.size() + 1), NULL, 0);

	wchar_t* w_buf = (wchar_t*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(wchar_t) * w_len);
	if (w_buf == 0)
		return "";

	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)(src.size() + 1), w_buf, w_len);

	int len = WideCharToMultiByte(CP_THREAD_ACP, 0, w_buf, -1, NULL, 0, NULL, NULL);
	char* buf = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * len);
	if (buf == 0)
		return "";

	WideCharToMultiByte(CP_THREAD_ACP, 0, w_buf, w_len + 1, buf, len, NULL, NULL);
	std::string dst = std::string(buf);

	HeapFree(heap, 0, w_buf);
	HeapFree(heap, 0, buf);

	return dst;

}

int extend_heap(struct asl_buf* buf, unsigned __int64 extend = h_ext) {

	char* t;
	if ((t = (char*)HeapReAlloc(heap, HEAP_ZERO_MEMORY, buf->ptr, buf->len + extend)) == NULL) {
		return -1;
	}
	buf->ptr = t;
	buf->len += extend;

	return 0;

}
