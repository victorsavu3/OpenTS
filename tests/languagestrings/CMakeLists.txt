# Exercises the generated language string table and Fetch_String's non-Windows path directly,
# against the same language.rc/language.h the generator itself reads. Only meaningful where the
# strings are compiled in rather than loaded from Language.dll.
if(OPENTS_EXPERIMENTAL_LINUX)
    opents_add_test(LanguageStrings
        NAME languagestrings
        SOURCES languagestringstest.cpp
        ENGINE
            data.cpp
            lcw.cpp
            load.cpp
            utf8.cpp
        INCLUDES ${OPENTS_GENERATED_DIR}
        STAMP
    )
endif()
