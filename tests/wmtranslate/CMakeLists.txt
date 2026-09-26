# Pins the SDL event -> (message, wParam, lParam) translation sdlstub.cpp provides in place of
# winstub.cpp's own Win32 message pump, so it exists only on the experimental Linux target.
if(NOT WIN32)
    opents_add_test(WmTranslate
        NAME wmtranslate
        SOURCES wmtranslatetest.cpp
        ENGINE
            sdltranslate.cpp
        LIBRARIES SDL3::SDL3
    )
endif()
