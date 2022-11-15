/* percent = (user_time_diff + kernel_time_diff) * 100 / (cpu_num * system_time_diff) */
#include <stdio.h>
#include <Windows.h>
#include "CGetProcBaseInfo.h"


int main() {

	CGetProcBaseInfo info;

	for (int i = 0; i < 10000; i++) {

		printf("QQ CPU usage is : %lf \n", info.CalculateCpuUsag(L"QQ.exe"));

	}

	return 0;
}
