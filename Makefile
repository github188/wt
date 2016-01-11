include Rules.mk

all:pub guide authenticate management acac
	@ctags -R *
	@cp ./run ~
	@echo "\(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/"

.PHONY:pub guide authenticate management acac probe stmp boss gac sguide sauthenticate

pub:
	@$(MAKE) -C$(PUBDIR)

guide:pub
	@$(MAKE) -C$(GUIDEDIR) WTDIR=$(WTDIR)

authenticate:pub
	@$(MAKE) -C$(AUTHENTICATEDIR) WTDIR=$(WTDIR)

management:pub
	@$(MAKE) -C$(MANAGEMENTDIR) WTDIR=$(WTDIR)

acac:pub
	@$(MAKE) -C$(ACACDIR) WTDIR=$(WTDIR)

probe:pub
	@$(MAKE) -C$(PROBEDIR) WTDIR=$(WTDIR)

stmp:pub
	@$(MAKE) -C$(STMPDIR) WTDIR=$(WTDIR)
	@ctags -R *

test:pub
	@$(MAKE) -C$(TESTDIR) WTDIR=$(WTDIR)

sall:boss gac sguide sauthenticate
	@ctags -R *
	@cp ./srun ~/run
	@echo "\(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/"

boss:pub
	@$(MAKE) -C$(BOSSDIR) WTDIR=$(WTDIR)
	@ctags -R *

gac:pub
	@$(MAKE) -C$(GACDIR) WTDIR=$(WTDIR)
	@ctags -R *

sguide:pub
	@$(MAKE) -C$(SGUIDEDIR) WTDIR=$(WTDIR)

sauthenticate:pub
	@$(MAKE) -C$(SAUTHENTICATEDIR) WTDIR=$(WTDIR)

gdb:gdb_pub gdb_guide gdb_authenticate gdb_management gdb_acac
	@ctags -R *
	@cp ./run ~
	@echo "\(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/"

.PHONY:gdb_pub gdb_guide gdb_authenticate gdb_management gdb_acac gdb_probe gdb_stmp gdb_boss gdb_gac gdb_sguide gdb_authenticate

gdb_pub:
	@$(MAKE) -C$(PUBDIR) GDB

gdb_guide:gdb_pub
	@$(MAKE) -C$(GUIDEDIR) WTDIR=$(WTDIR) GDB

gdb_authenticate:gdb_pub
	@$(MAKE) -C$(AUTHENTICATEDIR) WTDIR=$(WTDIR) GDB

gdb_management:gdb_pub
	@$(MAKE) -C$(MANAGEMENTDIR) WTDIR=$(WTDIR) GDB

gdb_acac:gdb_pub
	@$(MAKE) -C$(ACACDIR) WTDIR=$(WTDIR) GDB

gdb_probe:gdb_pub
	@$(MAKE) -C$(PROBEDIR) WTDIR=$(WTDIR) GDB

gdb_stmp:gdb_pub
	@$(MAKE) -C$(STMPDIR) WTDIR=$(WTDIR) GDB

sgdb:gdb_pub gdb_sguide gdb_sauthenticate gdb_boss gdb_gac
	@ctags -R *
#	@cp ./run ~
	@echo "\(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/ \(≧ ▽ ≦ )/"

gdb_boss:gdb_pub
	@$(MAKE) -C$(BOSSDIR) WTDIR=$(WTDIR) GDB

gdb_gac:gdb_pub
	@$(MAKE) -C$(GACDIR) WTDIR=$(WTDIR) GDB

gdb_sguide:gdb_pub
	@$(MAKE) -C$(SGUIDEDIR) WTDIR=$(WTDIR) GDB

gdb_sauthenticate:gdb_pub
	@$(MAKE) -C$(SAUTHENTICATEDIR) WTDIR=$(WTDIR) GDB

clean:
	@$(MAKE) -C$(PUBDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(GUIDEDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(AUTHENTICATEDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(MANAGEMENTDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(ACACDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(PROBEDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(STMPDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(BOSSDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(GACDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(SGUIDEDIR) clean WTDIR=$(WTDIR)
	@$(MAKE) -C$(SAUTHENTICATEDIR) clean WTDIR=$(WTDIR)
