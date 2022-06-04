# frozen_string_literal: true

def compile_with_backtracie!
    $CFLAGS << " -I#{File.realpath(File.join(__dir__, '../../ext/backtracie_native_extension'))} "
end
