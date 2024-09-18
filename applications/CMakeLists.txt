if (CERLIB_BUILD_TESTBED)
    if (ANDROID)
        message(FATAL_ERROR "Testbed cannot be built for Android! Open an AndroidProject-based folder using Android Studio instead.")
    endif ()

    add_subdirectory(testbed)
endif ()

if (CERLIB_BUILD_PLATFORMER_DEMO)
    if (ANDROID)
        message(FATAL_ERROR "Platformer cannot be built for Android! Open an AndroidProject-based folder using Android Studio instead.")
    endif ()

    add_subdirectory(platformer)
endif ()
