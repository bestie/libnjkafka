# frozen_string_literal: true

require_relative "lib/libnjkafka/version"

Gem::Specification.new do |spec|
  spec.name = "libnjkafka"
  spec.version = Libnjkafka::VERSION
  spec.authors = ["Stephen Best"]
  spec.email = ["bestie@gmail.com"]

  spec.summary = "Ruby extension API for libnjkafka"
  spec.description = "Ruby extension API for libnjkafka"
  spec.homepage = "https://github.com/bestie/libnjkafka"
  spec.required_ruby_version = ">= 3.2.0"
  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = "https://github.com/bestie/libnjkafka"
  spec.metadata["changelog_uri"] = "https://github.com/bestie/libnjkafka/CHANGELOG.md"

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  gemspec = File.basename(__FILE__)
  spec.files = IO.popen(%w[git ls-files -z], chdir: __dir__, err: IO::NULL) do |ls|
    ls.readlines("\x0", chomp: true).reject do |f|
      (f == gemspec) ||
        f.start_with?(*%w[bin/ Gemfile .gitignore .rspec spec/ .github/])
    end
  end
  spec.bindir = "exe"
  spec.executables = spec.files.grep(%r{\Aexe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]

  spec.extensions = ["ext/extconf.rb"]
  # Uncomment to register a new dependency of your gem
  # spec.add_dependency "example-gem", "~> 1.0"

  # For more information and examples about making a new gem, check out our
  # guide at: https://bundler.io/guides/creating_gem.html
end
