#ifndef GET_PROC_BASE_INFO_H_
#define GET_PROC_BASE_INFO_H_

class CGetProcBaseInfo {

public:
	CGetProcBaseInfo();
	~CGetProcBaseInfo();

public:

	//获取进程cpu占用率
	double CalculateCpuUsag(HANDLE hprocess);

	// 通过进程名获取cpu占用率
	double CalculateCpuUsag(LPCWSTR lpprocess_name);

private:

	//通过进程ID获取进程句柄
	//PROCESS_ALL_ACCESS 获取所有权限
	//PROCESS_QUERY_INFORMATION 获取进程的令牌、退出码和优先级等信息
	HANDLE GetProcessHandle(int nprocess_id);

	//通过进程名（带后缀.exe）获取进程句柄
	HANDLE GetProcessHandle(LPCWSTR lpprocess_name);

	double FileTimeToDouble(FILETIME& filetime);

private:

};

#endif // GET_PROC_BASE_INFO_H_
