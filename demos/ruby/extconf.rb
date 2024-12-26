require 'mkmf'


include_dir = File.expand_path(ENV.fetch("LIB_DIR"))
lib_dir = include_dir

dir_config('libnjkafka', include_dir, lib_dir)

$CFLAGS << " -I#{include_dir} -g"

unless have_header('libnjkafka.h')
  abort "libnjkafka.h is missing. Please ensure it is located in `#{include_dir}` and try again."
end

unless have_library('njkafka')
  abort "libnjkafka is missing. Please ensure it is located in `#{lib_dir}` and try again."
end

create_makefile('libnjkafka_ext')
