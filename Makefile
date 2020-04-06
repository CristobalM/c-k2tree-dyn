CURRENT_PATH=$(shell pwd)
BITVECTOR_INCLUDE=${CURRENT_PATH}/lib/c-bitvector/include

INCLUDES=-I${BITVECTOR_INCLUDE}
MAKE_FLAGS=INCLUDES="${INCLUDES}"

MODULES_DIRS := k2tree



all: fetch_deps format modules

re: clean all

fetch_deps:
	chmod a+x fetch_deps.sh && bash fetch_deps.sh

clean:
	for dir in ${MODULES_DIRS}; do \
		$(MAKE) clean -C $$dir ${MAKE_FLAGS};  \
	done

modules:
	for dir in ${MODULES_DIRS}; do \
		$(MAKE) -C $$dir ${MAKE_FLAGS}; \
	done


format:
	find . -path ./lib -prune -o -regex '.*\.\(c\|h\)' -exec clang-format -style=file -i {} \;
