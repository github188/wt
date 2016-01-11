SOURCE=utils.c sql.c net.c logs.c ini.c pool.c cJSON.c ac_utils.c encode.c order.c

.PHONY:all clean obj

all:clean obj
	@echo "(≧ ▽ ≦ ) (≧ ▽ ≦ ) (≧ ▽ ≦ ) (≧ ▽ ≦ )"

obj:
	@$(CC) -c -lm $(SOURCE)

GDB:clean gdb_obj
	@echo "(≧ ▽ ≦ ) (≧ ▽ ≦ ) (≧ ▽ ≦ ) (≧ ▽ ≦ )"

gdb_obj:
	@$(CC) -g -c $(SOURCE)

clean:
	@rm -f *.o
