CURRENT_PATH=$(shell pwd)
BITVECTOR_INCLUDE=${CURRENT_PATH}/lib/c-bitvector/include
VECTOR_INCLUDE=${CURRENT_PATH}/lib/c-vector/include
CIRCULAR_STACK_INCLUDE=${CURRENT_PATH}/lib/c-stack/include
K2TREE_INCLUDES=${CURRENT_PATH}/k2tree

INCLUDES=-I${BITVECTOR_INCLUDE} -I${K2TREE_INCLUDES} -I${VECTOR_INCLUDE} -I${CIRCULAR_STACK_INCLUDE}

CFLAGS :=  -Wall -Wextra -std=c99 -pedantic -Wmissing-prototypes -Wstrict-prototypes \
    -Wold-style-definition -Werror -O3


MAKE_FLAGS=INCLUDES="${INCLUDES}" CFLAGS="${CFLAGS}"

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
