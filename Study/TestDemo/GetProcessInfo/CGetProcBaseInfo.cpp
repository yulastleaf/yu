#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>



#include "CGetProcBaseInfo.h"


CGetProcBaseInfo::CGetProcBaseInfo() {

}

CGetProcBaseInfo::~CGetProcBaseInfo() {

}

HANDLE CGetProcBaseInfo::GetProcessHandle(int nprocess_id)
{
	return OpenProcess(PROCESS_ALL_ACCESS, FALSE, nprocess_id);
}

//通过进程名（带后缀.exe）获取进程句柄
HANDLE CGetProcBaseInfo::GetProcessHandle(LPCWSTR lpprocess_name)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		return NULL;
	}

	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk;
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe))
	{
		if (!_tcsicmp(pe.szExeFile, lpprocess_name)) // 不区分大小写
		{
			CloseHandle(hSnapshot);
			return GetProcessHandle(pe.th32ProcessID);
		}
	}
	return NULL;
}

double CGetProcBaseInfo::CalculateCpuUsag(HANDLE hprocess) {
	long long int old_cpu_pkntime = 0;
	long long int old_cpu_purtime = 0;
	long long int old_cpu_sys_kntime = 0;
	long long int old_cpu_sys_urtime = 0;

	FILETIME ftCreate;
	FILETIME ftExit;
	FILETIME ftKernel_PID;
	FILETIME ftUser_PID;

	//GetProcessTimes()获取PID进程CPU空闲时间、内核时间、用户时间,单位是100ns
	// HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (GetProcessTimes(hprocess, &ftCreate, &ftExit, &ftKernel_PID, &ftUser_PID)) {
		old_cpu_pkntime = (long long int)FileTimeToDouble(ftKernel_PID);
		old_cpu_purtime = (long long int)FileTimeToDouble(ftUser_PID);
	}
	FILETIME ftIdle, ftKernel, ftUser;
	//GetSystemTimes()获取系统CPU空闲时间、内核时间、用户时间，单位：100ns
	if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser)) {
		old_cpu_sys_kntime = (long long int)FileTimeToDouble(ftKernel);
		old_cpu_sys_urtime = (long long int)FileTimeToDouble(ftUser);
	}
	Sleep(1000);
	if (GetProcessTimes(hprocess, &ftCreate, &ftExit, &ftKernel_PID, &ftUser_PID)) {
		old_cpu_pkntime = (long long int)FileTimeToDouble(ftKernel_PID) - old_cpu_pkntime;
		old_cpu_purtime = (long long int)FileTimeToDouble(ftUser_PID) - old_cpu_purtime;
	}
	if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser)) {
		old_cpu_sys_kntime = (long long int)FileTimeToDouble(ftKernel) - old_cpu_sys_kntime;
		old_cpu_sys_urtime = (long long int)FileTimeToDouble(ftUser) - old_cpu_sys_urtime;
	}
	/*
	* 进程当前cpu占用率=（进程内核时间增量+进程用户时间增量）/（系统内核时间增量+系统用户时间增量）
	*/
	return (double)(old_cpu_pkntime + old_cpu_purtime) * 100 / (double)(old_cpu_sys_kntime + old_cpu_sys_urtime);
}


double CGetProcBaseInfo::FileTimeToDouble(FILETIME& filetime) {
	return (double)(filetime.dwHighDateTime * 4.294967296E9) + (double)filetime.dwLowDateTime;
}

// 通过进程名获取cpu占用率
double CGetProcBaseInfo::CalculateCpuUsag(LPCWSTR lpprocess_name) {
	return CalculateCpuUsag(GetProcessHandle(lpprocess_name));
}

