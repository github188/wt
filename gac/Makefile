include $(WTDIR)/Rules.mk

#集中管理
GAC_SOURCE=main.c router.c
GAC_DEPEND=main.o router.o
GAC_TARGET=gac.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(GAC_SOURCE) $(INCLUDE)
	@$(CC) $(GAC_DEPEND) $(PUBLIC_DEPEND) -o $(GAC_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/gac/$(GAC_TARGET) $(BIN_DIR)/gac/backup.out 
	@mv $(GAC_TARGET) $(BIN_DIR)/gac/
	@cp $(WTDIR)/../boss.ini $(BIN_DIR)/gac/config.ini

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(GAC_SOURCE) $(INCLUDE)
	@$(CC) -g $(GAC_DEPEND) $(PUBLIC_DEPEND) -o $(GAC_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
