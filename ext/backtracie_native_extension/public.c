#include <stdint.h>

#include "ruby/ruby.h"
#include "ruby/debug.h"

#include "backtracie.h"
#include "backtracie_internal.h"
#include "ruby_shards.h"
#include "extconf.h"

struct backtracie_bt {
    uint32_t frames_capacity;
    uint32_t frames_count;
    raw_location frames[];
};


backtracie_bt_t backtracie_bt_capture() {
    return backtracie_bt_capture_for_thread(Qnil);
}

backtracie_bt_t backtracie_bt_capture_for_thread(VALUE thread) {
    int num_frames;
    if (RTEST(thread)) {
        num_frames = backtracie_rb_profile_frames_count_for_thread(thread);
    } else {
        num_frames = backtracie_rb_profile_frames_count();
    }

    size_t required_size = sizeof(struct backtracie_bt) + num_frames * sizeof(raw_location);
    backtracie_bt_t bt = ruby_xmalloc(required_size);
    bt->frames_capacity = num_frames;
    if (RTEST(thread)) {
        bt->frames_count = backtracie_rb_profile_frames_for_thread(
            thread, bt->frames_capacity, &bt->frames[0]
        );
    } else {
        bt->frames_count = backtracie_rb_profile_frames(
            bt->frames_capacity, &bt->frames[0]
        );
    }
    return bt;
}

void backtracie_bt_free(backtracie_bt_t bt) {
    ruby_xfree(bt);
}

void backtracie_bt_gc_mark(backtracie_bt_t bt) {
    for (uint32_t i = 0; i < bt->frames_count; i++) {
        if (bt->frames[i].iseq) {
            rb_gc_mark(bt->frames[i].iseq);
        }
        if (bt->frames[i].callable_method_entry) {
            rb_gc_mark(bt->frames[i].callable_method_entry);
        }
        rb_gc_mark(bt->frames[i].self);
    }
}

#ifdef HAVE_RB_GC_MARK_MOVABLE
void backtracie_bt_gc_mark_moveable(backtracie_bt_t bt) {
    for (uint32_t i = 0; i < bt->frames_count; i++) {
        if (bt->frames[i].iseq) {
            rb_gc_mark_movable(bt->frames[i].iseq);
        }
        if (bt->frames[i].callable_method_entry) {
            rb_gc_mark_movable(bt->frames[i].callable_method_entry);
        }
        rb_gc_mark_movable(bt->frames[i].self);
    }
}
#endif

BACKTRACIE_API void backtracie_bt_gc_mark_custom(backtracie_bt_t bt, void (*mark_fn)(VALUE, void*), void *ctx) {
    for (uint32_t i = 0; i < bt->frames_count; i++) {
        if (bt->frames[i].iseq) {
            mark_fn(bt->frames[i].iseq, ctx);
        }
        if (bt->frames[i].callable_method_entry) {
            mark_fn(bt->frames[i].callable_method_entry, ctx);
        }
        mark_fn(bt->frames[i].self, ctx);
        // The CME is not a Ruby object that needs marking. Keeping self live also keeps the CME live.
        // The original_id is not used anywhere by the public functions, so needs no marking
    }
}

#ifdef HAVE_RB_GC_LOCATION
void backtracie_bt_gc_compact(backtracie_bt_t bt) {
    for (uint32_t i = 0; i < bt->frames_count; i++) {
        if (bt->frames[i].iseq) {
            bt->frames[i].iseq = rb_gc_location(bt->frames[i].iseq);
        }
        if (bt->frames[i].callable_method_entry) {
            bt->frames[i].callable_method_entry = rb_gc_location(bt->frames[i].callable_method_entry);
        }
        bt->frames[i].self = rb_gc_location(bt->frames[i].self);
    }
}
#endif

size_t backtracie_bt_memsize(backtracie_bt_t bt) {
    return sizeof(struct backtracie_bt) + bt->frames_capacity * sizeof(raw_location);
}

uint32_t backtracie_bt_get_frames_count(backtracie_bt_t bt) {
    return bt->frames_count;
}

VALUE backtracie_bt_get_frame_value(backtracie_bt_t bt, uint32_t ix) {
    return frame_from_location(&bt->frames[ix]);
}

VALUE backtracie_bt_get_frame_method_name(backtracie_bt_t bt, uint32_t ix) {
    if (bt->frames[ix].is_ruby_frame) {
        return qualified_method_name_for_location(&bt->frames[ix]);
    } else {
        return backtracie_rb_profile_frame_qualified_method_name(bt->frames[ix].callable_method_entry);
    }
}

VALUE backtracie_bt_get_frame_file_name(backtracie_bt_t bt, uint32_t ix) {
    if (bt->frames[ix].is_ruby_frame) {
        return rb_profile_frame_path(frame_from_location(&bt->frames[ix]));
    } else {
        // Ruby gives us nothing here.
        return Qnil;
    }
}

VALUE backtracie_bt_get_frame_line_number(backtracie_bt_t bt, uint32_t ix) {
    if (bt->frames[ix].is_ruby_frame) {
        return INT2NUM(bt->frames[ix].line_number);
    } else {
        return Qnil;
    }
}

