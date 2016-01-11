include $(WTDIR)/Rules.mk

#集中管理
MANAGEMENT_SOURCE=main.c server_mutual.c router.c
MANAGEMENT_DEPEND=main.o server_mutual.o router.o
MANAGEMENT_TARGET=management.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(MANAGEMENT_SOURCE) $(INCLUDE)
	@$(CC) $(MANAGEMENT_DEPEND) $(PUBLIC_DEPEND) -o $(MANAGEMENT_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/management/$(MANAGEMENT_TARGET) $(BIN_DIR)/management/backup.out 
	@mv $(MANAGEMENT_TARGET) $(BIN_DIR)/management/
	@cp $(WTDIR)/../config.ini $(BIN_DIR)/management/

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(MANAGEMENT_SOURCE) $(INCLUDE)
	@$(CC) -g $(MANAGEMENT_DEPEND) $(PUBLIC_DEPEND) -o $(MANAGEMENT_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
