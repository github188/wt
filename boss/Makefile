include $(WTDIR)/Rules.mk

#引导
BOSS_SOURCE=main.c guide.c server_mutual.c time_task.c list_guide.c
BOSS_DEPEND=main.o guide.o server_mutual.o time_task.o list_guide.o
BOSS_TARGET=boss.out

all:clean out mv
	@echo "(≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/"

out:
	@$(CC) -c $(BOSS_SOURCE) $(INCLUDE)
	@$(CC) $(BOSS_DEPEND) $(PUBLIC_DEPEND) -o $(BOSS_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/boss/$(BOSS_TARGET) $(BIN_DIR)/boss/backup.out
	@mv $(BOSS_TARGET) $(BIN_DIR)/boss/
	@cp $(WTDIR)/../boss.ini $(BIN_DIR)/boss/config.ini

GDB:clean gdb_out gdb_mv
	@echo "(≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/"

gdb_out:
	@$(CC) -g -c $(BOSS_SOURCE) $(INCLUDE)
	@$(CC) -g $(BOSS_DEPEND) $(PUBLIC_DEPEND) -o $(BOSS_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
