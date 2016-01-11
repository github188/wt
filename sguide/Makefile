include $(WTDIR)/Rules.mk

#引导
SGUIDE_SOURCE=main.c router_guide.c
SGUIDE_DEPEND=main.o router_guide.o
SGUIDE_TARGET=sguide.out

all:clean out mv
	@echo "(≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/"

out:
	@$(CC) -c $(SGUIDE_SOURCE) $(INCLUDE)
	@$(CC) $(SGUIDE_DEPEND) $(PUBLIC_DEPEND) -o $(SGUIDE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/sguide/$(SGUIDE_TARGET) $(BIN_DIR)/sguide/backup.out
	@mv $(SGUIDE_TARGET) $(BIN_DIR)/sguide/
	@cp $(WTDIR)/../boss.ini $(BIN_DIR)/sguide/config.ini

GDB:clean gdb_out gdb_mv
	@echo "(≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/ (≧ ▽ ≦ )/"

gdb_out:
	@$(CC) -g -c $(SGUIDE_SOURCE) $(INCLUDE)
	@$(CC) -g $(SGUIDE_DEPEND) $(PUBLIC_DEPEND) -o $(SGUIDE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
