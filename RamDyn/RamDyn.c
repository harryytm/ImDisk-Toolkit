#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <winternl.h>
#include <ntstatus.h>
#include <aclapi.h>
#include <intrin.h>
#include "..\inc\imdproxy.h"

#define RtlGenRandom SystemFunction036
__declspec(dllimport) BOOLEAN __stdcall RtlGenRandom(PVOID RandomBuffer, ULONG RandomBufferLength);
typedef struct {int newmode;} _startupinfo;
__declspec(dllimport) int __cdecl __wgetmainargs(int *_Argc, wchar_t ***_Argv, wchar_t ***_Env, int _DoWildCard, _startupinfo * _StartInfo);

#define DEF_BUFFER_SIZE (1 << 20)
#define MINIMAL_MEM (100 << 20)

static IMDPROXY_INFO_RESP proxy_info = {};
static ULONGLONG max_activity;
static unsigned int clean_timer, clean_ratio;
static _Bool physical;
static volatile ULONGLONG data_amount;
static WCHAR *image_file_name, *add_param, *drive_arg;
static WCHAR txt[MAX_PATH + 320] = {};
static void **ptr_table, *virtual_mem_ptr;
static _Bool *allocated_block;
static volatile size_t n_block = 0;
static volatile _Bool report_offset = FALSE;
static __int64 cluster_offset = 0;
static VOLUME_BITMAP_BUFFER *clean_buf;
static DWORD err_time = 0;
static HANDLE current_process, h_image = INVALID_HANDLE_VALUE;
static SYSTEM_INFO sys;
static ULONG_PTR n_pages, *pfn;
static int mem_block_size, mem_block_size_mask, mem_block_size_shift; 
static _Bool (*data_search)(void *ptr, int size);

static void make_event_name();


static void disp_message(WCHAR *disp_text, WCHAR *arg, BOOL wait)
{
	DWORD dw;

	_snwprintf(txt, _countof(txt) - 1, disp_text, arg);
	WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, WTSGetActiveConsoleSessionId(), L"ImDisk", 14, txt, (wcslen(txt) + 1) * sizeof(WCHAR), MB_OK | MB_ICONERROR, 0, &dw, wait);
}

static void disp_err_mem()
{
	if (GetTickCount() - err_time >= 10000) {
		disp_message(L"Not enough memory to write data into %s.\nSome data will be lost.", drive_arg, TRUE);
		err_time = GetTickCount();
	}
}


static void virtual_mem_read(void *buf, int size, __int64 offset)
{
	size_t index = offset >> mem_block_size_shift;
	int current_size;
	int block_offset = offset & mem_block_size_mask;

	data_amount += size;
	do {
		current_size = min(size + block_offset, mem_block_size) - block_offset;
		if (ptr_table[index])
			memcpy(buf, ptr_table[index] + block_offset, current_size);
		else
			ZeroMemory(buf, current_size);
		block_offset = 0;
		buf += current_size;
		index++;
		size -= current_size;
	} while (size > 0);
}

static void physical_mem_read(void *buf, int size, __int64 offset)
{
	size_t index = offset >> mem_block_size_shift;
	int current_size;
	int block_offset = offset & mem_block_size_mask;

	data_amount += size;
	do {
		current_size = min(size + block_offset, mem_block_size) - block_offset;
		if (allocated_block[index]) {
			MapUserPhysicalPages(virtual_mem_ptr, n_pages, pfn + index * n_pages);
			memcpy(buf, virtual_mem_ptr + block_offset, current_size);
		} else
			ZeroMemory(buf, current_size);
		block_offset = 0;
		buf += current_size;
		index++;
		size -= current_size;
	} while (size > 0);
}


#ifndef _WIN64
static _Bool data_search_std(void *ptr, int size)
{
	size_t *scan_ptr;

	if (!size) return FALSE;
	scan_ptr = ptr;
	ptr += size - sizeof(size_t);
	if (*(size_t*)ptr) return TRUE;
	*(size_t*)ptr = 1;
	while (!*(scan_ptr++));
	*(size_t*)ptr = 0;
	return --scan_ptr != ptr;
}
#endif

__attribute__((__target__("sse2")))
_Bool data_search_sse2(void *ptr, int size)
{
	unsigned char *end_ptr;
	__m128i zero;

	if (!size) return FALSE;
	zero = _mm_setzero_si128();
	end_ptr = ptr + size - sizeof(__m128i);
	if ((unsigned short)_mm_movemask_epi8(_mm_cmpeq_epi8(*(__m128i*)end_ptr, zero)) != 0xffff) return TRUE;
	*end_ptr = 1;
	while ((unsigned short)_mm_movemask_epi8(_mm_cmpeq_epi8(*(__m128i*)ptr, zero)) == 0xffff) ptr += sizeof(__m128i);
	*end_ptr = 0;
	return ptr != end_ptr;
}

__attribute__((__target__("avx")))
_Bool data_search_avx(void *ptr, int size)
{
	unsigned char *end_ptr;
	__m256i one;

	if (!size) return FALSE;
	one = _mm256_set1_epi8(0xff);
	end_ptr = ptr + size - sizeof(__m256i);
	if (!_mm256_testz_si256(*(__m256i*)end_ptr, one)) return TRUE;
	*end_ptr = 1;
	while (_mm256_testz_si256(*(__m256i*)ptr, one)) ptr += sizeof(__m256i);
	*end_ptr = 0;
	return ptr != end_ptr;
}


static void virtual_mem_write(void *buf, int size, __int64 offset)
{
	size_t index = offset >> mem_block_size_shift;
	int current_size;
	int block_offset = offset & mem_block_size_mask;
	void *ptr;
	_Bool data;
	MEMORYSTATUSEX mem_stat;

	data_amount += size;
	if (report_offset && !memcmp(buf, clean_buf, 256)) {
		cluster_offset = offset;
		report_offset = FALSE;
	}
	mem_stat.dwLength = sizeof mem_stat;
	do {
		current_size = min(size + block_offset, mem_block_size) - block_offset;
		data = data_search(buf, current_size);
		if ((ptr = ptr_table[index])) {
			if (data)
				memcpy(ptr + block_offset, buf, current_size);
			else if (data_search(ptr, block_offset) || data_search(ptr + block_offset + current_size, mem_block_size - block_offset - current_size))
				ZeroMemory(ptr + block_offset, current_size);
			else {
				VirtualFree(ptr, 0, MEM_RELEASE);
				ptr_table[index] = NULL;
				n_block--;
			}
		}
		else if (data) {
			GlobalMemoryStatusEx(&mem_stat);
			if (mem_stat.ullAvailPageFile >= MINIMAL_MEM && (ptr_table[index] = VirtualAlloc(NULL, mem_block_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
				memcpy(ptr_table[index] + block_offset, buf, current_size);
				n_block++;
			} else
				disp_err_mem();
		}
		block_offset = 0;
		buf += current_size;
		index++;
		size -= current_size;
	} while (size > 0);
}

static void physical_mem_write(void *buf, int size, __int64 offset)
{
	size_t index = offset >> mem_block_size_shift;
	int current_size;
	int block_offset = offset & mem_block_size_mask;
	_Bool data;
	ULONG_PTR allocated_pages, *pfn_ptr;
	MEMORYSTATUSEX mem_stat;

	data_amount += size;
	if (report_offset && !memcmp(buf, clean_buf, 256)) {
		cluster_offset = offset;
		report_offset = FALSE;
	}
	mem_stat.dwLength = sizeof mem_stat;
	do {
		current_size = min(size + block_offset, mem_block_size) - block_offset;
		data = data_search(buf, current_size);
		if (allocated_block[index]) {
			MapUserPhysicalPages(virtual_mem_ptr, n_pages, pfn + index * n_pages);
			if (data)
				memcpy(virtual_mem_ptr + block_offset, buf, current_size);
			else if (data_search(virtual_mem_ptr, block_offset) || data_search(virtual_mem_ptr + block_offset + current_size, mem_block_size - block_offset - current_size))
				ZeroMemory(virtual_mem_ptr + block_offset, current_size);
			else {
				allocated_pages = n_pages;
				FreeUserPhysicalPages(current_process, &allocated_pages, pfn + index * n_pages);
				allocated_block[index] = FALSE;
				n_block--;
			}
		}
		else if (data) {
			GlobalMemoryStatusEx(&mem_stat);
			if (mem_stat.ullAvailPageFile < MINIMAL_MEM) disp_err_mem();
			else {
				allocated_pages = n_pages;
				pfn_ptr = pfn + index * n_pages;
				if (!AllocateUserPhysicalPages(current_process, &allocated_pages, pfn_ptr)) disp_err_mem();
				else if (allocated_pages != n_pages) {
					FreeUserPhysicalPages(current_process, &allocated_pages, pfn_ptr);
					disp_err_mem();
				} else {
					MapUserPhysicalPages(virtual_mem_ptr, n_pages, pfn_ptr);
					memcpy(virtual_mem_ptr + block_offset, buf, current_size);
					allocated_block[index] = TRUE;
					n_block++;
				}
			}
		}
		block_offset = 0;
		buf += current_size;
		index++;
		size -= current_size;
	} while (size > 0);
}


static void virtual_trim_process(DEVICE_DATA_SET_RANGE *range, int n)
{
	size_t index;
	int current_size, block_offset;
	__int64 size;
	void *ptr;

	while (n) {
		index = range->StartingOffset >> mem_block_size_shift;
		block_offset = range->StartingOffset & mem_block_size_mask;
		for (size = range->LengthInBytes; size > 0; size -= current_size) {
			current_size = min(size + block_offset, (__int64)mem_block_size) - block_offset;
			if ((ptr = ptr_table[index])) {
				if (data_search(ptr, block_offset) || data_search(ptr + block_offset + current_size, mem_block_size - block_offset - current_size))
					ZeroMemory(ptr + block_offset, current_size);
				else {
					VirtualFree(ptr, 0, MEM_RELEASE);
					ptr_table[index] = NULL;
					n_block--;
				}
			}
			block_offset = 0;
			index++;
		}
		range++;
		n--;
	}
}

static void physical_trim_process(DEVICE_DATA_SET_RANGE *range, int n)
{
	size_t index;
	int current_size, block_offset;
	__int64 size;
	ULONG_PTR allocated_pages;

	while (n) {
		index = range->StartingOffset >> mem_block_size_shift;
		block_offset = range->StartingOffset & mem_block_size_mask;
		for (size = range->LengthInBytes; size > 0; size -= current_size) {
			current_size = min(size + block_offset, (__int64)mem_block_size) - block_offset;
			if (allocated_block[index]) {
				MapUserPhysicalPages(virtual_mem_ptr, n_pages, pfn + index * n_pages);
				if (data_search(virtual_mem_ptr, block_offset) || data_search(virtual_mem_ptr + block_offset + current_size, mem_block_size - block_offset - current_size))
					ZeroMemory(virtual_mem_ptr + block_offset, current_size);
				else {
					allocated_pages = n_pages;
					FreeUserPhysicalPages(current_process, &allocated_pages, pfn + index * n_pages);
					allocated_block[index] = FALSE;
					n_block--;
				}
			}
			block_offset = 0;
			index++;
		}
		range++;
		n--;
	}
}


static DWORD __stdcall mem_clean(LPVOID lpParam)
{
	__int64 prev_diff = 0, new_diff;
	ULONGLONG free_bytes, total_bytes;
	WCHAR file_name[MAX_PATH + 20];
	HANDLE h_vol, h_event;
	IO_STATUS_BLOCK iosb;
	FILE_FS_SIZE_INFORMATION size_inf;
	USHORT compress = COMPRESSION_FORMAT_NONE;
	MOVE_FILE_DATA mfd;
	DWORD cluster_size, clean_block_size, dw;
	ssize_t current_mem_block, mem_block_count, n_clean_block, buf_size;
	int i, j;
	__int64 bit_pos, remain_cluster;
	BYTE pass;
	unsigned char sid[SECURITY_MAX_SID_SIZE];
	DWORD sid_size = sizeof sid;
	PACL pACL = NULL;
	EXPLICIT_ACCESS ea = {GENERIC_ALL, SET_ACCESS, SUB_CONTAINERS_AND_OBJECTS_INHERIT, {NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_WELL_KNOWN_GROUP, (LPTSTR)sid}};
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), &sd, FALSE};

	file_name[_countof(file_name) - 1] = 0;
	CreateWellKnownSid(WinWorldSid, NULL, (SID*)sid, &sid_size);
	SetEntriesInAcl(1, &ea, NULL, &pACL);
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE);
	make_event_name();
	h_event = CreateEvent(&sa, FALSE, FALSE, txt);

	for (;;) {
		dw = WaitForSingleObject(h_event, clean_timer);
		if (dw == WAIT_FAILED) Sleep(clean_timer);
		if (dw == WAIT_OBJECT_0) goto flush_vol;

		// check ramdisk activity
		for(;;) {
			data_amount = 0;
			Sleep(1000);
flush_vol:
			if (*(long*)(drive_arg + 1) == ':') {
				_snwprintf(file_name, _countof(file_name) - 1, L"\\\\.\\%s", drive_arg);
				h_vol = CreateFile(file_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
			} else
				h_vol = CreateFile(drive_arg, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_NO_BUFFERING, NULL);
			FlushFileBuffers(h_vol);
			if (dw == WAIT_OBJECT_0 || data_amount < max_activity) break;
			CloseHandle(h_vol);
		}

		// check whether the space between used bytes and allocated blocks has increased
		if (NtQueryVolumeInformationFile(h_vol, &iosb, &size_inf, sizeof size_inf, FileFsSizeInformation) != STATUS_SUCCESS) goto close_vol_handle;
		cluster_size = size_inf.BytesPerSector * size_inf.SectorsPerAllocationUnit;
		if (dw == WAIT_OBJECT_0) goto cleanup;

		total_bytes = size_inf.TotalAllocationUnits.QuadPart * cluster_size;
		free_bytes = size_inf.AvailableAllocationUnits.QuadPart * cluster_size;

		new_diff = (__int64)n_block * mem_block_size - (total_bytes - free_bytes);
		if (new_diff - prev_diff < (__int64)(total_bytes * clean_ratio / 1000)) goto close_vol_handle;

cleanup:
		for (i = 0;; i++) {
			_snwprintf(file_name, _countof(file_name) - 1, L"%s\\CLEAN%u", drive_arg, i);
			mfd.FileHandle = CreateFile(file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW,
										FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL);
			if (mfd.FileHandle != INVALID_HANDLE_VALUE) break;
			if (GetLastError() != ERROR_FILE_EXISTS) goto close_vol_handle;
		}
		DeviceIoControl(mfd.FileHandle, FSCTL_SET_COMPRESSION, &compress, sizeof compress, NULL, 0, &dw, NULL);

		clean_block_size = max(mem_block_size, cluster_size);
		buf_size = (proxy_info.file_size / cluster_size) / 8 + 17;
		if (clean_block_size > buf_size) buf_size = clean_block_size;
		if (!(clean_buf = VirtualAlloc(NULL, buf_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) goto close_handles;
		if (!RtlGenRandom(clean_buf, 256)) goto free_buf;
		report_offset = TRUE;
		if (!WriteFile(mfd.FileHandle, clean_buf, 4096, &dw, NULL)) goto free_buf;
		if (report_offset) {
			report_offset = FALSE;
			goto free_buf;
		}
		mfd.StartingVcn.QuadPart = 0;
		if (!DeviceIoControl(mfd.FileHandle, FSCTL_GET_RETRIEVAL_POINTERS, &mfd.StartingVcn, sizeof mfd.StartingVcn, clean_buf, 256, &dw, NULL)) goto free_buf;
		cluster_offset -= ((RETRIEVAL_POINTERS_BUFFER*)clean_buf)->Extents[0].Lcn.QuadPart * cluster_size;

		ZeroMemory(clean_buf, 256);
		SetFilePointerEx(mfd.FileHandle, (LARGE_INTEGER)0LL, NULL, FILE_BEGIN);
		if (!WriteFile(mfd.FileHandle, clean_buf, clean_block_size, &dw, NULL)) goto free_buf;

		mem_block_count = clean_block_size / mem_block_size;
		for (pass = 0; pass < 2; pass++) {
			mfd.ClusterCount = clean_block_size / cluster_size;
			n_clean_block = (cluster_offset + clean_block_size - 1) / clean_block_size;
			current_mem_block = n_clean_block * mem_block_count;
			mfd.StartingLcn.QuadPart = n_clean_block * mfd.ClusterCount - cluster_offset / cluster_size;
			DeviceIoControl(h_vol, FSCTL_GET_VOLUME_BITMAP, &mfd.StartingVcn, sizeof mfd.StartingVcn, clean_buf, buf_size, &dw, NULL);
			for (;;) {
				i = 0; do {
					if (physical ? allocated_block[current_mem_block + i] : ptr_table[current_mem_block + i] != NULL) {
						for (j = 0;;) {
							bit_pos = mfd.StartingLcn.QuadPart + j;
							if (clean_buf->Buffer[bit_pos >> 3] & (1 << (bit_pos & 7))) break;
							if (++j < mfd.ClusterCount) continue;
							DeviceIoControl(h_vol, FSCTL_MOVE_FILE, &mfd, sizeof mfd, NULL, 0, &dw, NULL);
							break;
						}
						break;
					}
				} while (++i < mem_block_count);
				current_mem_block += mem_block_count;
				mfd.StartingLcn.QuadPart += mfd.ClusterCount;
				remain_cluster = clean_buf->BitmapSize.QuadPart - mfd.StartingLcn.QuadPart;
				if (remain_cluster >= mfd.ClusterCount) continue;
				if (!remain_cluster) break;
				mfd.ClusterCount = remain_cluster;
			}
		}

		// retrieve the new difference between used bytes and allocated blocks
		CloseHandle(mfd.FileHandle);
		VirtualFree(clean_buf, 0, MEM_RELEASE);
		if (NtQueryVolumeInformationFile(h_vol, &iosb, &size_inf, sizeof size_inf, FileFsSizeInformation) != STATUS_SUCCESS) goto close_vol_handle;
		cluster_size = size_inf.BytesPerSector * size_inf.SectorsPerAllocationUnit;
		prev_diff = (__int64)n_block * mem_block_size - (size_inf.TotalAllocationUnits.QuadPart - size_inf.AvailableAllocationUnits.QuadPart) * cluster_size;
		goto close_vol_handle;

free_buf:
		VirtualFree(clean_buf, 0, MEM_RELEASE);
close_handles:
		CloseHandle(mfd.FileHandle);
close_vol_handle:
		CloseHandle(h_vol);
	}
	return 0;
}


static int do_comm()
{
	HANDLE shm_request_event, shm_response_event;
	unsigned char *shm_view, *main_buf;
	struct {unsigned char request_code, pad[7]; ULONGLONG offset; ULONGLONG length;} *req_block;
	struct {unsigned char errorno, pad[7]; ULONGLONG length;} *resp_block;
	struct {unsigned char request_code, pad[7]; unsigned int length;} *trim_block;
	HANDLE hFileMap;
	ULARGE_INTEGER map_size;
	char proxy_name[24], objname[40], *objname_ptr;
	SIZE_T min_working_set, max_working_set;
	DWORD size_read, data_size;
	__int64 offset = 0;
	STARTUPINFO si = {sizeof si};
	PROCESS_INFORMATION pi;
	MEMORYSTATUSEX mem_stat;
	int remain_in_buf, current_size, block_offset;
	ULONGLONG remain_to_read;
	size_t index;
	void *buf;
	ULONG_PTR allocated_pages, *pfn_ptr;

	sprintf(proxy_name, "RamDyn%I64x", _rdtsc());
	objname_ptr = objname + sprintf(objname, "Global\\%s", proxy_name);

	map_size.QuadPart = DEF_BUFFER_SIZE + IMDPROXY_HEADER_SIZE;

	if (!(hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, map_size.HighPart, map_size.LowPart, objname)) ||
		!(shm_view = MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0)))
		return 1;

	if (physical) {
		GetProcessWorkingSetSize(current_process, &min_working_set, &max_working_set);
		min_working_set += DEF_BUFFER_SIZE + IMDPROXY_HEADER_SIZE;
		max_working_set += DEF_BUFFER_SIZE + IMDPROXY_HEADER_SIZE;
		SetProcessWorkingSetSize(current_process, min_working_set, max_working_set);
		VirtualLock(shm_view, DEF_BUFFER_SIZE + IMDPROXY_HEADER_SIZE);
	}

	main_buf = shm_view + IMDPROXY_HEADER_SIZE;
	req_block = (void*)shm_view;
	resp_block = (void*)shm_view;
	trim_block = (void*)shm_view;

	// load of image file
	if (h_image != INVALID_HANDLE_VALUE) {
		mem_stat.dwLength = sizeof mem_stat;
		remain_to_read = proxy_info.file_size;
		while (remain_to_read > 0) {
			size_read = min(remain_to_read, DEF_BUFFER_SIZE);
			if (!(ReadFile(h_image, main_buf, size_read, &data_size, NULL) && data_size == size_read)) {
				disp_message(L"Error while reading %s.", image_file_name, FALSE);
				break;
			}
			buf = main_buf;
			remain_in_buf = size_read;
			index = offset >> mem_block_size_shift;
			block_offset = offset & mem_block_size_mask;
			do {
				current_size = min(remain_in_buf + block_offset, mem_block_size) - block_offset;
				if (data_search(buf, current_size)) {
					if (physical) {
						if (allocated_block[index])
							memcpy(virtual_mem_ptr + block_offset, buf, current_size);
						else {
							GlobalMemoryStatusEx(&mem_stat);
							if (mem_stat.ullAvailPageFile < MINIMAL_MEM) disp_err_mem();
							else {
								allocated_pages = n_pages;
								pfn_ptr = pfn + index * n_pages;
								if (!AllocateUserPhysicalPages(current_process, &allocated_pages, pfn_ptr)) disp_err_mem();
								else if (allocated_pages != n_pages) {
									FreeUserPhysicalPages(current_process, &allocated_pages, pfn_ptr);
									disp_err_mem();
								} else {
									MapUserPhysicalPages(virtual_mem_ptr, n_pages, pfn_ptr);
									memcpy(virtual_mem_ptr + block_offset, buf, current_size);
									allocated_block[index] = TRUE;
									n_block++;
								}
							}
						}
					} else {
						if (ptr_table[index])
							memcpy(ptr_table[index] + block_offset, buf, current_size);
						else {
							GlobalMemoryStatusEx(&mem_stat);
							if (mem_stat.ullAvailPageFile >= MINIMAL_MEM && (ptr_table[index] = VirtualAlloc(NULL, mem_block_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
								memcpy(ptr_table[index] + block_offset, buf, current_size);
								n_block++;
							} else
								disp_err_mem();
						}
					}
				}
				block_offset = 0;
				buf += current_size;
				index++;
				remain_in_buf -= current_size;
			} while (remain_in_buf > 0);
			offset += DEF_BUFFER_SIZE;
			remain_to_read -= size_read;
		}
		CloseHandle(h_image);
	}

	strcpy(objname_ptr, "_Request");
	if (!(shm_request_event = CreateEventA(NULL, FALSE, FALSE, objname)) || GetLastError() == ERROR_ALREADY_EXISTS)
		return 1;
	strcpy(objname_ptr, "_Response");
	if (!(shm_response_event = CreateEventA(NULL, FALSE, FALSE, objname)))
		return 1;

	_snwprintf(txt, _countof(txt) - 1, L"imdisk -a -t proxy -o shm -f %S -m \"%s\" %s", proxy_name, drive_arg, add_param);
	if (CreateProcess(NULL, txt, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	if (WaitForSingleObject(shm_request_event, 10000) != WAIT_OBJECT_0 || req_block->request_code != IMDPROXY_REQ_INFO)
		return 1;

	proxy_info.req_alignment = 1;
	memcpy(shm_view, &proxy_info, sizeof proxy_info);

	if (physical)
		for (;;) {
			SetEvent(shm_response_event);
			WaitForSingleObject(shm_request_event, INFINITE);

			if (req_block->request_code == IMDPROXY_REQ_READ)
				physical_mem_read(main_buf, req_block->length, req_block->offset);
			else if (req_block->request_code == IMDPROXY_REQ_WRITE)
				physical_mem_write(main_buf, req_block->length, req_block->offset);
			else if (req_block->request_code == IMDPROXY_REQ_UNMAP)
				physical_trim_process((DEVICE_DATA_SET_RANGE*)main_buf, trim_block->length / sizeof(DEVICE_DATA_SET_RANGE));
			else
				return 0;

			resp_block->errorno = 0;
			resp_block->length = req_block->length;
		}
	else
		for (;;) {
			SetEvent(shm_response_event);
			WaitForSingleObject(shm_request_event, INFINITE);

			if (req_block->request_code == IMDPROXY_REQ_READ)
				virtual_mem_read(main_buf, req_block->length, req_block->offset);
			else if (req_block->request_code == IMDPROXY_REQ_WRITE)
				virtual_mem_write(main_buf, req_block->length, req_block->offset);
			else if (req_block->request_code == IMDPROXY_REQ_UNMAP)
				virtual_trim_process((DEVICE_DATA_SET_RANGE*)main_buf, trim_block->length / sizeof(DEVICE_DATA_SET_RANGE));
			else
				return 0;

			resp_block->errorno = 0;
			resp_block->length = req_block->length;
		}
}

#pragma GCC optimize "Os"

static void make_event_name()
{
	WCHAR *ptr = &txt[13];

	_snwprintf(txt, MAX_PATH - 1, L"Global\\RamDyn_%s", drive_arg);
	do if (*ptr == '\\') *ptr = '/'; while (*(++ptr));
}

static LRESULT __stdcall WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE h, h_cpl;
	DWORD data_size;

	if (Msg == WM_ENDSESSION) {
		h_cpl = LoadLibraryA("imdisk.cpl");
		h = (HANDLE)GetProcAddress(h_cpl, "ImDiskOpenDeviceByMountPoint")(drive_arg, GENERIC_READ);
		if (!DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &data_size, NULL) || !DeviceIoControl(h, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &data_size, NULL))
			GetProcAddress(h_cpl, "ImDiskForceRemoveDevice")(h, 0);
		CloseHandle(h);
		return 0;
	} else
		return DefWindowProc(hWnd, Msg, wParam, lParam);
}

__declspec(noreturn) static DWORD __stdcall msg_window(LPVOID lpParam)
{
	MSG msg;
	WNDCLASSA wc = {};

	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "X";
	RegisterClassA(&wc);
	CreateWindowA("X", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);

	for (;;) {
		GetMessage(&msg, NULL, 0, 0);
		DispatchMessage(&msg);
	}
}


int __stdcall wWinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	int argc;
	WCHAR **argv, **env, *str_ptr;
	_startupinfo si = {};
	size_t n_block;
	HANDLE token;
	TOKEN_PRIVILEGES tok_priv;
	int CPUInfo[4];
	_Bool use_trim = FALSE;

	__wgetmainargs(&argc, &argv, &env, 0, &si);
	if (argc < 3) {
syntax_help:
		MessageBoxA(NULL, "RamDyn.exe MountPoint Size|ImageFile CleanRatio|TRIM [CleanTimer CleanMaxActivity] PhysicalMemory BlockSize AddParam\n\n"
						  "* MountPoint: a drive letter followed by a colon, or the full path to an empty NTFS folder.\n"
						  "* Size|ImageFile: size of the volume, in KB. With at least one non-numeric character, it is assumed to be the name of an image file to load. "
											"0 triggers the cleanup function for the specified mount point, if TRIM is not used (following parameters are ignored).\n"
						  "* CleanRatio: with -1, TRIM commands are used in replacement of the cleanup function, and the 2 following parameters are not used; "
										"otherwise, it's an approximate ratio, per 1000, of the total drive space from which the cleanup function attempts to free the memory of the deleted files (default: 10).\n"
						  "* CleanTimer: minimal time between 2 cleanups (default: 10).\n"
						  "* CleanMaxActivity: the cleanup function waits until reads and writes are below this value, in MB/s (default: 10).\n"
						  "* PhysicalMemory: use 0 for allocating virtual memory, 1 for allocating physical memory (default: 0); "
											"allocating physical memory requires the privilege to lock pages in memory in the local group policy.\n"
						  "* BlockSize: size of memory blocks, in power of 2, from 12 (4 KB) to 30 (1 GB) (default: 20).\n"
						  "* AddParam: additional parameters to pass to imdisk.exe. Use double-quotes for zero or several parameters.",
					"Syntax", MB_OK);
		ExitProcess(1);
	}
	drive_arg = argv[1];

	str_ptr = image_file_name = argv[2];
	if (!*str_ptr) ExitProcess(1);
	while (*str_ptr >= '0' && *str_ptr <= '9') str_ptr++;
	if (!*str_ptr) {
		if ((proxy_info.file_size = _wtoi64(argv[2]) << 10)) {
			if (argc < 7) goto syntax_help;
		} else {
			make_event_name();
			SetEvent(OpenEvent(EVENT_MODIFY_STATE, FALSE, txt));
			ExitProcess(0);
		}
	} else {
		if ((h_image = CreateFile(argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE) ExitProcess(1);
		GetFileSizeEx(h_image, (LARGE_INTEGER*)&proxy_info.file_size);
	}

	if ((clean_ratio = _wtoi(argv[3])) == -1) {
		use_trim = TRUE;
		proxy_info.flags = IMDPROXY_FLAG_SUPPORTS_UNMAP;
		argv -= 2;
	} else {
		if (argc < 9) goto syntax_help;
		clean_timer = _wtoi(argv[4]) * 1000;
		max_activity = _wtoi64(argv[5]) << 20;
	}
	physical = argv[6][0] - '0';

	mem_block_size_shift = _wtoi(argv[7]);
	if (mem_block_size_shift < 12) mem_block_size_shift = 12;
	if (mem_block_size_shift > 30) mem_block_size_shift = 30;
	mem_block_size = 1 << mem_block_size_shift;
	mem_block_size_mask = mem_block_size - 1;

	add_param = argv[8];

	n_block = (proxy_info.file_size + mem_block_size_mask) / mem_block_size;

	if (physical) {
		tok_priv.PrivilegeCount = 1;
		tok_priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		current_process = GetCurrentProcess();
		if (!OpenProcessToken(current_process, TOKEN_ADJUST_PRIVILEGES, &token) ||
			!LookupPrivilegeValueA(NULL, "SeLockMemoryPrivilege", &tok_priv.Privileges[0].Luid) ||
			!AdjustTokenPrivileges(token, FALSE, &tok_priv, 0, NULL, NULL) ||
			GetLastError() != ERROR_SUCCESS)
			ExitProcess(1);
		CloseHandle(token);
		GetSystemInfo(&sys);
		if (!(n_pages = mem_block_size / sys.dwPageSize)) ExitProcess(1);
		pfn = VirtualAlloc(NULL, n_pages * n_block * sizeof(size_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		allocated_block = VirtualAlloc(NULL, n_block * sizeof(_Bool), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		virtual_mem_ptr = VirtualAlloc(NULL, mem_block_size, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE);
	} else
		ptr_table = VirtualAlloc(NULL, n_block * sizeof(size_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	__cpuid(CPUInfo, 1);
#ifndef _WIN64
	data_search = data_search_std;
	if (CPUInfo[3] & 0x4000000)
#endif
		data_search = data_search_sse2;
	if ((CPUInfo[2] & 0x18000000) == 0x18000000 && ((unsigned char)_xgetbv(0) & 6) == 6) data_search = data_search_avx;

	SetProcessShutdownParameters(0x100, 0);
	CreateThread(NULL, 0, msg_window, NULL, 0, NULL);
	if (clean_timer && !use_trim) CreateThread(NULL, 0, mem_clean, NULL, 0, NULL);

	ExitProcess(do_comm());
}
