/*
	ASL Viewer X-Tension
	Copyright (C) 2020 Yuya Hashimoto

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "X-tension.h"
#include <windows.h>
#include <stdio.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <codecvt>
#define MIN_VER	1990
#define NAME_BUF_LEN 256
#define MSG_BUF_LEN 1024
#define VER_BUF_LEN 10

int xwf_version = 0;
const wchar_t* XT_VER = L"XTV_ASL - v1.0.1";

WCHAR case_name[NAME_BUF_LEN] = { 0 };
wchar_t msg[MSG_BUF_LEN];
wchar_t VER[VER_BUF_LEN];

BOOLEAN EXIT = FALSE;
HANDLE heap = NULL;
unsigned __int64 h_ext = 1000;
const char* dlm = ",";
const char* pre = "(";

struct c_buf {
	char* ptr = 0;
	unsigned __int64 offset = 0;
	unsigned __int64 len = 0;
};

const char asl_sig[] = { 'A', 'S', 'L', ' ', 'D', 'B', 0, 0, 0, 0, 0, 0 };
const char n_val[] = { 0,0,0,0,0,0,0,0 };

const int padding_size = 36;
struct asl_header {
	unsigned __int32 ver = 0;
	unsigned __int64 f_offset = 0;
	unsigned __int64 l_offset = 0;
	unsigned __int64 timestamp = 0;
	unsigned __int32 cache = 0;
};

struct os_pair {
	unsigned __int64 current_offset = 0;
	unsigned __int64 next_offset = 0;
};


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
	std::string dst(buf);

	HeapFree(heap, 0, w_buf);
	HeapFree(heap, 0, buf);
	return dst;

}


int extend_heap(struct c_buf* buf, unsigned __int64 extend = h_ext) {

	char* t;
	if ((t = (char*)HeapReAlloc(heap, HEAP_ZERO_MEMORY, buf->ptr, buf->len + extend)) == NULL) {
		return -1;
	}
	buf->ptr = t;
	buf->len += extend;
	return 0;

}
unsigned __int16 get_int16(struct c_buf* sbuf) {

	unsigned char buf;
	unsigned _int16 val = 0;
	for (int i = 0; i < 2; i++) {
		buf = (unsigned char)(sbuf->ptr + sbuf->offset)[i];
		val += (unsigned __int16)((unsigned __int16)buf << ((1 - i) * 8));
	}
	sbuf->offset += 2;
	return val;
}

unsigned __int32 get_int32(struct c_buf* sbuf) {

	unsigned char buf;
	unsigned _int32 val = 0;
	for (int i = 0; i < 4; i++) {
		buf = (unsigned char)(sbuf->ptr + sbuf->offset)[i];
		val += (unsigned __int32)((unsigned __int32)buf << ((3 - i) * 8));
	}
	sbuf->offset += 4;
	return val;
}
unsigned __int64 get_int64(struct c_buf* sbuf) {

	unsigned char buf;
	unsigned _int64 val = 0;
	for (int i = 0; i < 8; i++) {
		buf = (unsigned char)(sbuf->ptr + sbuf->offset)[i];
		val += (unsigned __int64)((unsigned __int64)buf << ((7 - i) * 8));
	}
	sbuf->offset += 8;
	return val;
}

int get_header(struct c_buf* sbuf, struct asl_header* hdr) {

	int offset = 0;
	int pd = padding_size;

	sbuf->offset += sizeof(asl_sig) / sizeof(char);
	hdr->ver = get_int32(sbuf);
	hdr->f_offset = get_int64(sbuf);
	hdr->timestamp = get_int64(sbuf);
	hdr->cache = get_int32(sbuf);
	sbuf->offset += 1;
	hdr->l_offset = get_int64(sbuf);
	for (int i = 0; i < pd; i++) {
		if (*(sbuf->ptr + sbuf->offset + i) != 0x00) {
			return -1;
		}
	}
	return 0;
}

void trim(char* buf, unsigned __int32 len) {

	for (unsigned __int32 i = 0; i < len; i++) {
		if (buf[i] == '\n') {
			buf[i] = ' ';
		}
	}
}


std::string rmv(std::string str) {

	std::string lb = "\n";
	std::string ws = " ";
	std::string::size_type pos = 0;
	while ((pos = str.find("\n", pos)) != std::string::npos) {
		str.replace(pos, lb.length(), ws);
		pos += ws.length();
	}
	return str;
}


int serialize_asl_string(struct c_buf* sbuf, struct c_buf* dbuf, const char* pre = "", const char* post = "") {

	std::string val, s_val;
	unsigned __int64 len = 0;
	unsigned __int64 offset = 0;
	char pad[2] = { 0 };
	unsigned __int64  current_pos = sbuf->offset;

	if (((sbuf->ptr + sbuf->offset)[0] & 0x80) == 0x80) {
		len = 0x7F & (sbuf->ptr + sbuf->offset)[0];
		for (unsigned __int32 i = 0; i < len; i++)
			val = val + (char)(sbuf->ptr + sbuf->offset)[1 + i];
	}
	else if (memcmp(sbuf->ptr + sbuf->offset, n_val, 8) == 0) {
		val = "";
	}
	else {
		offset = get_int64(sbuf);
		if (offset > sbuf->len) {
			return -1;
		}
		if ((sbuf->ptr + offset)[0] != 0x00 || (sbuf->ptr + offset)[1] != 0x01) {
			return -1;
		}
		offset += 2;
		sbuf->offset = offset;
		len = get_int32(sbuf);
		char* buf = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * len);
		if (buf == 0)
			return -1;
		memcpy_s(buf, len / sizeof(buf[0]), sbuf->ptr + sbuf->offset, len / sizeof(buf[0]));
		val = (char*)buf;
		HeapFree(heap, 0, buf);
	}

	val = rmv(val);
	s_val = shift_jisfy(val);
	len = (unsigned __int64)s_val.length() + strlen(pre) +  strlen(post); 
	while ((dbuf->offset + sizeof(char) * len + sizeof(char)) > dbuf->len) {
		if (extend_heap(dbuf, (sizeof(char) * len + sizeof(char))) != 0) {
			return -1;
		}
	}
	sbuf->offset = current_pos + 8;
	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset), dbuf->len * sizeof(char), "%s%s%s", pre, s_val.c_str(), post);


	return 0;

}

int serialize_timestamp(struct c_buf* sbuf, struct c_buf* dbuf, const char* delim = dlm) {

	struct tm t;
	time_t epoch = get_int64(sbuf);
	unsigned __int32 nano = get_int32(sbuf);
	while ((dbuf->offset + sizeof(unsigned __int32) * 7 + sizeof(char) * 8) > dbuf->len) {
		if (extend_heap(dbuf, (sizeof(unsigned __int32) * 7 + sizeof(char) * 8)) != 0) {
			return -1;
		}
	}
	localtime_s(&t, &epoch);
	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset),
		dbuf->len * sizeof(char),
		"%d/%02d/%02d %02d:%02d:%02d.%09d%s",
		t.tm_year + 1900,
		t.tm_mon + 1,
		t.tm_mday,
		t.tm_hour,
		t.tm_min,
		t.tm_sec,
		nano,
		delim
	);
	return 0;
}


int serialize_int64(struct c_buf* sbuf, struct c_buf* dbuf, const char* delim = dlm) {

	int ret = 0;
	unsigned _int64 val = get_int64(sbuf);
	while ((dbuf->offset + sizeof(unsigned __int64) + sizeof(char) * 2) > dbuf->len) {
		if (extend_heap(dbuf, (sizeof(unsigned __int64) + sizeof(char) * 2)) != 0) {
			return -1;
		}
	}
	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset), dbuf->len * sizeof(char), "%lld%s", val, delim);
	return 0;
}

int serialize_int32(struct c_buf* sbuf, struct c_buf* dbuf, const char* delim = dlm) {

	int ret = 0;
	unsigned _int32 val = get_int32(sbuf);
	while ((dbuf->offset + sizeof(unsigned __int32) + sizeof(char) * 2) > dbuf->len) {
		if (extend_heap(dbuf, (sizeof(unsigned __int32) + sizeof(char) * 2)) != 0) {
			return -1;
		}
	}
	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset), dbuf->len * sizeof(char), "%d%s", val, delim);
	return 0;
}


int serialize_int16(struct c_buf* sbuf, struct c_buf* dbuf, const char* delim = dlm) {

	int ret = 0;
	unsigned _int16 val = get_int16(sbuf);
	while ((dbuf->offset + sizeof(unsigned __int16) + sizeof(char) * 2) > dbuf->len) {
		if (extend_heap(dbuf, (sizeof(unsigned __int16) + sizeof(char) * 2)) != 0) {
			return -1;
		}
	}
	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset), dbuf->len * sizeof(char), "%hd%s", val, delim);
	return 0;

}

int serialize_level(struct c_buf* sbuf, struct c_buf* dbuf, const char* delim = dlm) {

	int ret = 0;
	unsigned _int16 val = get_int16(sbuf);
	char* level = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * 10);
	if (level == 0)
		return -1;

	switch (val) {
	case 0:
		strncpy_s(level, 10, "Emergency", 10);
		break;
	case 1:
		strncpy_s(level, 10, "Alert", 10);
		break;
	case 2:
		strncpy_s(level, 10, "Critical", 10);
		break;
	case 3:
		strncpy_s(level, 10, "Error", 10);
		break;
	case 4:
		strncpy_s(level, 10, "Warning", 10);
		break;
	case 5:
		strncpy_s(level, 10, "Notice", 10);
		break;
	case 6:
		strncpy_s(level, 10, "Info", 10);
		break;
	case 7:
		strncpy_s(level, 10, "Debug", 10);
		break;
	default:
		return -1;
	}

	while ((dbuf->offset + sizeof(char) * 12) > dbuf->len) {
		if (extend_heap(dbuf, (sizeof(char) * 12)) != 0) {
			return -1;
		}
	}

	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset), dbuf->len * sizeof(char), "%s%s", level, delim);
	HeapFree(heap, 0, level);
	return 0;
}

int serialize_kvs(struct c_buf* sbuf, struct c_buf* dbuf, unsigned __int32 kv_count, const char* delim = dlm) {

	for (unsigned __int32 i = 0; i < kv_count / 2; i++) {
		if (i == 0) {
			if (serialize_asl_string(sbuf, dbuf, "\"(", "") != 0)
				return -1;
		} else {
			if (serialize_asl_string(sbuf, dbuf, "(", "=") != 0)
				return -1;
		}
		if ((i + 1) == (kv_count) / 2) {
			if (serialize_asl_string(sbuf, dbuf, "", ")\"") != 0)
				return -1;
		}
		else {
			if (serialize_asl_string(sbuf, dbuf, "", ")") != 0)
				return -1;
		}
	}

	return 0;
}

LONG get_serialized_asl_record(struct c_buf* sbuf, struct c_buf* dbuf, struct os_pair* osp) {

	unsigned __int32 rec_len = 0;
	unsigned __int64 offset = 0;
	unsigned __int32 kv_count = 0;

	sbuf->offset = osp->current_offset;
	for (int i = 0; i < 2; i++) {
		if ((sbuf->ptr + osp->current_offset)[i] != 0x00) {
			return -1;
		}
		sbuf->offset++;
	}
	/* rec_len */
	rec_len = get_int32(sbuf);

	/* offset to next rec */
	osp->next_offset = get_int64(sbuf);

	if ((rec_len == 0) && (osp->next_offset == 0))
		return -1;

	/* numeric_id */
	if (serialize_int64(sbuf, dbuf) != 0)
		return -1;

	/* timestamp */
	if (serialize_timestamp(sbuf, dbuf) != 0)
		return -1;

	/* level */
	if (serialize_level(sbuf, dbuf) != 0)
		return -1;

	/* flags */
	if (serialize_int16(sbuf, dbuf) != 0)
		return -1;

	/* pid */
	if (serialize_int32(sbuf, dbuf) != 0)
		return -1;

	/* uid */
	if (serialize_int32(sbuf, dbuf) != 0)
		return -1;

	/* gid */
	if (serialize_int32(sbuf, dbuf) != 0)
		return -1;

	/* user_read_access */
	if (serialize_int32(sbuf, dbuf) != 0)
		return -1;

	/* group_read_access */
	if (serialize_int32(sbuf, dbuf) != 0)
		return -1;

	/* ref_pid */
	if (serialize_int32(sbuf, dbuf) != 0)
		return -1;

	/* kv_count */
	kv_count = get_int32(sbuf);

	/* host */
	if (serialize_asl_string(sbuf, dbuf, "\"", "\",") != 0)
		return -1;

	/* proc */
	if (serialize_asl_string(sbuf, dbuf, "\"", "\",") != 0)
		return -1;

	/* facility */
	if (serialize_asl_string(sbuf, dbuf, "\"", "\",") != 0)
		return -1;

	/* msg */
	if (serialize_asl_string(sbuf, dbuf, "\"", "\",") != 0)
		return -1;

	/* ref_proc */
	if (serialize_asl_string(sbuf, dbuf, "\"", "\",") != 0)
		return -1;

	/* session */
	if (serialize_asl_string(sbuf, dbuf, "\"", "\",") != 0)
		return -1;

	/* kvs */
	if (serialize_kvs(sbuf, dbuf, kv_count) != 0)
		return -1;

	/* lb */
	dbuf->offset += snprintf((char*)(dbuf->ptr + dbuf->offset), dbuf->len * sizeof(char), "\n");

	dbuf->len = dbuf->offset;

	return 0;
}

LONG get_serialized_asl_records(struct c_buf* sbuf, struct c_buf* dbuf) {

	int ret = 0;
	struct os_pair osp;
	struct asl_header hdr;
	if (get_header(sbuf, &hdr) < 0) {
		return -1;
	}
	osp.current_offset = hdr.f_offset;
	while (true) {
		if (get_serialized_asl_record(sbuf, dbuf, &osp) != 0)
			return -1;
		if (osp.current_offset == hdr.l_offset)
			break;
		osp.current_offset = osp.next_offset;
	}
	return 0;
}

// XT_Init
LONG __stdcall XT_Init(CallerInfo info, DWORD nFlags, HANDLE hMainWnd, void* lpReserved) {

	if ((XT_INIT_XWF & nFlags) == 0 ||
		XT_INIT_WHX & nFlags ||
		XT_INIT_XWI & nFlags ||
		XT_INIT_BETA & nFlags
		) {
		return -1;
	}

	if (XT_INIT_ABOUTONLY & nFlags ||
		XT_INIT_QUICKCHECK & nFlags
		) {
		return 1;
	}

	XT_RetrieveFunctionPointers();

	if (info.version < MIN_VER) {
		wcscpy_s(msg, L"XTV_ASL: The Version of X-Ways Forensics must be v.");
		swprintf_s(VER, L"%d", MIN_VER);
		wcscat_s(msg, VER);
		wcscat_s(msg, L" or Later. Exiting...");
		XWF_OutputMessage(msg, 0);
		EXIT = TRUE;
		return 1;
	}

	if (XWF_GetCaseProp(NULL, XWF_CASEPROP_TITLE, case_name, NAME_BUF_LEN) == -1) {
		XWF_OutputMessage(L"XTV_ASL: Active Case is Required. Exiting...", 0);
		EXIT = TRUE;
		return 1;
	}

	if (!XWF_GetFirstEvObj(NULL)) {
		XWF_OutputMessage(L"XTV_ASL: No Evidence is Found. Exiting...", 0);
		EXIT = TRUE;
		return 1;
	}

	heap = GetProcessHeap();
	if (!heap) {
		XWF_OutputMessage(L"XTV_ASL: Unable to Allocate Memory. Exiting...", 0);
		EXIT = TRUE;
		return 1;
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// XT_Done
LONG __stdcall XT_Done(void* lpReserved) {

	if (EXIT) {
		return 0;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// XT_About
LONG __stdcall XT_About(HANDLE hParentWnd, void* lpReserved) {

	MessageBox(NULL, (wchar_t*)XT_VER, TEXT("about"), MB_OK);
	return 0;

}

///////////////////////////////////////////////////////////////////////////////
// XT_View
void* __stdcall XT_View(HANDLE hItem, LONG nItemID, HANDLE hVolume, HANDLE hEvidence, void* lpReserved, PINT64 lpResSize) {

	char chk_sig[12] = { 0 };
	DWORD actual_size = 0;
	struct c_buf sbuf, dbuf;
	const INT64 expected_size = XWF_GetSize(hItem, (LPVOID)1);

	actual_size = XWF_Read(hItem, 0, (BYTE*)chk_sig, 12);
	if (actual_size != 12 || memcmp(chk_sig, asl_sig, sizeof(asl_sig) / sizeof(char))) {
		return NULL;
	}

	sbuf.ptr = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * expected_size);
	if (sbuf.ptr == NULL) {
		return NULL;
	}
	sbuf.len = expected_size;

	dbuf.ptr = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(char) * expected_size);
	if (dbuf.ptr == NULL) {
		return NULL;
	}
	dbuf.len = expected_size;

	actual_size = XWF_Read(hItem, 0, (BYTE*)sbuf.ptr, (DWORD)sbuf.len);

	if (actual_size == (sbuf.len * sizeof(char))) {
		if (get_serialized_asl_records(&sbuf, &dbuf) != 0) {
			XWF_OutputMessage(L"XTV_ASL: Might be Corrupted or Malformed.", 0);
		}
	}
	else {
		XWF_OutputMessage(L"XTV_ASL: Unable to Read the File.", 0);
		HeapFree(heap, 0, sbuf.ptr);
		HeapFree(heap, 0, dbuf.ptr);
		*lpResSize = -1;
		return NULL;
	}

	*lpResSize = dbuf.len;
	return dbuf.ptr;

}

///////////////////////////////////////////////////////////////////////////////
// XT_ReleaseMem
bool __stdcall XT_ReleaseMem(void* lpBuffer) {

	if (lpBuffer && (HeapFree(heap, 0, lpBuffer) != 0)) {
		return TRUE;
	}
	else {
		return FALSE;
	}

}