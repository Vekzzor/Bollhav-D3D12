#include "pch.h"

inline char* HrToString(HRESULT hr)
{
	_com_error err(hr);
	char s_str[64] = {};
	sprintf_s(s_str, "%s", err.ErrorMessage());
	return s_str;
}

inline void ThrowIfFailed(HRESULT hr)
{
#if _DEBUG
	if(FAILED(hr))
	{
		throw std::runtime_error(HrToString(hr));
	}
#endif
}
