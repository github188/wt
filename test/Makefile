include $(WTDIR)/Rules.mk

#认证
TEST_SOURCE=main.c
TEST_DEPEND=main.o
TEST_TARGET=test.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(TEST_SOURCE) $(INCLUDE)
	@$(CC) $(TEST_DEPEND) $(PUBLIC_DEPEND) -o $(TEST_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/test/$(TEST_TARGET) $(BIN_DIR)/test/backup.out 
	@mv $(TEST_TARGET) $(BIN_DIR)/test/
	@cp $(WTDIR)/../config.ini $(BIN_DIR)/test/

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(TEST_SOURCE) $(INCLUDE)
	@$(CC) -g $(TEST_DEPEND) $(PUBLIC_DEPEND) -o $(TEST_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
