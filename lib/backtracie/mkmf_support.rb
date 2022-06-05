# frozen_string_literal: true

def compile_with_backtracie!
    $CFLAGS << " -I#{File.realpath(File.join(__dir__, '../../ext/backtracie_native_extension/include'))} "
    have_func("rb_gc_mark_movable", ["ruby.h"])
    have_func("rb_gc_location", ["ruby.h"])
end
