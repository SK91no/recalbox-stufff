//
// Created by bkg2k on 29/10/24.
//
#pragma once

#include <concepts>

template<class T>
concept IsPod = ((std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>) || std::is_void_v<T>);

template<class T>
concept IsNotPod = (!(std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>) && !std::is_void_v<T>);
