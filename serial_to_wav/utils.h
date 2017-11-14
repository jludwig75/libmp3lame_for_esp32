#pragma once


template<typename _T>
_T map(_T x, _T in_min, _T in_max, _T out_min, _T out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
