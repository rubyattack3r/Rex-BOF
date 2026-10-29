CC_X64 := x86_64-w64-mingw32-gcc
LD_X64 := x86_64-w64-mingw32-ld
NM_X64 := x86_64-w64-mingw32-nm
CC_X86 := i686-w64-mingw32-gcc
LD_X86 := i686-w64-mingw32-ld
NM_X86 := i686-w64-mingw32-nm
CFLAGS := -Os -Wall -Wextra -Werror -fno-asynchronous-unwind-tables -fno-builtin -fno-ident
CPPFLAGS := -Iinclude -Isrc/common -Isrc/techniques
OUTPUT_DIR := integrations/cna
SLIVER_OUTPUT_DIR := integrations/sliver
BOFLINT ?= boflint.py
VERSION ?=

TECHNIQUES := wmiprovider wmiservice wmiexec wsmanexec taskschedexec dcomishelldispatch2 scmchangeservice scmcreateservice
OUTPUTS := $(foreach technique,$(TECHNIQUES),$(OUTPUT_DIR)/rex_$(technique).x64.o $(OUTPUT_DIR)/rex_$(technique).x86.o)
SLIVER_OUTPUTS := $(foreach technique,$(TECHNIQUES),$(SLIVER_OUTPUT_DIR)/rex_$(technique).x64.o $(SLIVER_OUTPUT_DIR)/rex_$(technique).x86.o)

BASE_SOURCES := src/common/rex.c src/common/registry.c src/common/result.c src/common/runtime.c src/common/text.c
WMI_SOURCES := src/common/wmi.c
SERVICE_SOURCES := src/common/service.c

wmiprovider_SOURCES := $(BASE_SOURCES) $(WMI_SOURCES) src/techniques/wmiprovider.c
wmiservice_SOURCES := $(BASE_SOURCES) $(WMI_SOURCES) $(SERVICE_SOURCES) src/techniques/wmiservice.c
wmiexec_SOURCES := $(BASE_SOURCES) $(WMI_SOURCES) src/techniques/wmiexec.c
wsmanexec_SOURCES := $(BASE_SOURCES) src/techniques/wsmanexec.c
taskschedexec_SOURCES := $(BASE_SOURCES) src/techniques/taskschedexec.c
dcomishelldispatch2_SOURCES := $(BASE_SOURCES) src/techniques/dcomishelldispatch2.c
scmchangeservice_SOURCES := $(BASE_SOURCES) $(SERVICE_SOURCES) src/techniques/scmchangeservice.c
scmcreateservice_SOURCES := $(BASE_SOURCES) $(SERVICE_SOURCES) src/techniques/scmcreateservice.c

.PHONY: all clean demo docs imports lint release sliver-entrypoints sliver-manifest technique-contract verify verify-bof

all: $(OUTPUTS) $(SLIVER_OUTPUTS)

verify: clean lint all imports docs technique-contract sliver-manifest sliver-entrypoints

lint:
	cppcheck --quiet --error-exitcode=1 --std=c11 --inline-suppr \
		--enable=warning,portability,performance \
		--suppress=missingIncludeSystem -Iinclude -Isrc/common \
		-Isrc/techniques src tests
	shellcheck --severity=warning scripts/*.sh
	actionlint

imports: all
	@for object in $(OUTPUTS); do \
		case "$$object" in \
			*.x64.o) nm=$(NM_X64) ;; \
			*.x86.o) nm=$(NM_X86) ;; \
		esac; \
		symbols="$$($$nm -u "$$object" | awk '{print $$2}')"; \
		unexpected="$$(printf '%s\n' "$$symbols" | grep -Ev '^(__imp__?)?(BeaconDataExtract|BeaconDataLength|BeaconDataParse|BeaconPrintf|toWideChar|[A-Z0-9]+\$$.+)(@[0-9]+)?$$' || true)"; \
		test -z "$$unexpected" || { printf '%s: unexpected imports\n%s\n' "$$object" "$$unexpected"; exit 1; }; \
	done

docs:
	python3 scripts/check_docs.py

technique-contract:
	python3 scripts/check_technique_contract.py

sliver-manifest:
	python3 scripts/check_sliver_manifest.py integrations/sliver/extension.json

sliver-entrypoints: all
	python3 scripts/check_sliver_entrypoints.py integrations/sliver/extension.json

verify-bof: all
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
$(OUTPUT_DIR)/rex_$(1).x64.o: $$(patsubst %.c,.build/x64/%.o,$$($(1)_SOURCES))
	@mkdir -p $$(@D)
	$$(LD_X64) -r -o $$@ $$^

$(OUTPUT_DIR)/rex_$(1).x86.o: $$(patsubst %.c,.build/x86/%.o,$$($(1)_SOURCES))
	@mkdir -p $$(@D)
	$$(LD_X86) -r -o $$@ $$^
endef

$(foreach technique,$(TECHNIQUES),$(eval $(call TECHNIQUE_template,$(technique))))

$(SLIVER_OUTPUT_DIR)/%.o: $(OUTPUT_DIR)/%.o
	@mkdir -p $(@D)
	cp $< $@

.build/x64/%.o: %.c include/beacon.h include/rex.h src/common/text.h src/common/wmi.h src/common/service.h src/techniques/technique.h Makefile
	@command -v $(CC_X64) >/dev/null || { echo "$(CC_X64) is required to build the x64 BOFs" >&2; exit 1; }
	@mkdir -p $(@D)
	$(CC_X64) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.build/x86/%.o: %.c include/beacon.h include/rex.h src/common/text.h src/common/wmi.h src/common/service.h src/techniques/technique.h Makefile
	@command -v $(CC_X86) >/dev/null || { echo "$(CC_X86) is required to build the x86 BOFs" >&2; exit 1; }
	@mkdir -p $(@D)
	$(CC_X86) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r .build $(OUTPUT_DIR)/rex_*.o $(SLIVER_OUTPUT_DIR)/rex_*.o

