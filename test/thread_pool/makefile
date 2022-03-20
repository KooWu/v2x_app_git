target = test
objects = test_pthread_pool.o

CUR_DIR := $(shell pwd)

$(warning "make test_pthread_pool")
$(warning "cur dir is $(CUR_DIR)")


INCLUDE := -I $(CUR_DIR)
INCLUDE += -I $(CUR_DIR)/../../framework/thread_pool/
CFLAGS += -g
LDFLAGS += -L $(CUR_DIR)/../../framework/thread_pool/ -lmy_threadPool
LDFLAGS += -Wl,-rpath=$(CUR_DIR)/../../framework/thread_pool/
#LDFLAGS += -L $(CUR_DIR)/../../framework/log/ -lmy_log
#LDFLAGS += -Wl,-rpath=$(CUR_DIR)/../../framework/log/
#LDFLAGS += -L $(CUR_DIR)/../../opensource/cjson/lib/ -lcjson
#LDFLAGS += -Wl,-rpath=$(CUR_DIR)/../../opensource/cjson/lib/
LDFLAGS += -lpthread
CFLAGS += $(INCLUDE)

all: $(target) #make all或者make test都会执行7和9
obj: $(objects) #make obj会执行9

$(target):$(objects)
	gcc $^ -o $@ $(LDFLAGS) 
$(objects): %.o: %.c
	gcc -c $(CFLAGS) $< -o $@

.PHONY:clean cleanobj cleantarget#说明clean/cleanobj cleantarget是一个伪目标
clean: cleanobj cleantarget#说明-rm -rf -执行错误也无所谓
cleanobj:
	-rm -rf *.o
cleantarget:
	-rm -rf $(target)