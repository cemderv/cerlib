// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <android/asset_manager_jni.h>
#include <cerlib/Export.hpp>

namespace cer::details {
  CERLIB_API void set_android_asset_manager(void* asset_manager);
}

#define CERLIB_ANDROID_DEFINE_JNI_FUNCTIONS(company_name, game_name)                                                   \
  extern "C" {                                                                                                         \
  JNIEXPORT void JNICALL Java_com_##company_name##_##game_name##_MainActivity_setAssetManager(JNIEnv* env,             \
                                                                                              jclass  obj,             \
                                                                                              jobject asset_manager) { \
    const auto native_asset_manager = AAssetManager_fromJava(env, asset_manager);                                      \
    cer::details::set_android_asset_manager(native_asset_manager);                                                     \
  }                                                                                                                    \
  }