// backtracist: Ruby gem for beautiful backtraces
// Copyright (C) 2021 Ivo Anjo <ivo@ivoanjo.me>
// 
// This file is part of backtracist.
// 
// backtracist is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// backtracist is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with backtracist.  If not, see <http://www.gnu.org/licenses/>.

#include "ruby/ruby.h"
#include "ruby/debug.h"

#include "extconf.h"

#include "ruby_3.0.0.h"

// Constants

#define MAX_STACK_DEPTH 2000 // FIXME: Need to handle when this is not enough

// Globals

static VALUE backtracist_location_class = Qnil;

// Headers

static VALUE primitive_caller_locations(VALUE self);
inline static VALUE new_location(VALUE absolute_path, VALUE base_label, VALUE label, VALUE lineno, VALUE path, VALUE debug);
static bool is_ruby_frame(VALUE ruby_frame);
static VALUE ruby_frame_to_location(VALUE frame, VALUE last_ruby_line);
static VALUE cfunc_frame_to_location(VALUE frame, VALUE last_ruby_frame, VALUE last_ruby_line);
static VALUE debug_frame(VALUE frame);

// Macros

#define VALUE_COUNT(array) (sizeof(array) / sizeof(VALUE))

void Init_backtracist_native_extension(void) {
  VALUE backtracist_module = rb_const_get(rb_cObject, rb_intern("Backtracist"));

  // We need to keep a reference to Backtracist::Locations around, to create new instances
  backtracist_location_class = rb_const_get(backtracist_module, rb_intern("Location"));
  rb_global_variable(&backtracist_location_class);

  VALUE backtracist_primitive_module = rb_define_module_under(backtracist_module, "Primitive");

  rb_define_module_function(backtracist_primitive_module, "caller_locations", primitive_caller_locations, 0);
}

static VALUE primitive_caller_locations(VALUE self) {
  int stack_depth = 0;
  VALUE frames[MAX_STACK_DEPTH];
  int lines[MAX_STACK_DEPTH];

  stack_depth = modified_rb_profile_frames(0, MAX_STACK_DEPTH, frames, lines);

  // Ignore:
  // * the current stack frame (native)
  // * the Backtracist.caller_locations that called us
  // * the frame from the caller itself (since we're replicating the semantics of Kernel#caller_locations)
  int ignored_stack_top_frames = 3;
  // Ignore the last frame -- seems to be an uninteresting VM frame. MRI itself seems to ignore the last frame in
  // the implementation of backtrace_collect()
  int ignored_stack_bottom_frames = 1;

  stack_depth -= ignored_stack_bottom_frames;

  VALUE locations = rb_ary_new_capa(stack_depth - ignored_stack_top_frames);

  // MRI does not give us the path or line number for frames implemented using C code. The convention in
  // Kernel#caller_locations is to instead use the path and line number of the last Ruby frame seen.
  // Thus, we keep that frame here to able to replicate that behavior.
  // (This is why we also iterate the frames array backwards below -- so that it's easier to keep the last_ruby_frame)
  VALUE last_ruby_frame = Qnil;
  VALUE last_ruby_line = Qnil;

  for (int i = stack_depth - 1; i >= ignored_stack_top_frames; i--) {
    VALUE frame = frames[i];
    int line = lines[i];

    VALUE location = Qnil;

    if (is_ruby_frame(frame)) {
      last_ruby_frame = frame;
      last_ruby_line = INT2FIX(line);

      location = ruby_frame_to_location(frame, last_ruby_line);
    } else {
      location = cfunc_frame_to_location(frame, last_ruby_frame, last_ruby_line);
    }

    rb_ary_store(locations, i - ignored_stack_top_frames, location);
  }

  return locations;
}

inline static VALUE new_location(VALUE absolute_path, VALUE base_label, VALUE label, VALUE lineno, VALUE path, VALUE debug) {
  VALUE arguments[] = { absolute_path, base_label, label, lineno, path, debug };
  return rb_class_new_instance(VALUE_COUNT(arguments), arguments, backtracist_location_class);
}

static bool is_ruby_frame(VALUE frame) {
  VALUE absolute_path = rb_profile_frame_absolute_path(frame);

  return absolute_path != Qnil &&
    (rb_funcall(absolute_path, rb_intern("=="), 1, rb_str_new2("<cfunc>")) == Qfalse);
}

static VALUE ruby_frame_to_location(VALUE frame, VALUE last_ruby_line) {
  return new_location(
    rb_profile_frame_absolute_path(frame),
    rb_profile_frame_base_label(frame),
    rb_profile_frame_label(frame),
    last_ruby_line,
    rb_profile_frame_path(frame),
    debug_frame(frame)
  );
}

static VALUE cfunc_frame_to_location(VALUE frame, VALUE last_ruby_frame, VALUE last_ruby_line) {
  VALUE method_name = rb_profile_frame_method_name(frame); // Replaces label and base_label in cfuncs

  return new_location(
    last_ruby_frame != Qnil ? rb_profile_frame_absolute_path(last_ruby_frame) : Qnil,
    method_name,
    method_name,
    last_ruby_line,
    last_ruby_frame != Qnil ? rb_profile_frame_path(last_ruby_frame) : Qnil,
    debug_frame(frame)
  );
}

// Used to dump all the things we get from the rb_profile_frames API, for debugging
static VALUE debug_frame(VALUE frame) {
  VALUE arguments[] = {
    rb_profile_frame_path(frame),
    rb_profile_frame_absolute_path(frame),
    rb_profile_frame_label(frame),
    rb_profile_frame_base_label(frame),
    rb_profile_frame_full_label(frame),
    rb_profile_frame_first_lineno(frame),
    rb_profile_frame_classpath(frame),
    rb_profile_frame_singleton_method_p(frame),
    rb_profile_frame_method_name(frame),
    rb_profile_frame_qualified_method_name(frame)
  };
  return rb_ary_new_from_values(VALUE_COUNT(arguments), arguments);
}
