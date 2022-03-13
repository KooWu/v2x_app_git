target = test
objects = test_log.o

CUR_DIR := $(shell pwd)

$(warning "make liblog.so")
$(warning "cur dir is $(CUR_DIR)")


INCLUDE := -I $(CUR_DIR)
INCLUDE += -I $(CUR_DIR)/../../framework/log/
CFLAGS += -g
LDFLAGS += -lpthread
LDFLAGS += -L $(CUR_DIR)/../../framework/log/ -lmy_log
LDFLAGS += -Wl,-rpath=$(CUR_DIR)/../../framework/log/
LDFLAGS += -L $(CUR_DIR)/../../opensource/cjson/lib/ -lcjson
LDFLAGS += -Wl,-rpath=$(CUR_DIR)/../../opensource/cjson/lib/
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