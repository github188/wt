include $(WTDIR)/Rules.mk

#引导
GUIDE_SOURCE=main.c router_guide.c server_mutual.c time_task.c ss.c rts.c
GUIDE_DEPEND=main.o router_guide.o server_mutual.o time_task.o ss.o rts.o
GUIDE_TARGET=guide.out

all:clean out mv
	@echo "(≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/"

out:
	@$(CC) -c $(GUIDE_SOURCE) $(INCLUDE)
	@$(CC) $(GUIDE_DEPEND) $(PUBLIC_DEPEND) -o $(GUIDE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/guide/$(GUIDE_TARGET) $(BIN_DIR)/guide/backup.out
	@mv $(GUIDE_TARGET) $(BIN_DIR)/guide/
	@cp $(WTDIR)/../config.ini $(BIN_DIR)/guide/

GDB:clean gdb_out gdb_mv
	@echo "(≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/"

gdb_out:
	@$(CC) -g -c $(GUIDE_SOURCE) $(INCLUDE)
	@$(CC) -g $(GUIDE_DEPEND) $(PUBLIC_DEPEND) -o $(GUIDE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
