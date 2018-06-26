set (MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 0)
set (MONGOSQL_AUTH_ENABLE_SASL_SSPI 0)

IF(APPLE)
    MESSAGE(STATUS "Using GSSAPI for mongoc kerberos")
    set(MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 1)
    set(MONGO_KRB_LIBS "-framework GSS")
ELSEIF(WIN32)
    MESSAGE(STATUS "Using SSPI for mongoc kerberos")
    set(MONGOSQL_AUTH_ENABLE_SASL_SSPI 1)
    set(MONGO_KRB_LIBS secur32.lib Shlwapi.lib)
ELSE()
    MESSAGE(STATUS "Using GSSAPI for mongoc kerberos")
    set(MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 1)
    include(FindGSSAPI)
    if (GSS_FOUND)
        set(MONGO_KRB_LIBS ${GSS_LIBS})
        include_directories(${GSS_INCLUDE_DIRS})
        MESSAGE(STATUS "Found GSS libraries")
        MESSAGE(STATUS "GSSAPI include path: ${GSS_INCLUDE_DIRS}")
    else()
        MESSAGE(FATAL_ERROR "Could not find GSS libraries")
    endif()
ENDIF()

MESSAGE(STATUS "Using kerberos library: ${MONGO_KRB_LIBS}")
set(MONGOSQL_AUTH_ENABLE_SASL 1)
