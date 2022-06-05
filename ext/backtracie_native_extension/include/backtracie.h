#ifndef __BACKTRACIE_H
#define __BACKTRACIE_H

#if defined(_WIN32) || defined(_WIN64)
#  ifdef BACKTRACIE_EXPORTS
#    define BACKTRACIE_API __declspec(dllexport)
#  else
#    define BACKTRACIE_API __declspec(dllimport)
#  endif
#else
#  define BACKTRACIE_API __attribute__((visibility("default")))
#endif

// Forward-declare the structure type - consuming code outside this gem
// should not have knowledge of its fields.
struct backtracie_bt;
typedef struct backtracie_bt *backtracie_bt_t;

// Captures a backtrace for the current Ruby thread.
// Allocates memory with ruby_xmalloc().
BACKTRACIE_API backtracie_bt_t backtracie_bt_capture();
// Same as backtracie_bt_capture, but for the specified thread.
BACKTRACIE_API backtracie_bt_t backtracie_bt_capture_for_thread(VALUE thread);
BACKTRACIE_API void backtracie_bt_gc_mark(backtracie_bt_t bt);
#ifdef HAVE_RB_GC_MARK_MOVABLE
BACKTRACIE_API void backtracie_bt_gc_mark_moveable(backtracie_bt_t bt);
#endif
#ifdef HAVE_RB_GC_LOCATION
BACKTRACIE_API void backtracie_bt_gc_compact(backtracie_bt_t bt);
#endif
BACKTRACIE_API void backtracie_bt_free(backtracie_bt_t bt);
BACKTRACIE_API size_t backtracie_bt_memsize(backtracie_bt_t bt);
BACKTRACIE_API uint32_t backtracie_bt_get_frames_count(backtracie_bt_t bt);
BACKTRACIE_API VALUE backtracie_bt_get_frame_value(backtracie_bt_t bt, uint32_t ix);
BACKTRACIE_API VALUE backtracie_bt_get_frame_value(backtracie_bt_t bt, uint32_t ix);
BACKTRACIE_API VALUE backtracie_bt_get_frame_method_name(backtracie_bt_t bt, uint32_t ix);
BACKTRACIE_API VALUE backtracie_bt_get_frame_file_name(backtracie_bt_t bt, uint32_t ix);
BACKTRACIE_API VALUE backtracie_bt_get_frame_line_number(backtracie_bt_t bt, uint32_t ix);

#endif
