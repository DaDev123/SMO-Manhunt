# TODO (Khangaroo): Make this process a lot less hacky (no, export did not work)
# See MakefileNSO

.PHONY: all clean starlight send exefs emu log sendlog

GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
GIT_COMMIT := $(shell git rev-parse --short HEAD)
GIT_REV    := ${GIT_BRANCH}-${GIT_COMMIT}
ifneq (,$(strip $(shell git status --porcelain 2>/dev/null)))
GIT_REV := $(GIT_REV)-dirty
endif

SMOVER ?= 100
BUILDVER ?= 101
BUILDVERSTR ?= $(GIT_REV)
IP ?= 10.0.0.221 # ftp server ip (usually is switch's local IP)
DEBUGLOG ?= 0 # defaults to disable debug logger 
SERVERIP ?= 0.0.0.0 # put debug logger server IP here
ISEMU ?= 0 # set to 1 to compile for emulators

PROJNAME ?= StarlightBase

all: starlight

starlight:
	$(MAKE) all -f MakefileNSO SMOVER=$(SMOVER) BUILDVERSTR=$(BUILDVERSTR) BUILDVER=$(BUILDVER) DEBUGLOG=$(DEBUGLOG) SERVERIP=${SERVERIP} EMU=${ISEMU}
	$(MAKE) starlight_patch_$(SMOVER)/*.ips

	mkdir -p starlight_patch_$(SMOVER)/atmosphere/exefs_patches/$(PROJNAME)/
	mkdir -p starlight_patch_$(SMOVER)/atmosphere/contents/0100000000010000/exefs/

	mv starlight_patch_$(SMOVER)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips starlight_patch_$(SMOVER)/atmosphere/exefs_patches/$(PROJNAME)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips
	mv $(shell basename $(CURDIR))$(SMOVER).elf starlight_patch_$(SMOVER)/subsdk1.elf
	mv $(shell basename $(CURDIR))$(SMOVER).nso starlight_patch_$(SMOVER)/atmosphere/contents/0100000000010000/exefs/subsdk1

	cp -R romfs starlight_patch_$(SMOVER)/atmosphere/contents/0100000000010000
	
exefs:
	$(MAKE) all -f MakefileNSO SMOVER=$(SMOVER) BUILDVERSTR=$(BUILDVERSTR) BUILDVER=$(BUILDVER) DEBUGLOG=$(DEBUGLOG) SERVERIP=${SERVERIP} EMU=${ISEMU}
	$(MAKE) starlight_patch_$(SMOVER)/*.ips

	mkdir -p exefs/atmosphere/exefs_patches/$(PROJNAME)/
	mkdir -p exefs/atmosphere/contents/0100000000010000/exefs/

	mv starlight_patch_$(SMOVER)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips exefs/atmosphere/exefs_patches/$(PROJNAME)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips
	mv $(shell basename $(CURDIR))$(SMOVER).elf exefs/subsdk1.elf
	mv $(shell basename $(CURDIR))$(SMOVER).nso exefs/atmosphere/contents/0100000000010000/exefs/subsdk1

starlight_patch_$(SMOVER)/*.ips: patches/*.slpatch patches/configs/$(SMOVER).config patches/maps/$(SMOVER)/*.map \
								build$(SMOVER)/$(shell basename $(CURDIR))$(SMOVER).map scripts/genPatch.py
	@rm -f starlight_patch_$(SMOVER)/*.ips
	python3 scripts/genPatch.py $(SMOVER)

emu:
	$(MAKE) all -f MakefileNSO SMOVER=$(SMOVER) BUILDVERSTR=$(BUILDVERSTR) BUILDVER=$(BUILDVER) EMU=1
	$(MAKE) starlight_patch_$(SMOVER)/*.ips

	mkdir -p starlight_patch_$(SMOVER)/SMOManHunt-Emulator/exefs/

	mv starlight_patch_$(SMOVER)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips starlight_patch_$(SMOVER)/SMOManHunt-Emulator/exefs/3CA12DFAAF9C82DA064D1698DF79CDA1.ips
	mv $(shell basename $(CURDIR))$(SMOVER).elf starlight_patch_$(SMOVER)/subsdk1.elf
	mv $(shell basename $(CURDIR))$(SMOVER).nso starlight_patch_$(SMOVER)/SMOManHunt-Emulator/exefs/subsdk1

	cp -R romfs starlight_patch_$(SMOVER)/SMOManHunt-Emulator/

send: all
	python3 scripts/sendPatch.py $(IP) $(PROJNAME)

log: all
	python3 scripts/tcpServer.py $(SERVERIP)

sendlog: all
	python3 scripts/sendPatch.py $(IP) $(PROJNAME) $(USER) $(PASS)
	python3 scripts/tcpServer.py $(SERVERIP)

clean:
	$(MAKE) clean -f MakefileNSO
	@rm -fr starlight_patch_*
