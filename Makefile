ZIG ?= zig
LLVM_NM ?= llvm-nm
LLVM_OBJCOPY ?= llvm-objcopy
LLVM_OBJDUMP ?= llvm-objdump
CLANG_FORMAT ?= clang-format

# The win7 targets set _WIN32_WINNT to 0x0601.
TARGET_x64 := x86_64-windows.win7-gnu
TARGET_x86 := x86-windows.win7-gnu

WARNINGS := -Wall -Wextra -Wpedantic -Werror
BOF_CFLAGS := -Oz -g0 -std=c11 $(WARNINGS) -DNDEBUG -fno-addrsig \
	-Wno-dollar-in-identifier-extension -fno-builtin \
	-fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables \
	-fno-ident -fno-jump-tables
CPPFLAGS := -Iinclude -Isrc/common -Isrc/techniques
OUTPUT_DIR := integrations/cna
SLIVER_OUTPUT_DIR := integrations/sliver
BOFLINT ?= boflint.py
VERSION ?=

TECHNIQUES := wmiprovider wmieventsubscription wmiservice wmiexec wsmanexec taskschedexec dcomishelldispatch2 scmchangeservice scmcreateservice scmcommand
OUTPUTS := $(foreach technique,$(TECHNIQUES),$(OUTPUT_DIR)/rex_$(technique).x64.o $(OUTPUT_DIR)/rex_$(technique).x86.o)
SLIVER_OUTPUTS := $(foreach technique,$(TECHNIQUES),$(SLIVER_OUTPUT_DIR)/rex_$(technique).x64.o $(SLIVER_OUTPUT_DIR)/rex_$(technique).x86.o)

BASE_SOURCES := src/common/rex.c src/common/registry.c src/common/result.c src/common/runtime.c src/common/text.c
COM_SOURCES := src/common/com.c
WMI_SOURCES := src/common/wmi.c
SERVICE_SOURCES := src/common/service.c
HEADERS := $(wildcard include/*.h src/common/*.h src/techniques/*.h)
FORMAT_SOURCES := $(wildcard include/*.h src/common/*.c src/common/*.h \
	src/techniques/*.c src/techniques/*.h tests/*.c) src/technique_unit.c

wmiprovider_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) $(WMI_SOURCES) src/techniques/wmiprovider.c
wmiprovider_DEFINES := -DREX_WITH_COM_SECURITY -DREX_WITH_WMI
wmieventsubscription_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) $(WMI_SOURCES) src/techniques/wmieventsubscription.c
wmieventsubscription_DEFINES := -DREX_WITH_COM_SECURITY -DREX_WITH_WMI
wmiservice_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) $(WMI_SOURCES) $(SERVICE_SOURCES) src/techniques/wmiservice.c
wmiservice_DEFINES := -DREX_WITH_COM_SECURITY -DREX_WITH_WMI -DREX_WITH_SERVICE
wmiexec_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) $(WMI_SOURCES) src/techniques/wmiexec.c
wmiexec_DEFINES := -DREX_WITH_COM_SECURITY -DREX_WITH_WMI
wsmanexec_SOURCES := $(BASE_SOURCES) src/techniques/wsmanexec.c
taskschedexec_SOURCES := $(BASE_SOURCES) src/techniques/taskschedexec.c
dcomishelldispatch2_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) src/techniques/dcomishelldispatch2.c
dcomishelldispatch2_DEFINES := -DREX_WITH_COM_SECURITY
dcommmc20_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) src/techniques/dcommmc20.c
dcommmc20_DEFINES := -DREX_WITH_COM_SECURITY
dcomshellbrowserwindow_SOURCES := $(BASE_SOURCES) $(COM_SOURCES) src/techniques/dcomshellbrowserwindow.c
dcomshellbrowserwindow_DEFINES := -DREX_WITH_COM_SECURITY
scmchangeservice_SOURCES := $(BASE_SOURCES) $(SERVICE_SOURCES) src/techniques/scmchangeservice.c
scmchangeservice_DEFINES := -DREX_WITH_SERVICE
scmcreateservice_SOURCES := $(BASE_SOURCES) $(SERVICE_SOURCES) src/techniques/scmcreateservice.c
scmcreateservice_DEFINES := -DREX_WITH_SERVICE
scmcommand_SOURCES := $(BASE_SOURCES) $(SERVICE_SOURCES) src/techniques/scmcommand.c
scmcommand_DEFINES := -DREX_WITH_SERVICE

.PHONY: all clean demo docs format format-check imports lint objects release \
	sliver-entrypoints sliver-manifest technique-contract verify verify-bof

all: $(OUTPUTS) $(SLIVER_OUTPUTS)

verify: clean lint all objects docs technique-contract sliver-manifest sliver-entrypoints

lint: format-check
	cppcheck --quiet --error-exitcode=1 --std=c11 --inline-suppr \
		--enable=warning,portability,performance \
		--suppress=missingIncludeSystem -Iinclude -Isrc/common \
		-Isrc/techniques src tests
	shellcheck --severity=warning scripts/*.sh
	actionlint

format:
	$(CLANG_FORMAT) -i $(FORMAT_SOURCES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SOURCES)

objects: all
	LLVM_NM="$(LLVM_NM)" LLVM_OBJDUMP="$(LLVM_OBJDUMP)" \
		sh scripts/verify-objects.sh $(OUTPUTS)

imports: objects

docs:
	python3 scripts/check_docs.py

technique-contract:
	python3 scripts/check_technique_contract.py

sliver-manifest:
	python3 scripts/check_sliver_manifest.py integrations/sliver/extension.json

sliver-entrypoints: all
	LLVM_NM="$(LLVM_NM)" python3 scripts/check_sliver_entrypoints.py integrations/sliver/extension.json

verify-bof: objects
	@for object in $(OUTPUTS); do \
		python3 $(BOFLINT) --loader cs "$$object" || exit 1; \
	done

release: verify
	@test -n "$(VERSION)" || { echo "VERSION is required (for example, VERSION=v0.1.0)" >&2; exit 2; }
	sh scripts/package-release.sh "$(VERSION)"
	sh scripts/package-sliver-release.sh "$(VERSION)"

# Render the README demo locally. Generated media is committed so CI and
# documentation rendering do not depend on beaconcast.
demo:
	@command -v beaconcast >/dev/null || { echo "Install beaconcast: go install github.com/rubyattack3r/beaconcast/cmd/beaconcast@v0.1.0" >&2; exit 1; }
	@for mode in dark light; do \
		beaconcast render --mode $$mode \
			--svg docs/demo/demo-$$mode.svg \
			--gif docs/demo/demo-$$mode.gif docs/demo/demo.toml; \
	done

define TECHNIQUE_template
$(OUTPUT_DIR)/rex_$(1).x64.o: src/technique_unit.c $$($(1)_SOURCES) $$(HEADERS) Makefile
	@mkdir -p $$(@D)
	$$(ZIG) cc -target $$(TARGET_x64) $$(CPPFLAGS) $$(BOF_CFLAGS) \
		$$($(1)_DEFINES) -DREX_TECHNIQUE_SOURCE=\"techniques/$(1).c\" \
		-c $$< -o $$@
	$$(LLVM_OBJCOPY) --strip-unneeded $$@

$(OUTPUT_DIR)/rex_$(1).x86.o: src/technique_unit.c $$($(1)_SOURCES) $$(HEADERS) Makefile
	@mkdir -p $$(@D)
	$$(ZIG) cc -target $$(TARGET_x86) $$(CPPFLAGS) $$(BOF_CFLAGS) \
		$$($(1)_DEFINES) -DREX_TECHNIQUE_SOURCE=\"techniques/$(1).c\" \
		-c $$< -o $$@
	$$(LLVM_OBJCOPY) --strip-unneeded $$@
endef

$(foreach technique,$(TECHNIQUES),$(eval $(call TECHNIQUE_template,$(technique))))

$(SLIVER_OUTPUT_DIR)/%.o: $(OUTPUT_DIR)/%.o
	@mkdir -p $(@D)
	cp $< $@

clean:
	$(RM) -r .build $(OUTPUT_DIR)/rex_*.o $(SLIVER_OUTPUT_DIR)/rex_*.o

