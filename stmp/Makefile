include $(WTDIR)/Rules.mk

#STMP
STMP_SOURCE=main.c base64.c http.c email.c
STMP_DEPEND=main.o base64.o http.o email.o
STMP_TARGET=stmp.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(STMP_SOURCE) $(INCLUDE)
	@$(CC) $(STMP_DEPEND) $(PUBLIC_DEPEND) -o $(STMP_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/stmp/$(STMP_TARGET) $(BIN_DIR)/stmp/backup.out 
	@mv $(STMP_TARGET) $(BIN_DIR)/stmp/
	@cp $(WTDIR)/../mail.ini $(BIN_DIR)/stmp/config.ini

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(STMP_SOURCE) $(INCLUDE)
	@$(CC) -g $(STMP_DEPEND) $(PUBLIC_DEPEND) -o $(STMP_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o
