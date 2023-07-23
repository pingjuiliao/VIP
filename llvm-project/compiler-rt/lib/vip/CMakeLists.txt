include_directories(..)

set(VIP_RTL_SOURCES
  vip.cpp
  )

set(VIP_RTL_HEADERS
  vip.h
  )

set(VIP_COMMON_CFLAGS ${SANITIZER_COMMON_CFLAGS})

add_compiler_rt_component(vip)

foreach(arch ${VIP_SUPPORTED_ARCH})
  set(VIP_CFLAGS ${VIP_COMMON_CFLAGS})
  add_compiler_rt_runtime(clang_rt.vip
    STATIC
    ARCHS ${arch}
    SOURCES ${VIP_RTL_SOURCES}
            $<TARGET_OBJECTS:RTInterception.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommon.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommonLibc.${arch}>
    ADDITIONAL_HEADERS ${VIP_RTL_HEADERS}
    CFLAGS ${VIP_CFLAGS}
    PARENT_TARGET vip)
  add_sanitizer_rt_symbols(clang_rt.vip
    ARCHS ${arch})
endforeach()
