
#ifndef POINTCLOUD_EXPORT_H
#define POINTCLOUD_EXPORT_H

#ifdef POINTCLOUD_STATIC_DEFINE
#  define POINTCLOUD_EXPORT
#  define POINTCLOUD_NO_EXPORT
#else
#  ifndef POINTCLOUD_EXPORT
#    if defined(_MSC_VER)
#      ifdef pointcloud_EXPORTS
#        define POINTCLOUD_EXPORT __declspec(dllexport)
#      else
#        define POINTCLOUD_EXPORT __declspec(dllimport)
#      endif
#    else
#      define POINTCLOUD_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef POINTCLOUD_NO_EXPORT
#    if defined(_MSC_VER)
#      define POINTCLOUD_NO_EXPORT
#    else
#      define POINTCLOUD_NO_EXPORT __attribute__((visibility("hidden")))
#    endif
#  endif
#endif

#ifndef POINTCLOUD_DEPRECATED
#  define POINTCLOUD_DEPRECATED 
#endif

#ifndef POINTCLOUD_DEPRECATED_EXPORT
#  define POINTCLOUD_DEPRECATED_EXPORT POINTCLOUD_EXPORT POINTCLOUD_DEPRECATED
#endif

#ifndef POINTCLOUD_DEPRECATED_NO_EXPORT
#  define POINTCLOUD_DEPRECATED_NO_EXPORT POINTCLOUD_NO_EXPORT POINTCLOUD_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef POINTCLOUD_NO_DEPRECATED
#    define POINTCLOUD_NO_DEPRECATED
#  endif
#endif

#endif
