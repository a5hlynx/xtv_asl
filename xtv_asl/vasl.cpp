/*
	ASL Viewer X-Tension
	Copyright (C) 2024 Yuya Hashimoto

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

#pragma once
#include "X-tension.h"
#include "asl_legacy.h"
#include "asl_ver2.h"

#define MIN_VER	1990
#define NAME_BUF_LEN 256
#define MSG_BUF_LEN 1024
#define VER_BUF_LEN 10

int xwf_version = 0;
const wchar_t* XT_VER = L"XTV_ASL - v1.1.0";

WCHAR case_name[NAME_BUF_LEN] = { 0 };
wchar_t msg[MSG_BUF_LEN];
wchar_t VER[VER_BUF_LEN];

BOOLEAN EXIT = FALSE;
HANDLE heap = NULL;
unsigned __int64 h_ext = 1000;

int Serialize_Asl(struct asl_buf* sbuf, struct asl_buf* dbuf) {

	int ret = 0;
	unsigned int ver;
	ver = get_version(sbuf->ptr + sbuf->offset);

	switch (ver) {
	case _DB_VERSION_LEGACY_1:
		return serialize_asl_legacy(*sbuf, dbuf);
	case _DB_VERSION_2:
		return serialize_asl_ver2(*sbuf, dbuf);
	default:
		return -1;
	}
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

	BYTE _c[_ASL_DB_COOKIE_LEN] = { 0 };

	DWORD actual_size = 0;
	struct asl_buf sbuf, dbuf;
	const INT64 expected_size = XWF_GetSize(hItem, (LPVOID)1);

	actual_size = XWF_Read(hItem, 0, _c, _ASL_DB_COOKIE_LEN);

	if (actual_size != _ASL_DB_COOKIE_LEN || memcmp(_c, _ASL_DB_COOKIE, _ASL_DB_COOKIE_LEN)) {
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
		if (Serialize_Asl(&sbuf, &dbuf) != 0) {
			XWF_OutputMessage(L"XTV_ASL: Might be Corrupted or Malformed.", 0);
			HeapFree(heap, 0, sbuf.ptr);
			HeapFree(heap, 0, dbuf.ptr);
			*lpResSize = -1;
			return NULL;
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
