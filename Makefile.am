SUBDIRS = src bin tests
EXTRA_DIST = scripts/demo_v2.sh

.PHONY: asan-check
asan-check:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
		LDFLAGS="$(LDFLAGS) -fsanitize=address,undefined" \
		check
ACLOCAL_AMFLAGS = -I m4
